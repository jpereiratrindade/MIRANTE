#include "mirante/storage/database.hpp"
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iostream>
#include <chrono>
#include <format>
#include <cstdlib>
#include <algorithm>

using json = nlohmann::json;

namespace mirante::storage {

Database::~Database() {
    Close();
}

std::string Database::Sha256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, input.data(), input.size());
    unsigned int length = 0;
    EVP_DigestFinal_ex(ctx, hash, &length);
    EVP_MD_CTX_free(ctx);

    std::ostringstream os;
    for (unsigned int i = 0; i < length; ++i) {
        os << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return os.str();
}

void Database::Close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

std::expected<void, std::string> Database::Open(const std::string& db_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (db_) sqlite3_close(db_);

    int rc = sqlite3_open(db_path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string err = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        db_ = nullptr;
        return std::unexpected("Falha ao abrir SQLite: " + err);
    }

    // Configurar WAL mode para concorrência
    char* err_msg = nullptr;
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL; PRAGMA foreign_keys=ON;", nullptr, nullptr, &err_msg);
    if (err_msg) {
        sqlite3_free(err_msg);
    }

    return RunMigrations();
}

std::expected<void, std::string> Database::RunMigrations() {
    const char* schema = R"(
        CREATE TABLE IF NOT EXISTS sessions (
            session_id TEXT PRIMARY KEY,
            project_id TEXT NOT NULL,
            title TEXT NOT NULL,
            entrance_door TEXT NOT NULL,
            state TEXT NOT NULL,
            current_cycle INTEGER NOT NULL,
            data_json TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS mprr_session_events (
            event_id TEXT PRIMARY KEY,
            session_id TEXT NOT NULL,
            sequence_number INTEGER NOT NULL,
            previous_hash TEXT NOT NULL,
            event_type TEXT NOT NULL,
            payload_json TEXT NOT NULL,
            author TEXT NOT NULL,
            timestamp_iso TEXT NOT NULL,
            current_hash TEXT NOT NULL,
            FOREIGN KEY (session_id) REFERENCES sessions(session_id)
        );

        CREATE INDEX IF NOT EXISTS idx_events_session_seq ON mprr_session_events(session_id, sequence_number);

        -- Autenticação SisTer (E-mail + Código OTP + Sessões)
        CREATE TABLE IF NOT EXISTS users (
            email TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            role TEXT DEFAULT 'researcher',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            last_login_at DATETIME
        );

        CREATE TABLE IF NOT EXISTS otps (
            email TEXT PRIMARY KEY,
            code TEXT NOT NULL,
            expires_at INTEGER NOT NULL,
            attempts INTEGER DEFAULT 0,
            requested_at INTEGER NOT NULL
        );

        CREATE TABLE IF NOT EXISTS auth_sessions (
            token TEXT PRIMARY KEY,
            email TEXT NOT NULL,
            name TEXT NOT NULL,
            role TEXT NOT NULL,
            created_at INTEGER NOT NULL,
            expires_at INTEGER NOT NULL,
            FOREIGN KEY (email) REFERENCES users(email)
        );
        CREATE INDEX IF NOT EXISTS idx_auth_sessions_token ON auth_sessions(token, expires_at);
    )";

    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, schema, nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        std::string err = err_msg ? err_msg : "Erro desconhecido ao rodar migrações";
        sqlite3_free(err_msg);
        return std::unexpected(err);
    }
    return {};
}

std::string Database::GetLastEventHash(const std::string& session_id) {
    const char* query = "SELECT current_hash FROM mprr_session_events WHERE session_id = ? ORDER BY sequence_number DESC LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, query, -1, &stmt, nullptr) != SQLITE_OK) {
        return "GENESIS_0000000000000000000000000000000000000000000000000000000000000000";
    }

    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
    std::string last_hash = "GENESIS_0000000000000000000000000000000000000000000000000000000000000000";
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* h = sqlite3_column_text(stmt, 0);
        if (h) last_hash = reinterpret_cast<const char*>(h);
    }
    sqlite3_finalize(stmt);
    return last_hash;
}

