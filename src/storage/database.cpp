#include "mirante/storage/database.hpp"
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <format>

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

} // namespace mirante::storage
