#include "mirante/http/server.hpp"
#include <httplib/httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <format>
#include <filesystem>

using json = nlohmann::json;

namespace mirante::http {

HttpServer::HttpServer(
    std::shared_ptr<storage::Database> db,
    std::shared_ptr<assist::OllamaClient> ollama,
    std::string web_dir,
    int port
) : db_(std::move(db)), ollama_(std::move(ollama)), web_dir_(std::move(web_dir)), port_(port) {}

HttpServer::~HttpServer() {
    Stop();
}

void HttpServer::Start() {
    httplib::Server svr;

    // CORS headers
    svr.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS, PUT, DELETE"},
        {"Access-Control-Allow-Headers", "Content-Type, Authorization"}
    });

    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });

    // Health Check
    svr.Get("/api/health", [this](const httplib::Request&, httplib::Response& res) {
        json j = {
            {"status", "ok"},
            {"system", "MIRANTE"},
            {"version", "0.1.0"},
            {"ollama_online", ollama_ ? ollama_->IsOnline() : false}
        };
        res.set_content(j.dump(), "application/json");
    });

    // Modelos Locais Disponíveis
    svr.Get("/api/models", [this](const httplib::Request&, httplib::Response& res) {
        json j = json::array();
        if (ollama_) {
            auto models = ollama_->ListModels();
            for (const auto& m : models) {
                j.push_back({
                    {"name", m.name},
                    {"digest", m.digest},
                    {"size_bytes", m.size_bytes}
                });
            }
        }
        res.set_content(j.dump(), "application/json");
    });

    // Listar Sessões
    svr.Get("/api/sessions", [this](const httplib::Request&, httplib::Response& res) {
        auto list_exp = db_->ListSessions();
        if (!list_exp) {
            res.status = 500;
            res.set_content(json({{"error", list_exp.error()}}).dump(), "application/json");
            return;
        }

        json j = json::array();
        for (const auto& s : *list_exp) {
            j.push_back({
                {"session_id", s.session_id},
                {"project_id", s.project_id},
                {"title", s.title},
                {"cycle", s.current_cycle}
            });
        }
        res.set_content(j.dump(), "application/json");
    });

    // Criar Nova Sessão
    svr.Post("/api/sessions", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            domain::SessionAggregate s;
            s.session_id = body.value("session_id", std::format("SES-{}", std::chrono::system_clock::now().time_since_epoch().count() % 1000000));
            s.project_id = body.value("project_id", "PRJ-DEFAULT");
            s.title = body.value("title", "Sessão MPRR");
            
            std::string door_str = body.value("entrance_door", "PATH_B_PHENOMENON_TO_KNOWLEDGE");
            s.entrance_door = (door_str == "PATH_A_KNOWLEDGE_TO_PHENOMENON") 
                ? domain::EntranceDoor::PathA_KnowledgeToPhenomenon 
                : domain::EntranceDoor::PathB_PhenomenonToKnowledge;

            s.spatial_boundary = body.value("spatial_boundary", "");
            s.temporal_boundary = body.value("temporal_boundary", "");
            s.research_context = body.value("research_context", "");
            
            s.phenomenon_description.text = body.value("phenomenon_text", "");
            s.phenomenon_description.author = body.value("author", "Pesquisador");
            s.phenomenon_description.origin = domain::ProvenanceOrigin::Human;

            auto save_res = db_->SaveSession(s);
            if (!save_res) {
                res.status = 500;
                res.set_content(json({{"error", save_res.error()}}).dump(), "application/json");
                return;
            }

            db_->AppendEvent(s.session_id, "SESSION_CREATED", body.dump(), s.phenomenon_description.author);

            res.status = 201;
            res.set_content(json({{"session_id", s.session_id}}).dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    // Obter Sessão
    svr.Get(R"(/api/sessions/([\w\-]+))", [this](const httplib::Request& req, httplib::Response& res) {
        std::string sid = req.matches[1];
        auto s_exp = db_->LoadSession(sid);
        if (!s_exp) {
            res.status = 404;
            res.set_content(json({{"error", s_exp.error()}}).dump(), "application/json");
            return;
        }

        // Carrega representação JSON completa
        res.set_content(json({
            {"session_id", s_exp->session_id},
            {"project_id", s_exp->project_id},
            {"title", s_exp->title},
            {"entrance_door", domain::ToString(s_exp->entrance_door)},
            {"current_cycle", s_exp->current_cycle},
            {"phenomenon", {
                {"text", s_exp->phenomenon_description.text},
                {"author", s_exp->phenomenon_description.author}
            }},
            {"shift", s_exp->shift.has_value() ? json({
                {"id", s_exp->shift->id},
                {"initial", s_exp->shift->initial_framework},
                {"shifted", s_exp->shift->shifted_framework},
                {"dimension", domain::ToString(s_exp->shift->dimension)},
                {"rationale", s_exp->shift->rationale.text}
            }) : json(nullptr)},
            {"relations_count", s_exp->relations.size()},
            {"questions_count", s_exp->questions.size()},
            {"hypotheses_count", s_exp->hypotheses.size()},
            {"confrontation_done", s_exp->confrontation.has_value() && s_exp->confrontation->is_concluded}
        }).dump(), "application/json");
    });

    // M1: Registrar Deslocamento
    svr.Post(R"(/api/sessions/([\w\-]+)/shift)", [this](const httplib::Request& req, httplib::Response& res) {
        std::string sid = req.matches[1];
        auto s_exp = db_->LoadSession(sid);
        if (!s_exp) { res.status = 404; return; }

        try {
            json body = json::parse(req.body);
            domain::ProvenancedText rationale{
                .text = body.value("rationale", ""),
                .origin = body.value("is_assisted", false) ? domain::ProvenanceOrigin::Assisted : domain::ProvenanceOrigin::Human,
                .author = body.value("author", "Pesquisador"),
                .model_digest = body.value("model_digest", ""),
                .timestamp_iso = "2026-10-23T14:00:00Z"
            };

            auto shift_res = s_exp->SetPerspectiveShift(
                body.value("initial_framework", ""),
                body.value("shifted_framework", ""),
                domain::ShiftDimension::TimeHorizon,
                rationale
            );

            if (!shift_res) {
                res.status = 422;
                res.set_content(json({
                    {"error", shift_res.error().message},
                    {"invariant", shift_res.error().invariant_id},
                    {"hint", shift_res.error().corrective_hint}
                }).dump(), "application/json");
                return;
            }

            db_->SaveSession(*s_exp);
            db_->AppendEvent(sid, "M1_PERSPECTIVE_SHIFT", body.dump(), rationale.author);

            res.set_content(json({{"status", "ok"}}).dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    // M2: Adicionar Relação Candidata
    svr.Post(R"(/api/sessions/([\w\-]+)/relation)", [this](const httplib::Request& req, httplib::Response& res) {
        std::string sid = req.matches[1];
        auto s_exp = db_->LoadSession(sid);
        if (!s_exp) { res.status = 404; return; }

        try {
            json body = json::parse(req.body);
            domain::ProvenancedText desc{
                .text = body.value("description", ""),
                .origin = body.value("is_assisted", false) ? domain::ProvenanceOrigin::Assisted : domain::ProvenanceOrigin::Human,
                .author = body.value("author", "Pesquisador"),
                .model_digest = body.value("model_digest", ""),
                .timestamp_iso = "2026-10-23T14:00:00Z"
            };

            auto rel_res = s_exp->AddCandidateRelation(
                body.value("term_a", ""),
                body.value("term_b", ""),
                domain::RelationType::ConjecturedMechanism,
                desc
            );

            if (!rel_res) {
                res.status = 422;
                res.set_content(json({
                    {"error", rel_res.error().message},
                    {"invariant", rel_res.error().invariant_id},
                    {"hint", rel_res.error().corrective_hint}
                }).dump(), "application/json");
                return;
            }

            db_->SaveSession(*s_exp);
            db_->AppendEvent(sid, "M2_CANDIDATE_RELATION", body.dump(), desc.author);

            res.set_content(json({{"status", "ok"}}).dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    // M3: Adicionar Questão Científica
    svr.Post(R"(/api/sessions/([\w\-]+)/question)", [this](const httplib::Request& req, httplib::Response& res) {
        std::string sid = req.matches[1];
        auto s_exp = db_->LoadSession(sid);
        if (!s_exp) { res.status = 404; return; }

        try {
            json body = json::parse(req.body);
            domain::ProvenancedText qtext{
                .text = body.value("question_text", ""),
                .origin = body.value("is_assisted", false) ? domain::ProvenanceOrigin::Assisted : domain::ProvenanceOrigin::Human,
                .author = body.value("author", "Pesquisador"),
                .model_digest = body.value("model_digest", ""),
                .timestamp_iso = "2026-10-23T14:00:00Z"
            };

            auto q_res = s_exp->AddScientificQuestion(
                body.value("relation_id", "REL-001"),
                qtext,
                body.value("evidence_path", "")
            );

            if (!q_res) {
                res.status = 422;
                res.set_content(json({
                    {"error", q_res.error().message},
                    {"invariant", q_res.error().invariant_id},
                    {"hint", q_res.error().corrective_hint}
                }).dump(), "application/json");
                return;
            }

            db_->SaveSession(*s_exp);
            db_->AppendEvent(sid, "M3_SCIENTIFIC_QUESTION", body.dump(), qtext.author);

            res.set_content(json({{"status", "ok"}}).dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    // C4: Registrar Confronto
    svr.Post(R"(/api/sessions/([\w\-]+)/confront)", [this](const httplib::Request& req, httplib::Response& res) {
        std::string sid = req.matches[1];
        auto s_exp = db_->LoadSession(sid);
        if (!s_exp) { res.status = 404; return; }

        try {
            json body = json::parse(req.body);
            std::vector<domain::ProvenancedText> rivals;
            std::vector<domain::ProvenancedText> risks;

            if (body.contains("rivals")) {
                for (const auto& r : body["rivals"]) {
                    rivals.push_back({
                        .text = r.value("text", ""),
                        .origin = domain::ProvenanceOrigin::Human,
                        .author = r.value("author", "Revisor Crítico"),
                        .timestamp_iso = "2026-10-23T14:00:00Z"
                    });
                }
            }

            if (body.contains("risks")) {
                for (const auto& r : body["risks"]) {
                    risks.push_back({
                        .text = r.value("text", ""),
                        .origin = domain::ProvenanceOrigin::Human,
                        .author = r.value("author", "Revisor Crítico"),
                        .timestamp_iso = "2026-10-23T14:00:00Z"
                    });
                }
            }

            auto conf_res = s_exp->RecordConfrontation(rivals, risks, {});
            if (!conf_res) {
                res.status = 422;
                res.set_content(json({
                    {"error", conf_res.error().message},
                    {"invariant", conf_res.error().invariant_id},
                    {"hint", conf_res.error().corrective_hint}
                }).dump(), "application/json");
                return;
            }

            db_->SaveSession(*s_exp);
            db_->AppendEvent(sid, "C4_CONFRONTATION_RECORDED", body.dump(), "Revisor Crítico");

            res.set_content(json({{"status", "ok"}}).dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    // C5: Consolidar Reposicionamento
    svr.Post(R"(/api/sessions/([\w\-]+)/reposition)", [this](const httplib::Request& req, httplib::Response& res) {
        std::string sid = req.matches[1];
        auto s_exp = db_->LoadSession(sid);
        if (!s_exp) { res.status = 404; return; }

        try {
            json body = json::parse(req.body);
            std::string dec_str = body.value("decision", "DEVELOP");
            domain::HypothesisStatus status = domain::HypothesisStatus::Develop;
            if (dec_str == "REFORMULATE") status = domain::HypothesisStatus::Reformulate;
            else if (dec_str == "SUSPEND") status = domain::HypothesisStatus::Suspend;
            else if (dec_str == "REJECT") status = domain::HypothesisStatus::Reject;
            else if (dec_str == "SUSTAINED_BY_SPECIFIC_EVIDENCE") status = domain::HypothesisStatus::SustainedBySpecificEvidence;

            domain::ProvenancedText just{
                .text = body.value("justification", ""),
                .origin = domain::ProvenanceOrigin::Human,
                .author = body.value("author", "Pesquisador"),
                .timestamp_iso = "2026-10-23T14:00:00Z"
            };

            auto rep_res = s_exp->ConsolidateReposition(
                status,
                just,
                body.value("next_empirical_test", ""),
                body.value("required_knowledge", ""),
                body.value("resumption_criteria", "")
            );

            if (!rep_res) {
                res.status = 422;
                res.set_content(json({
                    {"error", rep_res.error().message},
                    {"invariant", rep_res.error().invariant_id},
                    {"hint", rep_res.error().corrective_hint}
                }).dump(), "application/json");
                return;
            }

            db_->SaveSession(*s_exp);
            db_->AppendEvent(sid, "C5_REPOSITION_CONSOLIDATED", body.dump(), just.author);

            res.set_content(json({{"status", "ok"}}).dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    // Rotas de Assistência Local (Ollama)
    svr.Post("/api/assist/shift", [this](const httplib::Request& req, httplib::Response& res) {
        if (!ollama_ || !ollama_->IsOnline()) {
            res.status = 503;
            res.set_content(json({{"error", "Daemon Ollama local offline."}}).dump(), "application/json");
            return;
        }
        try {
            json body = json::parse(req.body);
            std::string model = body.value("model", "qwen2.5:14b");
            auto r = ollama_->SuggestShift(body.value("phenomenon", ""), body.value("current_framework", ""), model);
            if (!r) {
                res.status = 500;
                res.set_content(json({{"error", r.error()}}).dump(), "application/json");
                return;
            }
            res.set_content(json({{"suggestion", *r}, {"model", model}}).dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    svr.Post("/api/assist/confront", [this](const httplib::Request& req, httplib::Response& res) {
        if (!ollama_ || !ollama_->IsOnline()) {
            res.status = 503;
            res.set_content(json({{"error", "Daemon Ollama local offline."}}).dump(), "application/json");
            return;
        }
        try {
            json body = json::parse(req.body);
            std::string model = body.value("model", "qwen2.5:14b");
            auto r = ollama_->ConfrontHypothesis(body.value("hypothesis", ""), model);
            if (!r) {
                res.status = 500;
                res.set_content(json({{"error", r.error()}}).dump(), "application/json");
                return;
            }
            res.set_content(json({{"suggestion", *r}, {"model", model}}).dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    // Servir Arquivos Estáticos da Web
    if (std::filesystem::exists(web_dir_)) {
        svr.set_mount_point("/", web_dir_);
    }

    std::cout << "[MIRANTE] Servidor C++26 iniciado em http://0.0.0.0:" << port_ << std::endl;
    std::cout << "[MIRANTE] Servindo interface web a partir de: " << web_dir_ << std::endl;
    running_ = true;
    svr.listen("0.0.0.0", port_);
    running_ = false;
}

void HttpServer::Stop() {
    running_ = false;
}

} // namespace mirante::http