std::expected<std::string, std::string> Database::AppendEvent(
    const std::string& session_id,
    const std::string& event_type,
    const std::string& payload_json,
    const std::string& author
) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) return std::unexpected("Banco de dados não aberto");

    std::string prev_hash = GetLastEventHash(session_id);

    // Contar sequência
    uint64_t seq = 1;
    const char* seq_query = "SELECT COALESCE(MAX(sequence_number), 0) + 1 FROM mprr_session_events WHERE session_id = ?;";
    sqlite3_stmt* seq_stmt = nullptr;
    if (sqlite3_prepare_v2(db_, seq_query, -1, &seq_stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(seq_stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(seq_stmt) == SQLITE_ROW) {
            seq = sqlite3_column_int64(seq_stmt, 0);
        }
        sqlite3_finalize(seq_stmt);
    }

    auto now = std::chrono::system_clock::now();
    std::string ts_iso = std::format("{:%FT%TZ}", now);
    std::string event_id = std::format("EVT-{}-{:04d}", session_id, seq);

    // Merkle chain calculation
    std::string raw_chain_data = prev_hash + "|" + ts_iso + "|" + session_id + "|" + event_type + "|" + payload_json + "|" + author;
    std::string cur_hash = Sha256(raw_chain_data);

    const char* insert_sql = R"(
        INSERT INTO mprr_session_events (event_id, session_id, sequence_number, previous_hash, event_type, payload_json, author, timestamp_iso, current_hash)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";
    sqlite3_stmt* insert_stmt = nullptr;
    if (sqlite3_prepare_v2(db_, insert_sql, -1, &insert_stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(sqlite3_errmsg(db_));
    }

    sqlite3_bind_text(insert_stmt, 1, event_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(insert_stmt, 2, session_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(insert_stmt, 3, seq);
    sqlite3_bind_text(insert_stmt, 4, prev_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(insert_stmt, 5, event_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(insert_stmt, 6, payload_json.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(insert_stmt, 7, author.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(insert_stmt, 8, ts_iso.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(insert_stmt, 9, cur_hash.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(insert_stmt);
    sqlite3_finalize(insert_stmt);

    if (rc != SQLITE_DONE) {
        return std::unexpected("Falha ao gravar evento de auditoria: " + std::string(sqlite3_errmsg(db_)));
    }

    return cur_hash;
}

std::expected<void, std::string> Database::SaveSession(const domain::SessionAggregate& session) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) return std::unexpected("Banco de dados não aberto");

    json j;
    j["session_id"] = session.session_id;
    j["project_id"] = session.project_id;
    j["title"] = session.title;
    j["entrance_door"] = domain::ToString(session.entrance_door);
    j["state"] = domain::ToString(session.state);
    j["current_cycle"] = session.current_cycle;
    j["spatial_boundary"] = session.spatial_boundary;
    j["temporal_boundary"] = session.temporal_boundary;
    j["research_context"] = session.research_context;
    j["participants"] = session.participants;
    j["initial_sources"] = session.initial_sources;

    j["phenomenon"] = {
        {"text", session.phenomenon_description.text},
        {"author", session.phenomenon_description.author},
        {"origin", domain::ToString(session.phenomenon_description.origin)}
    };

    if (session.shift.has_value()) {
        j["shift"] = {
            {"id", session.shift->id},
            {"initial_framework", session.shift->initial_framework},
            {"shifted_framework", session.shift->shifted_framework},
            {"dimension", domain::ToString(session.shift->dimension)},
            {"rationale", session.shift->rationale.text},
            {"author", session.shift->rationale.author},
            {"origin", domain::ToString(session.shift->rationale.origin)}
        };
    }

    j["relations"] = json::array();
    for (const auto& r : session.relations) {
        j["relations"].push_back({
            {"id", r.id},
            {"term_a", r.term_a},
            {"term_b", r.term_b},
            {"type", domain::ToString(r.type)},
            {"status", domain::ToString(r.status)},
            {"description", r.description.text},
            {"author", r.description.author},
            {"origin", domain::ToString(r.description.origin)}
        });
    }

    j["questions"] = json::array();
    for (const auto& q : session.questions) {
        j["questions"].push_back({
            {"id", q.id},
            {"relation_id", q.relation_id},
            {"text", q.question_text.text},
            {"evidence_path", q.required_evidence_path},
            {"author", q.question_text.author},
            {"origin", domain::ToString(q.question_text.origin)}
        });
    }

    j["hypotheses"] = json::array();
    for (const auto& h : session.hypotheses) {
        j["hypotheses"].push_back({
            {"id", h.id},
            {"question_id", h.question_id},
            {"formulation", h.formulation.text},
            {"status", domain::ToString(h.status)},
            {"cycle_version", h.cycle_version}
        });
    }

    if (session.confrontation.has_value()) {
        json j_conf;
        j_conf["id"] = session.confrontation->id;
        j_conf["hypothesis_id"] = session.confrontation->hypothesis_id;
        j_conf["is_concluded"] = session.confrontation->is_concluded;
        
        j_conf["rival_explanations"] = json::array();
        for (const auto& r : session.confrontation->rival_explanations) {
            j_conf["rival_explanations"].push_back({{"text", r.text}, {"author", r.author}, {"origin", domain::ToString(r.origin)}});
        }
        j_conf["epistemic_risks"] = json::array();
        for (const auto& r : session.confrontation->epistemic_risks) {
            j_conf["epistemic_risks"].push_back({{"text", r.text}, {"author", r.author}, {"origin", domain::ToString(r.origin)}});
        }
        j["confrontation"] = j_conf;
    }

    j["repositions"] = json::array();
    for (const auto& rep : session.reposition_history) {
        j["repositions"].push_back({
            {"id", rep.id},
            {"cycle", rep.cycle_version},
            {"decision", domain::ToString(rep.decision)},
            {"justification", rep.verdict_justification.text},
            {"next_empirical_test", rep.next_empirical_test},
            {"required_knowledge", rep.required_knowledge},
            {"resumption_criteria", rep.resumption_criteria}
        });
    }

    std::string data_str = j.dump(2);

    const char* sql = R"(
        INSERT INTO sessions (session_id, project_id, title, entrance_door, state, current_cycle, data_json, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP)
        ON CONFLICT(session_id) DO UPDATE SET
            title = excluded.title,
            state = excluded.state,
            current_cycle = excluded.current_cycle,
            data_json = excluded.data_json,
            updated_at = CURRENT_TIMESTAMP;
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(sqlite3_errmsg(db_));
    }

    std::string door_str = std::string(domain::ToString(session.entrance_door));
    std::string state_str = std::string(domain::ToString(session.state));

    sqlite3_bind_text(stmt, 1, session.session_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, session.project_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, session.title.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, door_str.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, state_str.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, static_cast<int>(session.current_cycle));
    sqlite3_bind_text(stmt, 7, data_str.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return std::unexpected(sqlite3_errmsg(db_));
    }
    return {};
}

std::expected<std::string, std::string> Database::SaveSessionWithEvent(
    const domain::SessionAggregate& session,
    const std::string& event_type,
    const std::string& payload_json,
    const std::string& author
) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) return std::unexpected("Banco de dados não aberto");

    char* err_msg = nullptr;
    if (sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::string err = err_msg ? err_msg : "Erro ao iniciar transação SQLite";
        sqlite3_free(err_msg);
        return std::unexpected(err);
    }

    // 1. Gravar Sessão
    json j;
    j["session_id"] = session.session_id;
    j["project_id"] = session.project_id;
    j["title"] = session.title;
    j["entrance_door"] = domain::ToString(session.entrance_door);
    j["state"] = domain::ToString(session.state);
    j["current_cycle"] = session.current_cycle;
    j["spatial_boundary"] = session.spatial_boundary;
    j["temporal_boundary"] = session.temporal_boundary;
    j["research_context"] = session.research_context;
    j["participants"] = session.participants;
    j["initial_sources"] = session.initial_sources;

    j["phenomenon"] = {
        {"text", session.phenomenon_description.text},
        {"author", session.phenomenon_description.author},
        {"origin", domain::ToString(session.phenomenon_description.origin)}
    };

    if (session.shift.has_value()) {
        j["shift"] = {
            {"id", session.shift->id},
            {"initial_framework", session.shift->initial_framework},
            {"shifted_framework", session.shift->shifted_framework},
            {"dimension", domain::ToString(session.shift->dimension)},
            {"rationale", session.shift->rationale.text},
            {"author", session.shift->rationale.author},
            {"origin", domain::ToString(session.shift->rationale.origin)}
        };
    }

    j["relations"] = json::array();
    for (const auto& r : session.relations) {
        j["relations"].push_back({
            {"id", r.id},
            {"term_a", r.term_a},
            {"term_b", r.term_b},
            {"type", domain::ToString(r.type)},
            {"status", domain::ToString(r.status)},
            {"description", r.description.text},
            {"author", r.description.author},
            {"origin", domain::ToString(r.description.origin)}
        });
    }

    j["questions"] = json::array();
    for (const auto& q : session.questions) {
        j["questions"].push_back({
            {"id", q.id},
            {"relation_id", q.relation_id},
            {"text", q.question_text.text},
            {"evidence_path", q.required_evidence_path},
            {"author", q.question_text.author},
            {"origin", domain::ToString(q.question_text.origin)}
        });
    }

    j["hypotheses"] = json::array();
    for (const auto& h : session.hypotheses) {
        j["hypotheses"].push_back({
            {"id", h.id},
            {"question_id", h.question_id},
            {"formulation", h.formulation.text},
            {"status", domain::ToString(h.status)},
            {"cycle_version", h.cycle_version}
        });
    }

    if (session.confrontation.has_value()) {
        json j_conf;
        j_conf["id"] = session.confrontation->id;
        j_conf["hypothesis_id"] = session.confrontation->hypothesis_id;
        j_conf["is_concluded"] = session.confrontation->is_concluded;
        
        j_conf["rival_explanations"] = json::array();
        for (const auto& r : session.confrontation->rival_explanations) {
            j_conf["rival_explanations"].push_back({{"text", r.text}, {"author", r.author}, {"origin", domain::ToString(r.origin)}});
        }
        j_conf["epistemic_risks"] = json::array();
        for (const auto& r : session.confrontation->epistemic_risks) {
            j_conf["epistemic_risks"].push_back({{"text", r.text}, {"author", r.author}, {"origin", domain::ToString(r.origin)}});
        }
        j["confrontation"] = j_conf;
    }

    j["repositions"] = json::array();
    for (const auto& rep : session.reposition_history) {
        j["repositions"].push_back({
            {"id", rep.id},
            {"cycle", rep.cycle_version},
            {"decision", domain::ToString(rep.decision)},
            {"justification", rep.verdict_justification.text},
            {"next_empirical_test", rep.next_empirical_test},
            {"required_knowledge", rep.required_knowledge},
            {"resumption_criteria", rep.resumption_criteria}
        });
    }

    std::string data_str = j.dump(2);

    const char* sql = R"(
        INSERT INTO sessions (session_id, project_id, title, entrance_door, state, current_cycle, data_json, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP)
        ON CONFLICT(session_id) DO UPDATE SET
            title = excluded.title,
            state = excluded.state,
            current_cycle = excluded.current_cycle,
            data_json = excluded.data_json,
            updated_at = CURRENT_TIMESTAMP;
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        return std::unexpected(sqlite3_errmsg(db_));
    }

    std::string door_str = std::string(domain::ToString(session.entrance_door));
    std::string state_str = std::string(domain::ToString(session.state));

    sqlite3_bind_text(stmt, 1, session.session_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, session.project_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, session.title.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, door_str.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, state_str.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, static_cast<int>(session.current_cycle));
    sqlite3_bind_text(stmt, 7, data_str.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::string err = sqlite3_errmsg(db_);
        sqlite3_finalize(stmt);
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        return std::unexpected("Falha ao salvar sessão na transação: " + err);
    }
    sqlite3_finalize(stmt);

    // 2. Gravar Evento no Merkle-Chain
    std::string prev_hash = GetLastEventHash(session.session_id);
    uint64_t seq = 1;
    const char* seq_query = "SELECT COALESCE(MAX(sequence_number), 0) + 1 FROM mprr_session_events WHERE session_id = ?;";
    sqlite3_stmt* seq_stmt = nullptr;
    if (sqlite3_prepare_v2(db_, seq_query, -1, &seq_stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(seq_stmt, 1, session.session_id.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(seq_stmt) == SQLITE_ROW) {
            seq = sqlite3_column_int64(seq_stmt, 0);
        }
        sqlite3_finalize(seq_stmt);
    }

    auto now = std::chrono::system_clock::now();
    std::string ts_iso = std::format("{:%FT%TZ}", now);
    std::string event_id = std::format("EVT-{}-{:04d}", session.session_id, seq);
    std::string raw_chain_data = prev_hash + "|" + ts_iso + "|" + session.session_id + "|" + event_type + "|" + payload_json + "|" + author;
    std::string cur_hash = Sha256(raw_chain_data);

    const char* insert_sql = R"(
        INSERT INTO mprr_session_events (event_id, session_id, sequence_number, previous_hash, event_type, payload_json, author, timestamp_iso, current_hash)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";
    sqlite3_stmt* insert_stmt = nullptr;
    if (sqlite3_prepare_v2(db_, insert_sql, -1, &insert_stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        return std::unexpected(sqlite3_errmsg(db_));
    }

    sqlite3_bind_text(insert_stmt, 1, event_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(insert_stmt, 2, session.session_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(insert_stmt, 3, seq);
    sqlite3_bind_text(insert_stmt, 4, prev_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(insert_stmt, 5, event_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(insert_stmt, 6, payload_json.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(insert_stmt, 7, author.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(insert_stmt, 8, ts_iso.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(insert_stmt, 9, cur_hash.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(insert_stmt) != SQLITE_DONE) {
        std::string err = sqlite3_errmsg(db_);
        sqlite3_finalize(insert_stmt);
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        return std::unexpected("Falha ao registrar evento na transação: " + err);
    }
    sqlite3_finalize(insert_stmt);

    // 3. Confirmar Transação
    if (sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::string err = err_msg ? err_msg : "Erro ao confirmar transação";
        sqlite3_free(err_msg);
        return std::unexpected(err);
    }

    return cur_hash;
}

std::expected<domain::SessionAggregate, std::string> Database::LoadSession(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) return std::unexpected("Banco de dados não aberto");

    const char* sql = "SELECT data_json FROM sessions WHERE session_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(sqlite3_errmsg(db_));
    }

    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return std::unexpected("Sessão não encontrada: " + session_id);
    }

    const unsigned char* raw_json = sqlite3_column_text(stmt, 0);
    std::string j_str = raw_json ? reinterpret_cast<const char*>(raw_json) : "{}";
    sqlite3_finalize(stmt);

    try {
        json j = json::parse(j_str);
        domain::SessionAggregate s;
        s.session_id = j.value("session_id", session_id);
        s.project_id = j.value("project_id", "");
        s.title = j.value("title", "");
        s.current_cycle = j.value("current_cycle", 1);
        s.spatial_boundary = j.value("spatial_boundary", "");
        s.temporal_boundary = j.value("temporal_boundary", "");
        s.research_context = j.value("research_context", "");

        if (j.contains("phenomenon")) {
            s.phenomenon_description.text = j["phenomenon"].value("text", "");
            s.phenomenon_description.author = j["phenomenon"].value("author", "");
        }

        if (j.contains("shift")) {
            domain::PerspectiveShift ps;
            ps.id = j["shift"].value("id", "");
            ps.initial_framework = j["shift"].value("initial_framework", "");
            ps.shifted_framework = j["shift"].value("shifted_framework", "");
            ps.rationale.text = j["shift"].value("rationale", "");
            ps.rationale.author = j["shift"].value("author", "");
            s.shift = ps;
        }

        if (j.contains("relations")) {
            for (const auto& rj : j["relations"]) {
                domain::CandidateRelation r;
                r.id = rj.value("id", "");
                r.term_a = rj.value("term_a", "");
                r.term_b = rj.value("term_b", "");
                r.description.text = rj.value("description", "");
                r.description.author = rj.value("author", "");
                s.relations.push_back(r);
            }
        }

        if (j.contains("questions")) {
            for (const auto& qj : j["questions"]) {
                domain::ScientificQuestion q;
                q.id = qj.value("id", "");
                q.relation_id = qj.value("relation_id", "");
                q.question_text.text = qj.value("text", "");
                q.required_evidence_path = qj.value("evidence_path", "");
                q.question_text.author = qj.value("author", "");
                s.questions.push_back(q);
            }
        }

        if (j.contains("hypotheses")) {
            for (const auto& hj : j["hypotheses"]) {
                domain::Hypothesis h;
                h.id = hj.value("id", "");
                h.question_id = hj.value("question_id", "");
                h.formulation.text = hj.value("formulation", "");
                h.cycle_version = hj.value("cycle_version", 1);
                s.hypotheses.push_back(h);
            }
        }

        if (j.contains("confrontation")) {
            domain::ConfrontationRecord cr;
            cr.id = j["confrontation"].value("id", "");
            cr.hypothesis_id = j["confrontation"].value("hypothesis_id", "");
            cr.is_concluded = j["confrontation"].value("is_concluded", false);
            if (j["confrontation"].contains("rival_explanations")) {
                for (const auto& item : j["confrontation"]["rival_explanations"]) {
                    cr.rival_explanations.push_back(domain::ProvenancedText{
                        .text = item.value("text", ""),
                        .origin = domain::ProvenanceOrigin::Human,
                        .author = item.value("author", ""),
                        .model_digest = "",
                        .citation = "",
                        .timestamp_iso = ""
                    });
                }
            }
            if (j["confrontation"].contains("epistemic_risks")) {
                for (const auto& item : j["confrontation"]["epistemic_risks"]) {
                    cr.epistemic_risks.push_back(domain::ProvenancedText{
                        .text = item.value("text", ""),
                        .origin = domain::ProvenanceOrigin::Human,
                        .author = item.value("author", ""),
                        .model_digest = "",
                        .citation = "",
                        .timestamp_iso = ""
                    });
                }
            }
            s.confrontation = cr;
        }

        return s;
    } catch (const std::exception& e) {
        return std::unexpected("Falha no parse do JSON da sessão: " + std::string(e.what()));
    }
}

std::expected<std::vector<domain::SessionAggregate>, std::string> Database::ListSessions() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) return std::unexpected("Banco de dados não aberto");

    const char* sql = "SELECT session_id, project_id, title, entrance_door, state, current_cycle FROM sessions ORDER BY updated_at DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(sqlite3_errmsg(db_));
    }

    std::vector<domain::SessionAggregate> list;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        domain::SessionAggregate s;
        s.session_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        s.project_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        s.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        s.current_cycle = sqlite3_column_int(stmt, 5);
        list.push_back(s);
    }
    sqlite3_finalize(stmt);
    return list;
}

std::expected<std::vector<domain::SessionEvent>, std::string> Database::GetEventLedger(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) return std::unexpected("Banco de dados não aberto");

    const char* sql = R"(
        SELECT event_id, sequence_number, previous_hash, event_type, payload_json, author, timestamp_iso, current_hash
        FROM mprr_session_events
        WHERE session_id = ?
        ORDER BY sequence_number ASC;
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(sqlite3_errmsg(db_));
    }

    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
    std::vector<domain::SessionEvent> events;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        events.push_back(domain::SessionEvent{
            .event_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)),
            .session_id = session_id,
            .sequence_number = static_cast<uint64_t>(sqlite3_column_int64(stmt, 1)),
            .previous_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),
            .event_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)),
            .payload_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)),
            .author = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)),
            .timestamp_iso = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)),
            .current_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7))
        });
    }

    sqlite3_finalize(stmt);
    return events;
}

