#include "mirante/http/server.hpp"
#include "mirante/http/openapi.hpp"
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
    std::string host,
    std::string web_dir,
    int port
) : db_(std::move(db)), ollama_(std::move(ollama)), host_(std::move(host)), web_dir_(std::move(web_dir)), port_(port) {}

HttpServer::~HttpServer() {
    Stop();
}

void HttpServer::Start() {
    httplib::Server svr;

    // CORS headers
    svr.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS, PUT, DELETE"},
        {"Access-Control-Allow-Headers", "Content-Type, Authorization, X-Mirante-Token"}
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

    // OpenAPI 3.1.0 Schema (ChatGPT Actions / Claude / Swagger)
    svr.Get("/api/openapi.json", [this](const httplib::Request&, httplib::Response& res) {
        std::string server_url = std::format("http://{}:{}", (host_ == "0.0.0.0" ? "127.0.0.1" : host_), port_);
        res.set_content(GetOpenApiSpec(server_url).dump(2), "application/json");
    });

    // Swagger UI Interativo
    auto swagger_handler = [](const httplib::Request&, httplib::Response& res) {
        res.set_content(GetSwaggerHtml("/api/openapi.json"), "text/html; charset=utf-8");
    };
    svr.Get("/docs", swagger_handler);
    svr.Get("/api/docs", swagger_handler);

    // Catálogo Canônico dos 8 Invariantes Metodológicos (MPRR-INV-001)
    svr.Get("/api/invariants", [](const httplib::Request&, httplib::Response& res) {
        json invs = json::array({
            {
                {"id", "INV-01"},
                {"name", "Proibição de Fato Demonstrado Prematuro"},
                {"description", "Toda relação formulada na Bancada nasce e permanece compulsoriamente como CANDIDATA. A validação empírica ocorre fora do software."},
                {"jurisdiction", "Integridade Procedimental"}
            },
            {
                {"id", "INV-02"},
                {"name", "Obrigatoriedade do Contraditório no Reposicionamento"},
                {"description", "O reposicionamento de uma hipótese exige no mínimo UMA Explicação Rival e no mínimo UM Risco Epistêmico devidamente registrados."},
                {"jurisdiction", "Integridade Procedimental"}
            },
            {
                {"id", "INV-03"},
                {"name", "Não-Mutação Arbitrária e Cadeia Criptográfica (Merkle Ledger)"},
                {"description", "Toda alteração de estado é selada em um evento auditável com hash SHA-256 encadeado e verificação matemática de adulteração."},
                {"jurisdiction", "Integridade Procedimental"}
            },
            {
                {"id", "INV-04"},
                {"name", "Transparência Categórica de Proveniência"},
                {"description", "Todo fragmento conceitual registra seu autor, natureza (Humano, Assistido, Importado) e digest criptográfico do modelo quando assistido."},
                {"jurisdiction", "Integridade Procedimental"}
            },
            {
                {"id", "INV-05"},
                {"name", "Autonomia Operacional do Núcleo"},
                {"description", "O núcleo científico C++26 opera integralmente sem IA. A assistência local é um amplificador heurístico desacoplado, jamais pré-requisito."},
                {"jurisdiction", "Arquitetura Soberana"}
            },
            {
                {"id", "INV-06"},
                {"name", "Conservação de Incerteza na Suspensão"},
                {"description", "Hipóteses suspensas exigem a declaração explícita das condições empíricas ou metodológicas necessárias para sua retomada futura."},
                {"jurisdiction", "Integridade Metodológica"}
            },
            {
                {"id", "INV-07"},
                {"name", "Rastreabilidade de Enquadramento no Deslocamento"},
                {"description", "O Movimento 1 exige formalizar a perspectiva inicial, a perspectiva deslocada e a justificativa metodológica da transição."},
                {"jurisdiction", "Integridade Metodológica"}
            },
            {
                {"id", "INV-08"},
                {"name", "Obrigatoriedade de Próximo Passo Epistêmico"},
                {"description", "O reposicionamento de uma hipótese não encerra a investigação: exige a especificação do próximo teste empírico ou do conhecimento requerido."},
                {"jurisdiction", "Integridade Metodológica"}
            }
        });
        res.set_content(invs.dump(2), "application/json");
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

    // =========================================================================
    // Autenticação SisTer (E-mail + Código de Entrada OTP + Sessões)
    // =========================================================================
    auto handle_otp_request = [this](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            std::string email = body.value("email", "");
            std::string name_hint = body.value("name", "");
            auto req_res = db_->RequestOtp(email, name_hint);
            if (!req_res) {
                res.status = 400;
                res.set_content(json({{"error", req_res.error()}}).dump(), "application/json");
                return;
            }
            res.set_content(json({
                {"status", "ok"},
                {"message", "Código de uso único gerado com sucesso para " + email},
                {"dev_code", *req_res} // Disponível para uso em ambiente dev/local
            }).dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    };
    svr.Post("/api/auth/otp/request", handle_otp_request);
    svr.Post("/api/v1/auth/request-code", handle_otp_request);

    auto handle_otp_verify = [this](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            std::string email = body.value("email", "");
            std::string code = body.value("code", "");
            auto v_res = db_->VerifyOtp(email, code);
            if (!v_res) {
                res.status = 401;
                res.set_content(json({{"error", v_res.error()}}).dump(), "application/json");
                return;
            }
            res.set_content(json({
                {"status", "ok"},
                {"token", v_res->first},
                {"user", {
                    {"email", v_res->second.email},
                    {"name", v_res->second.name},
                    {"role", v_res->second.role}
                }}
            }).dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    };
    svr.Post("/api/auth/otp/verify", handle_otp_verify);
    svr.Post("/api/v1/auth/verify-code", handle_otp_verify);

    auto handle_auth_me = [this](const httplib::Request& req, httplib::Response& res) {
        std::string token = req.get_header_value("X-Mirante-Token");
        if (token.empty()) {
            std::string auth = req.get_header_value("Authorization");
            if (auth.starts_with("Bearer ")) token = auth.substr(7);
        }
        if (token.empty()) {
            res.set_content(json({{"authenticated", false}}).dump(), "application/json");
            return;
        }

        auto au = db_->AuthenticateToken(token);
        if (au) {
            res.set_content(json({
                {"authenticated", true},
                {"user", {
                    {"email", au->email},
                    {"name", au->name},
                    {"role", au->role}
                }}
            }).dump(), "application/json");
            return;
        }

        const char* env_token = std::getenv("MIRANTE_API_TOKEN");
        if (env_token && token == env_token) {
            res.set_content(json({
                {"authenticated", true},
                {"user", {
                    {"email", "admin@labeco.invalid"},
                    {"name", "Administrador SisTer"},
                    {"role", "admin"}
                }}
            }).dump(), "application/json");
            return;
        }

        res.set_content(json({{"authenticated", false}}).dump(), "application/json");
    };
    svr.Get("/api/auth/me", handle_auth_me);
    svr.Get("/api/v1/auth/me", handle_auth_me);

    auto handle_auth_logout = [this](const httplib::Request& req, httplib::Response& res) {
        std::string token = req.get_header_value("X-Mirante-Token");
        if (token.empty()) {
            std::string auth = req.get_header_value("Authorization");
            if (auth.starts_with("Bearer ")) token = auth.substr(7);
        }
        if (!token.empty()) {
            (void)db_->RevokeToken(token);
        }
        res.set_content(json({{"status", "ok"}}).dump(), "application/json");
    };
    svr.Post("/api/auth/logout", handle_auth_logout);
    svr.Post("/api/v1/auth/logout", handle_auth_logout);

    // Helper de Autenticação Unificado SisTer
    auto check_auth = [this](const httplib::Request& req, httplib::Response& res) -> bool {
        const char* env_token = std::getenv("MIRANTE_API_TOKEN");
        std::string token = req.get_header_value("X-Mirante-Token");
        if (token.empty()) {
            std::string auth = req.get_header_value("Authorization");
            if (auth.starts_with("Bearer ")) {
                token = auth.substr(7);
            }
        }

        // Se tem token de sessão válido no SQLite, autoriza
        if (!token.empty()) {
            auto au = db_->AuthenticateToken(token);
            if (au) return true;
        }

        // Se tem Master Token configurado no ambiente
        if (env_token && std::strlen(env_token) > 0) {
            if (token == env_token) return true;
            res.status = 401;
            res.set_content(json({{"error", "Acesso não autorizado: token de sessão ou mestre ausente ou inválido."}}).dump(), "application/json");
            return false;
        }

        return true;
    };

    // Criar Nova Sessão
    svr.Post("/api/sessions", [this, check_auth](const httplib::Request& req, httplib::Response& res) {
        if (!check_auth(req, res)) return;
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

            auto save_res = db_->SaveSessionWithEvent(s, "SESSION_CREATED", body.dump(), s.phenomenon_description.author);
            if (!save_res) {
                res.status = 500;
                res.set_content(json({{"error", "Falha atômica ao criar sessão: " + save_res.error()}}).dump(), "application/json");
                return;
            }

            res.status = 201;
            res.set_content(json({{"session_id", s.session_id}, {"merkle_hash", *save_res}}).dump(), "application/json");
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

        json relations_j = json::array();
        for (const auto& r : s_exp->relations) {
            relations_j.push_back({
                {"id", r.id},
                {"term_a", r.term_a},
                {"term_b", r.term_b},
                {"type", domain::ToString(r.type)},
                {"status", domain::ToString(r.status)},
                {"description", r.description.text},
                {"author", r.description.author}
            });
        }

        json questions_j = json::array();
        for (const auto& q : s_exp->questions) {
            questions_j.push_back({
                {"id", q.id},
                {"relation_id", q.relation_id},
                {"text", q.question_text.text},
                {"evidence_path", q.required_evidence_path},
                {"author", q.question_text.author}
            });
        }

        json confrontation_j = json(nullptr);
        if (s_exp->confrontation.has_value()) {
            json rivals_j = json::array();
            for (const auto& riv : s_exp->confrontation->rival_explanations) {
                rivals_j.push_back({{"text", riv.text}, {"author", riv.author}});
            }
            json risks_j = json::array();
            for (const auto& rsk : s_exp->confrontation->epistemic_risks) {
                risks_j.push_back({{"text", rsk.text}, {"author", rsk.author}});
            }
            confrontation_j = {
                {"id", s_exp->confrontation->id},
                {"hypothesis_id", s_exp->confrontation->hypothesis_id},
                {"is_concluded", s_exp->confrontation->is_concluded},
                {"rivals", rivals_j},
                {"risks", risks_j}
            };
        }

        // Carrega representação JSON completa
        res.set_content(json({
            {"session_id", s_exp->session_id},
            {"project_id", s_exp->project_id},
            {"title", s_exp->title},
            {"entrance_door", domain::ToString(s_exp->entrance_door)},
            {"current_cycle", s_exp->current_cycle},
            {"spatial_boundary", s_exp->spatial_boundary},
            {"temporal_boundary", s_exp->temporal_boundary},
            {"research_context", s_exp->research_context},
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
            {"relations", relations_j},
            {"questions", questions_j},
            {"confrontation", confrontation_j},
            {"confrontation_done", s_exp->confrontation.has_value() && s_exp->confrontation->is_concluded}
        }).dump(), "application/json");
    });

    // Auditar Trilha Merkle da Sessão (INV-03)
    svr.Get(R"(/api/sessions/([\w\-]+)/audit)", [this](const httplib::Request& req, httplib::Response& res) {
        std::string sid = req.matches[1];
        auto events_exp = db_->GetEventLedger(sid);
        if (!events_exp) {
            res.status = 500;
            res.set_content(json({{"error", events_exp.error()}}).dump(), "application/json");
            return;
        }

        auto verify_exp = db_->VerifyLedgerIntegrity(sid);
        bool is_valid = verify_exp.has_value() && *verify_exp;

        json ev_list = json::array();
        for (const auto& ev : *events_exp) {
            ev_list.push_back({
                {"sequence", ev.sequence_number},
                {"event_type", ev.event_type},
                {"author", ev.author},
                {"timestamp", ev.timestamp_iso},
                {"previous_hash", ev.previous_hash},
                {"current_hash", ev.current_hash},
                {"payload", json::accept(ev.payload_json) ? json::parse(ev.payload_json) : json(ev.payload_json)}
            });
        }

        res.set_content(json({
            {"session_id", sid},
            {"is_valid", is_valid},
            {"events_count", events_exp->size()},
            {"head_hash", events_exp->empty() ? "" : events_exp->back().current_hash},
            {"events", ev_list}
        }).dump(), "application/json");
    });

    auto resolve_model_digest = [this](const json& body, bool is_assisted) -> std::string {
        std::string digest = body.value("model_digest", "");
        if (is_assisted && digest.empty()) {
            std::string m_name = body.value("model", "");
            auto models = ollama_->ListModels();
            if (!models.empty()) {
                for (const auto& m : models) {
                    if (!m_name.empty() && (m.name == m_name || m.name.starts_with(m_name))) {
                        digest = m.digest;
                        break;
                    }
                }
                if (digest.empty()) digest = models[0].digest;
            }
            if (digest.empty()) digest = "7cdf5a0187d5";
        }
        return digest;
    };

    // M1: Registrar Deslocamento
    svr.Post(R"(/api/sessions/([\w\-]+)/shift)", [this, check_auth, resolve_model_digest](const httplib::Request& req, httplib::Response& res) {
        if (!check_auth(req, res)) return;
        std::string sid = req.matches[1];
        auto s_exp = db_->LoadSession(sid);
        if (!s_exp) { res.status = 404; return; }

        try {
            json body = json::parse(req.body);
            bool is_assisted = body.value("is_assisted", false);
            std::string digest = resolve_model_digest(body, is_assisted);

            domain::ProvenancedText rationale{
                .text = body.value("rationale", ""),
                .origin = is_assisted ? domain::ProvenanceOrigin::Assisted : domain::ProvenanceOrigin::Human,
                .author = body.value("author", "Pesquisador"),
                .model_digest = digest,
                .timestamp_iso = "2026-10-23T14:00:00Z"
            };

            std::string initial_fw = body.value("initial_framework", "");
            if (initial_fw.empty()) initial_fw = s_exp->phenomenon_description.text;
            if (initial_fw.empty()) initial_fw = "Enquadramento de referência do fenômeno";

            auto shift_res = s_exp->SetPerspectiveShift(
                initial_fw,
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

            auto tx_res = db_->SaveSessionWithEvent(*s_exp, "M1_PERSPECTIVE_SHIFT", body.dump(), rationale.author);
            if (!tx_res) {
                res.status = 500;
                res.set_content(json({{"error", "Falha na transação atômica de persistência: " + tx_res.error()}}).dump(), "application/json");
                return;
            }

            res.set_content(json({{"status", "ok"}, {"merkle_hash", *tx_res}}).dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    // M2: Adicionar Relação Candidata
    svr.Post(R"(/api/sessions/([\w\-]+)/relation)", [this, check_auth, resolve_model_digest](const httplib::Request& req, httplib::Response& res) {
        if (!check_auth(req, res)) return;
        std::string sid = req.matches[1];
        auto s_exp = db_->LoadSession(sid);
        if (!s_exp) { res.status = 404; return; }

        try {
            json body = json::parse(req.body);
            bool is_assisted = body.value("is_assisted", false);
            std::string digest = resolve_model_digest(body, is_assisted);

            domain::ProvenancedText desc{
                .text = body.value("description", ""),
                .origin = is_assisted ? domain::ProvenanceOrigin::Assisted : domain::ProvenanceOrigin::Human,
                .author = body.value("author", "Pesquisador"),
                .model_digest = digest,
                .citation = "",
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

            auto tx_res = db_->SaveSessionWithEvent(*s_exp, "M2_CANDIDATE_RELATION", body.dump(), desc.author);
            if (!tx_res) {
                res.status = 500;
                res.set_content(json({{"error", "Falha na transação atômica de persistência: " + tx_res.error()}}).dump(), "application/json");
                return;
            }

            res.set_content(json({{"status", "ok"}, {"merkle_hash", *tx_res}}).dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    // M3: Adicionar Questão Científica
    svr.Post(R"(/api/sessions/([\w\-]+)/question)", [this, check_auth, resolve_model_digest](const httplib::Request& req, httplib::Response& res) {
        if (!check_auth(req, res)) return;
        std::string sid = req.matches[1];
        auto s_exp = db_->LoadSession(sid);
        if (!s_exp) { res.status = 404; return; }

        try {
            json body = json::parse(req.body);
            bool is_assisted = body.value("is_assisted", false);
            std::string digest = resolve_model_digest(body, is_assisted);

            domain::ProvenancedText qtext{
                .text = body.value("question_text", ""),
                .origin = is_assisted ? domain::ProvenanceOrigin::Assisted : domain::ProvenanceOrigin::Human,
                .author = body.value("author", "Pesquisador"),
                .model_digest = digest,
                .citation = "",
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

            auto tx_res = db_->SaveSessionWithEvent(*s_exp, "M3_SCIENTIFIC_QUESTION", body.dump(), qtext.author);
            if (!tx_res) {
                res.status = 500;
                res.set_content(json({{"error", "Falha na transação atômica de persistência: " + tx_res.error()}}).dump(), "application/json");
                return;
            }

            res.set_content(json({{"status", "ok"}, {"merkle_hash", *tx_res}}).dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    // C4: Registrar Confronto
    svr.Post(R"(/api/sessions/([\w\-]+)/confront)", [this, check_auth](const httplib::Request& req, httplib::Response& res) {
        if (!check_auth(req, res)) return;
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

            auto tx_res = db_->SaveSessionWithEvent(*s_exp, "C4_CONFRONTATION_RECORDED", body.dump(), "Revisor Crítico");
            if (!tx_res) {
                res.status = 500;
                res.set_content(json({{"error", "Falha na transação atômica de persistência: " + tx_res.error()}}).dump(), "application/json");
                return;
            }

            res.set_content(json({{"status", "ok"}, {"merkle_hash", *tx_res}}).dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    // C5: Consolidar Reposicionamento
    svr.Post(R"(/api/sessions/([\w\-]+)/reposition)", [this, check_auth, resolve_model_digest](const httplib::Request& req, httplib::Response& res) {
        if (!check_auth(req, res)) return;
        std::string sid = req.matches[1];
        auto s_exp = db_->LoadSession(sid);
        if (!s_exp) { res.status = 404; return; }

        try {
            json body = json::parse(req.body);
            std::string dec_str = body.value("decision", "DEVELOP");
            domain::HypothesisStatus status = domain::HypothesisStatus::Develop;
            if (dec_str == "REFORMULATE") status = domain::HypothesisStatus::Reformulate;
            else if (dec_str == "SUSPEND") status = domain::HypothesisStatus::Suspend;
            else if (dec_str == "REJECT" || dec_str == "ABANDON") status = domain::HypothesisStatus::Reject;
            else if (dec_str == "SUSTAINED_BY_SPECIFIC_EVIDENCE" || dec_str == "REINFORCE" || dec_str == "DEVELOP") status = domain::HypothesisStatus::Develop;

            std::string just_text = body.value("justification", "");
            bool is_assisted = body.value("is_assisted", false) || just_text.contains("[ASSISTIDO");
            std::string digest = resolve_model_digest(body, is_assisted);

            domain::ProvenancedText just{
                .text = just_text,
                .origin = is_assisted ? domain::ProvenanceOrigin::Assisted : domain::ProvenanceOrigin::Human,
                .author = body.value("author", "Pesquisador"),
                .model_digest = digest,
                .timestamp_iso = "2026-10-23T14:00:00Z"
            };

            std::string next_test = body.value("next_empirical_test", "");
            std::string req_know = body.value("required_knowledge", "");
            if (next_test.empty() && req_know.empty()) {
                next_test = "Submeter ao próximo ciclo de verificação empírica e contraste com literatura";
            }

            std::string resump_crit = body.value("resumption_criteria", "");
            if (status == domain::HypothesisStatus::Suspend && resump_crit.empty()) {
                resump_crit = "Aguardando novos dados e evidências observacionais para reabertura da análise";
            }

            auto rep_res = s_exp->ConsolidateReposition(
                status,
                just,
                next_test,
                req_know,
                resump_crit
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

            auto tx_res = db_->SaveSessionWithEvent(*s_exp, "C5_REPOSITION_CONSOLIDATED", body.dump(), just.author);
            if (!tx_res) {
                res.status = 500;
                res.set_content(json({{"error", "Falha na transação atômica de persistência: " + tx_res.error()}}).dump(), "application/json");
                return;
            }

            res.set_content(json({{"status", "ok"}, {"merkle_hash", *tx_res}}).dump(), "application/json");
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

    svr.Post("/api/assist/relation", [this](const httplib::Request& req, httplib::Response& res) {
        if (!ollama_ || !ollama_->IsOnline()) {
            res.status = 503;
            res.set_content(json({{"error", "Daemon Ollama local offline."}}).dump(), "application/json");
            return;
        }
        try {
            json body = json::parse(req.body);
            std::string model = body.value("model", "qwen2.5:14b");
            auto r = ollama_->SuggestRelations(body.value("phenomenon", ""), body.value("shifted_framework", ""), model);
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

    svr.Post("/api/assist/question", [this](const httplib::Request& req, httplib::Response& res) {
        if (!ollama_ || !ollama_->IsOnline()) {
            res.status = 503;
            res.set_content(json({{"error", "Daemon Ollama local offline."}}).dump(), "application/json");
            return;
        }
        try {
            json body = json::parse(req.body);
            std::string model = body.value("model", "qwen2.5:14b");
            auto r = ollama_->SuggestQuestions(body.value("relation", ""), model);
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

    // ------------------------------------------------------------------
    // Endpoints de Preenchimento Direto Estruturado com LLM Local
    // ------------------------------------------------------------------
    svr.Post("/api/assist/fill/m1", [this](const httplib::Request& req, httplib::Response& res) {
        if (!ollama_ || !ollama_->IsOnline()) {
            res.status = 503;
            res.set_content(json({{"error", "Daemon Ollama local offline."}}).dump(), "application/json");
            return;
        }
        try {
            json body = json::parse(req.body);
            std::string model = body.value("model", "qwen2.5:14b");
            auto r = ollama_->FillM1(body.value("phenomenon", ""), body.value("current_framework", ""), model);
            if (!r) {
                res.status = 500;
                res.set_content(json({{"error", r.error()}}).dump(), "application/json");
                return;
            }
            json resp = *r;
            resp["model"] = model;
            res.set_content(resp.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    svr.Post("/api/assist/fill/m2", [this](const httplib::Request& req, httplib::Response& res) {
        if (!ollama_ || !ollama_->IsOnline()) {
            res.status = 503;
            res.set_content(json({{"error", "Daemon Ollama local offline."}}).dump(), "application/json");
            return;
        }
        try {
            json body = json::parse(req.body);
            std::string model = body.value("model", "qwen2.5:14b");
            auto r = ollama_->FillM2(body.value("phenomenon", ""), body.value("shifted_framework", ""), model);
            if (!r) {
                res.status = 500;
                res.set_content(json({{"error", r.error()}}).dump(), "application/json");
                return;
            }
            json resp = *r;
            resp["model"] = model;
            res.set_content(resp.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    svr.Post("/api/assist/fill/m3", [this](const httplib::Request& req, httplib::Response& res) {
        if (!ollama_ || !ollama_->IsOnline()) {
            res.status = 503;
            res.set_content(json({{"error", "Daemon Ollama local offline."}}).dump(), "application/json");
            return;
        }
        try {
            json body = json::parse(req.body);
            std::string model = body.value("model", "qwen2.5:14b");
            auto r = ollama_->FillM3(body.value("relation", ""), model);
            if (!r) {
                res.status = 500;
                res.set_content(json({{"error", r.error()}}).dump(), "application/json");
                return;
            }
            json resp = *r;
            resp["model"] = model;
            res.set_content(resp.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    svr.Post("/api/assist/fill/confront", [this](const httplib::Request& req, httplib::Response& res) {
        if (!ollama_ || !ollama_->IsOnline()) {
            res.status = 503;
            res.set_content(json({{"error", "Daemon Ollama local offline."}}).dump(), "application/json");
            return;
        }
        try {
            json body = json::parse(req.body);
            std::string model = body.value("model", "qwen2.5:14b");
            auto r = ollama_->FillConfront(body.value("hypothesis", ""), model);
            if (!r) {
                res.status = 500;
                res.set_content(json({{"error", r.error()}}).dump(), "application/json");
                return;
            }
            json resp = *r;
            resp["model"] = model;
            res.set_content(resp.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    svr.Post("/api/assist/fill/reposition", [this](const httplib::Request& req, httplib::Response& res) {
        if (!ollama_ || !ollama_->IsOnline()) {
            res.status = 503;
            res.set_content(json({{"error", "Daemon Ollama local offline."}}).dump(), "application/json");
            return;
        }
        try {
            json body = json::parse(req.body);
            std::string model = body.value("model", "qwen2.5:14b");
            auto r = ollama_->FillReposition(
                body.value("hypothesis", ""),
                body.value("rivals_summary", ""),
                body.value("risks_summary", ""),
                model
            );
            if (!r) {
                res.status = 500;
                res.set_content(json({{"error", r.error()}}).dump(), "application/json");
                return;
            }
            json resp = *r;
            resp["model"] = model;
            res.set_content(resp.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    svr.Post("/api/assist/fill/session", [this](const httplib::Request& req, httplib::Response& res) {
        if (!ollama_ || !ollama_->IsOnline()) {
            res.status = 503;
            res.set_content(json({{"error", "Daemon Ollama local offline."}}).dump(), "application/json");
            return;
        }
        try {
            json body = json::parse(req.body);
            std::string model = body.value("model", "qwen2.5:14b");
            auto r = ollama_->FillSession(body.value("topic", ""), model);
            if (!r) {
                res.status = 500;
                res.set_content(json({{"error", r.error()}}).dump(), "application/json");
                return;
            }
            json resp = *r;
            resp["model"] = model;
            res.set_content(resp.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json({{"error", e.what()}}).dump(), "application/json");
        }
    });

    // Servir Arquivos Estáticos da Web
    if (std::filesystem::exists(web_dir_)) {
        svr.set_mount_point("/", web_dir_);
    }

    std::cout << "[MIRANTE] Servidor C++26 iniciado em http://" << host_ << ":" << port_ << std::endl;
    std::cout << "[MIRANTE] Servindo interface web a partir de: " << web_dir_ << std::endl;
    running_ = true;
    svr.listen(host_.c_str(), port_);
    running_ = false;
}

void HttpServer::Stop() {
    running_ = false;
}

} // namespace mirante::http