std::expected<bool, std::string> Database::VerifyLedgerIntegrity(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) return std::unexpected("Banco de dados não aberto");

    const char* sql = R"(
        SELECT event_id, sequence_number, previous_hash, event_type, payload_json, author, timestamp_iso, current_hash
        FROM mprr_session_events
        WHERE session_id = ?
        ORDER BY sequence_number ASC;
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(sqlite3_errmsg(db_));
    }

    sqlite3_bind_text(stmt, 1, session_id.c_str(), -1, SQLITE_STATIC);
    std::string expected_prev_hash = "GENESIS_0000000000000000000000000000000000000000000000000000000000000000";

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string prev_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string ev_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        std::string payload = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        std::string author = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        std::string ts_iso = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        std::string cur_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));

        if (prev_hash != expected_prev_hash) {
            sqlite3_finalize(stmt);
            return false; // Quebra na cadeia
        }

        std::string recalculated = Sha256(prev_hash + "|" + ts_iso + "|" + session_id + "|" + ev_type + "|" + payload + "|" + author);
        if (recalculated != cur_hash) {
            sqlite3_finalize(stmt);
            return false; // Conteúdo adulterado
        }

        expected_prev_hash = cur_hash;
    }

    sqlite3_finalize(stmt);
    return true; // Cadeia íntegra!
}

std::string Database::RandomDigits(size_t count) {
    std::string out;
    while (out.size() < count) {
        unsigned char buf[16];
        if (RAND_bytes(buf, sizeof(buf)) != 1) {
            return "123456";
        }
        for (size_t i = 0; i < sizeof(buf) && out.size() < count; ++i) {
            if (buf[i] < 250) {
                out += static_cast<char>('0' + (buf[i] % 10));
            }
        }
    }
    return out;
}

std::string Database::RandomHex(size_t bytes) {
    std::vector<unsigned char> buf(bytes);
    if (RAND_bytes(buf.data(), bytes) != 1) return "";
    std::ostringstream os;
    for (unsigned char c : buf) {
        os << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
    return os.str();
}

bool Database::SendOtpEmail(const std::string& to_email, const std::string& code, std::string& err_out) {
    const char* smtp_host = std::getenv("MIRANTE_SMTP_HOST");
    if (!smtp_host || std::strlen(smtp_host) == 0) {
        std::cout << "\n========================================================" << std::endl;
        std::cout << " [AUTENTICAÇÃO SISTER] Código de Entrada para: " << to_email << std::endl;
        std::cout << " CÓDIGO TEMPORÁRIO (OTP): >>> " << code << " <<<" << std::endl;
        std::cout << " Válido por 10 minutos (Ambiente Local / Dev)" << std::endl;
        std::cout << "========================================================\n" << std::endl;
        return true;
    }

    std::string port = std::getenv("MIRANTE_SMTP_PORT") ? std::getenv("MIRANTE_SMTP_PORT") : "587";
    std::string user = std::getenv("MIRANTE_SMTP_USER") ? std::getenv("MIRANTE_SMTP_USER") : "";
    std::string pass = std::getenv("MIRANTE_SMTP_PASS") ? std::getenv("MIRANTE_SMTP_PASS") : "";
    std::string from = std::getenv("MIRANTE_SMTP_FROM") ? std::getenv("MIRANTE_SMTP_FROM") : "mprr@labeco.invalid";

    std::string temp_file = std::format("/tmp/mirante-otp-{}.eml", RandomHex(6));
    {
        std::ofstream ofs(temp_file);
        if (!ofs) { err_out = "Erro ao criar arquivo de e-mail temporário"; return false; }
        ofs << "From: \"SisTer MIRANTE\" <" << from << ">\r\n"
            << "To: <" << to_email << ">\r\n"
            << "Subject: MIRANTE - Codigo de Entrada: " << code << "\r\n"
            << "Content-Type: text/plain; charset=utf-8\r\n\r\n"
            << "Olá!\r\n\r\n"
            << "Você solicitou acesso ao MIRANTE (Ecossistema SisTer / LABECO).\r\n"
            << "Seu código temporário de uso único é:\r\n\r\n"
            << "    " << code << "\r\n\r\n"
            << "Este código é válido por 10 minutos.\r\n"
            << "Se você não solicitou este acesso, desconsidere esta mensagem.\r\n\r\n"
            << "--\r\n"
            << "MIRANTE · Experiência Assistida de Percepção Relacional\r\n"
            << "Sempre pronto. Sempre incompleto!\r\n";
    }

    std::string clean_pass = pass;
    clean_pass.erase(std::remove_if(clean_pass.begin(), clean_pass.end(), [](unsigned char c) { return std::isspace(c); }), clean_pass.end());

    std::string cmd = std::format(
        "curl --silent --show-error --connect-timeout 10 --max-time 20 --url 'smtp://{}:{}' "
        "--mail-from '{}' --mail-rcpt '{}' --upload-file '{}' --ssl-reqd --user '{}:{}' 2>&1",
        smtp_host, port, from, to_email, temp_file, user, clean_pass
    );

    FILE* p = popen(cmd.c_str(), "r");
    std::string curl_out;
    if (p) {
        char buf[512];
        while (fgets(buf, sizeof(buf), p)) curl_out += buf;
        int st = pclose(p);
        std::filesystem::remove(temp_file);
        if (st != 0) {
            err_out = "Falha no envio SMTP: " + curl_out;
            return false;
        }
    } else {
        std::filesystem::remove(temp_file);
        err_out = "Não foi possível disparar curl para SMTP";
        return false;
    }
    return true;
}

std::expected<std::string, std::string> Database::RequestOtp(const std::string& raw_email, const std::string& name_hint) {
    std::string email = raw_email;
    email.erase(0, email.find_first_not_of(" \t\r\n"));
    email.erase(email.find_last_not_of(" \t\r\n") + 1);
    std::transform(email.begin(), email.end(), email.begin(), [](unsigned char c) { return std::tolower(c); });

    size_t at = email.find('@');
    if (at == std::string::npos || at == 0 || at + 3 > email.size() || email.find('.', at + 2) == std::string::npos) {
        return std::unexpected("Endereço de e-mail inválido.");
    }

    std::string code = RandomDigits(6);
    int64_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t expires_at = now + 600; // 10 minutos

    std::string default_name = name_hint.empty() ? email.substr(0, at) : name_hint;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!db_) return std::unexpected("Banco de dados não aberto");

        // Cadastra ou atualiza usuário
        sqlite3_stmt* u_stmt = nullptr;
        const char* u_sql = "INSERT OR IGNORE INTO users (email, name, role) VALUES (?, ?, 'researcher');";
        if (sqlite3_prepare_v2(db_, u_sql, -1, &u_stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(u_stmt, 1, email.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(u_stmt, 2, default_name.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(u_stmt);
            sqlite3_finalize(u_stmt);
        }

        // Grava OTP
        sqlite3_stmt* o_stmt = nullptr;
        const char* o_sql = "INSERT OR REPLACE INTO otps (email, code, expires_at, attempts, requested_at) VALUES (?, ?, ?, 0, ?);";
        if (sqlite3_prepare_v2(db_, o_sql, -1, &o_stmt, nullptr) != SQLITE_OK) {
            return std::unexpected(sqlite3_errmsg(db_));
        }

        sqlite3_bind_text(o_stmt, 1, email.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(o_stmt, 2, code.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(o_stmt, 3, expires_at);
        sqlite3_bind_int64(o_stmt, 4, now);
        int rc = sqlite3_step(o_stmt);
        sqlite3_finalize(o_stmt);

        if (rc != SQLITE_DONE) {
            return std::unexpected("Falha ao registrar código de autenticação: " + std::string(sqlite3_errmsg(db_)));
        }
    }

    std::string smtp_err;
    if (!SendOtpEmail(email, code, smtp_err)) {
        return std::unexpected("Erro no envio do e-mail com código: " + smtp_err);
    }

    return code; // Retornado para uso local/dev e confirmação
}

std::expected<std::pair<std::string, Database::AuthUser>, std::string> Database::VerifyOtp(const std::string& raw_email, const std::string& raw_code) {
    std::string email = raw_email;
    email.erase(0, email.find_first_not_of(" \t\r\n"));
    email.erase(email.find_last_not_of(" \t\r\n") + 1);
    std::transform(email.begin(), email.end(), email.begin(), [](unsigned char c) { return std::tolower(c); });

    std::string code = raw_code;
    code.erase(0, code.find_first_not_of(" \t\r\n"));
    code.erase(code.find_last_not_of(" \t\r\n") + 1);

    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) return std::unexpected("Banco de dados não aberto");

    int64_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    // Busca OTP
    const char* o_sql = "SELECT code, expires_at, attempts FROM otps WHERE email = ?;";
    sqlite3_stmt* o_stmt = nullptr;
    if (sqlite3_prepare_v2(db_, o_sql, -1, &o_stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(sqlite3_errmsg(db_));
    }
    sqlite3_bind_text(o_stmt, 1, email.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(o_stmt) != SQLITE_ROW) {
        sqlite3_finalize(o_stmt);
        return std::unexpected("Nenhum código ativo encontrado para este e-mail. Solicite um novo código.");
    }

    std::string stored_code = reinterpret_cast<const char*>(sqlite3_column_text(o_stmt, 0));
    int64_t expires_at = sqlite3_column_int64(o_stmt, 1);
    int attempts = sqlite3_column_int(o_stmt, 2);
    sqlite3_finalize(o_stmt);

    if (now > expires_at) {
        return std::unexpected("Código de entrada expirado. Solicite um novo código.");
    }

    if (attempts >= 5) {
        return std::unexpected("Número excessivo de tentativas incorretas. Solicite um novo código.");
    }

    if (code != stored_code) {
        // Incrementa tentativas
        sqlite3_stmt* inc_stmt = nullptr;
        if (sqlite3_prepare_v2(db_, "UPDATE otps SET attempts = attempts + 1 WHERE email = ?;", -1, &inc_stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(inc_stmt, 1, email.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(inc_stmt);
            sqlite3_finalize(inc_stmt);
        }
        return std::unexpected("Código de entrada incorreto.");
    }

    // Código correto! Remove da tabela otps
    sqlite3_stmt* del_stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "DELETE FROM otps WHERE email = ?;", -1, &del_stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(del_stmt, 1, email.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(del_stmt);
        sqlite3_finalize(del_stmt);
    }

    // Busca dados do usuário
    std::string user_name = email;
    std::string user_role = "researcher";
    sqlite3_stmt* u_stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "SELECT name, role FROM users WHERE email = ?;", -1, &u_stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(u_stmt, 1, email.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(u_stmt) == SQLITE_ROW) {
            const char* n = reinterpret_cast<const char*>(sqlite3_column_text(u_stmt, 0));
            const char* r = reinterpret_cast<const char*>(sqlite3_column_text(u_stmt, 1));
            if (n) user_name = n;
            if (r) user_role = r;
        }
        sqlite3_finalize(u_stmt);
    }

    // Atualiza last_login_at
    sqlite3_stmt* up_stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "UPDATE users SET last_login_at = CURRENT_TIMESTAMP WHERE email = ?;", -1, &up_stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(up_stmt, 1, email.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(up_stmt);
        sqlite3_finalize(up_stmt);
    }

    // Cria Token de Sessão (30 dias)
    std::string token = RandomHex(32);
    int64_t session_exp = now + (86400 * 30);

    sqlite3_stmt* s_stmt = nullptr;
    const char* s_sql = "INSERT INTO auth_sessions (token, email, name, role, created_at, expires_at) VALUES (?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db_, s_sql, -1, &s_stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(sqlite3_errmsg(db_));
    }

    sqlite3_bind_text(s_stmt, 1, token.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(s_stmt, 2, email.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(s_stmt, 3, user_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(s_stmt, 4, user_role.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(s_stmt, 5, now);
    sqlite3_bind_int64(s_stmt, 6, session_exp);
    int s_rc = sqlite3_step(s_stmt);
    sqlite3_finalize(s_stmt);

    if (s_rc != SQLITE_DONE) {
        return std::unexpected("Falha ao gravar sessão de usuário: " + std::string(sqlite3_errmsg(db_)));
    }

    AuthUser au{
        .authenticated = true,
        .email = email,
        .name = user_name,
        .role = user_role
    };

    return std::make_pair(token, au);
}

std::expected<Database::AuthUser, std::string> Database::AuthenticateToken(const std::string& token) {
    if (token.empty()) return std::unexpected("Token ausente");

    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) return std::unexpected("Banco de dados não aberto");

    int64_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    const char* sql = "SELECT email, name, role FROM auth_sessions WHERE token = ? AND expires_at > ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(sqlite3_errmsg(db_));
    }

    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, now);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return std::unexpected("Sessão inválida ou expirada");
    }

    AuthUser au{
        .authenticated = true,
        .email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)),
        .name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
        .role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))
    };

    sqlite3_finalize(stmt);
    return au;
}

std::expected<void, std::string> Database::RevokeToken(const std::string& token) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!db_) return std::unexpected("Banco de dados não aberto");

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "DELETE FROM auth_sessions WHERE token = ?;", -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    return {};
}

} // namespace mirante::storage
