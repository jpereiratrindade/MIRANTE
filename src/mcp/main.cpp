#include "mirante/domain/entities.hpp"
#include "mirante/domain/invariants.hpp"
#include "mirante/storage/database.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <format>
#include <memory>
#include <filesystem>

using json = nlohmann::json;
using namespace mirante::domain;
using namespace mirante::storage;

namespace {

std::string GetDefaultDbPath() {
    const char* env_path = std::getenv("MIRANTE_DB_PATH");
    if (env_path && std::strlen(env_path) > 0) return env_path;
    if (std::filesystem::exists("mirante.sqlite3")) return "mirante.sqlite3";
    return "/run/media/jpereiratrindade/labeco10T/dev/cpp/MIRANTE/mirante.sqlite3";
}

json MakeToolSuccess(const std::string& text) {
    return {
        {"content", json::array({
            {{"type", "text"}, {"text", text}}
        })},
        {"isError", false}
    };
}

json MakeToolError(const std::string& error_msg) {
    return {
        {"content", json::array({
            {{"type", "text"}, {"text", "ERRO DE INVARIANTE / EXECUÇÃO: " + error_msg}}
        })},
        {"isError", true}
    };
}

json GetToolDefinitions() {
    return json::array({
        {
            {"name", "list_sessions"},
            {"description", "Lista todas as sessões de investigação científica registradas no MIRANTE."},
            {"inputSchema", {
                {"type", "object"},
                {"properties", json::object()}
            }}
        },
        {
            {"name", "get_session"},
            {"description", "Recupera o estado completo de uma sessão MPRR: fenômeno, deslocamento (M1), relações candidatas (M2), questões (M3), confronto crítico (C4) e reposicionamento (C5)."},
            {"inputSchema", {
                {"type", "object"},
                {"required", {"session_id"}},
                {"properties", {
                    {"session_id", {{"type", "string"}, {"description", "ID da sessão (ex: SES-2026-10-RES01)"}}}
                }}
            }}
        },
        {
            {"name", "create_session"},
            {"description", "Inicia uma nova sessão de percepção relacional segundo o protocolo MPRR v0.1.0."},
            {"inputSchema", {
                {"type", "object"},
                {"required", {"project_id", "title", "phenomenon_text"}},
                {"properties", {
                    {"session_id", {{"type", "string"}, {"description", "ID opcional da sessão (gerado automaticamente se omitido)"}}},
                    {"project_id", {{"type", "string"}, {"description", "Código ou nome do projeto de pesquisa"}}},
                    {"title", {{"type", "string"}, {"description", "Título da investigação"}}},
                    {"entrance_door", {
                        {"type", "string"},
                        {"enum", {"PATH_A_KNOWLEDGE_TO_PHENOMENON", "PATH_B_PHENOMENON_TO_KNOWLEDGE"}},
                        {"description", "Porta de entrada metodológica: Caminho A ou Caminho B (default: PATH_B)"}
                    }},
                    {"phenomenon_text", {{"type", "string"}, {"description", "Descrição factual densa do fenômeno empírico de interesse"}}},
                    {"spatial_boundary", {{"type", "string"}, {"description", "Delimitação espacial"}}},
                    {"temporal_boundary", {{"type", "string"}, {"description", "Delimitação temporal"}}},
                    {"research_context", {{"type", "string"}, {"description", "Contexto de investigação"}}},
                    {"author", {{"type", "string"}, {"description", "Autor responsável pelo registro"}}}
                }}
            }}
        },
        {
            {"name", "shift_perspective"},
            {"description", "Movimento 1: Registra formalmente um deslocamento de perspectiva (enquadramento inicial, enquadramento deslocado, dimensão e justificativa) sob o invariante INV-07."},
            {"inputSchema", {
                {"type", "object"},
                {"required", {"session_id", "initial_framework", "shifted_framework", "rationale"}},
                {"properties", {
                    {"session_id", {{"type", "string"}}},
                    {"initial_framework", {{"type", "string"}, {"description", "Enquadramento epistêmico ou escala inicial"}}},
                    {"shifted_framework", {{"type", "string"}, {"description", "Perspectiva conceitual ou escala alternativa deslocada"}}},
                    {"dimension", {
                        {"type", "string"},
                        {"enum", {"SPATIAL_SCALE", "TIME_HORIZON", "UNIT_OF_ANALYSIS", "OBSERVER_ROLE", "ORGANIZATION_LEVEL", "PRIVILEGED_VARIABLE", "DISCIPLINE", "SOCIAL_ACTOR", "CUSTOM"}},
                        {"description", "Dimensão do deslocamento"}
                    }},
                    {"rationale", {{"type", "string"}, {"description", "Justificativa metodológica da mudança de enquadramento"}}},
                    {"author", {{"type", "string"}}},
                    {"is_assisted", {{"type", "boolean"}, {"description", "True se formulado com assistência de IA"}}},
                    {"model_digest", {{"type", "string"}, {"description", "Hash SHA-256 do modelo se assistido (INV-04)"}}}
                }}
            }}
        },
        {
            {"name", "add_candidate_relation"},
            {"description", "Movimento 2: Formula uma relação candidata na bancada do MPRR. Toda relação nasce e permanece compulsoriamente como CANDIDATA (Invariante INV-01)."},
            {"inputSchema", {
                {"type", "object"},
                {"required", {"session_id", "term_a", "term_b", "description"}},
                {"properties", {
                    {"session_id", {{"type", "string"}}},
                    {"term_a", {{"type", "string"}, {"description", "Primeiro polo/termo da relação"}}},
                    {"term_b", {{"type", "string"}, {"description", "Segundo polo/termo da relação"}}},
                    {"relation_type", {
                        {"type", "string"},
                        {"enum", {"ASSOCIATION", "MEDIATION", "CONTEXT_DEPENDENCY", "FEEDBACK", "TEMPORAL_SEQUENCE", "POSSIBLE_NECESSARY_CONDITION", "CONJECTURED_MECHANISM", "HEURISTIC_ANALOGY"}}
                    }},
                    {"description", {{"type", "string"}, {"description", "Articulação detalhada da conjectura relacional"}}},
                    {"author", {{"type", "string"}}},
                    {"is_assisted", {{"type", "boolean"}}},
                    {"model_digest", {{"type", "string"}}}
                }}
            }}
        },
        {
            {"name", "formulate_question"},
            {"description", "Movimento 3: Formula questão científica investigável atrelada a uma relação da bancada, especificando a via de evidência necessária para teste empírico."},
            {"inputSchema", {
                {"type", "object"},
                {"required", {"session_id", "question_text"}},
                {"properties", {
                    {"session_id", {{"type", "string"}}},
                    {"relation_id", {{"type", "string"}, {"description", "ID da relação candidata associada (opcional, associa à primeira se omitido)"}}},
                    {"question_text", {{"type", "string"}, {"description", "A pergunta científica investigável"}}},
                    {"evidence_path", {{"type", "string"}, {"description", "Via ou desenho de evidência empírica para responder a questão"}}},
                    {"author", {{"type", "string"}}},
                    {"is_assisted", {{"type", "boolean"}}},
                    {"model_digest", {{"type", "string"}}}
                }}
            }}
        },
        {
            {"name", "record_confrontation"},
            {"description", "Controle Crítico C4: Submete a hipótese formulada ao contraditório obrigatório. Exige no mínimo 1 explicação rival e 1 risco epistêmico de invalidação (Invariante INV-02)."},
            {"inputSchema", {
                {"type", "object"},
                {"required", {"session_id", "rival_explanations", "epistemic_risks"}},
                {"properties", {
                    {"session_id", {{"type", "string"}}},
                    {"rival_explanations", {
                        {"type", "array"},
                        {"description", "Lista de explicações alternativas plausíveis que concorrem com a hipótese"},
                        {"items", {{"type", "string"}}}
                    }},
                    {"epistemic_risks", {
                        {"type", "array"},
                        {"description", "Lista de riscos epistêmicos (vieses de confirmação, artefatos de medição, variáveis omitidas)"},
                        {"items", {{"type", "string"}}}
                    }},
                    {"author", {{"type", "string"}, {"description", "Identificação do revisor crítico / advogado do diabo"}}}
                }}
            }}
        },
        {
            {"name", "consolidate_reposition"},
            {"description", "Controle Crítico C5: Consolida a decisão de reposicionamento da hipótese (DEVELOP, REFORMULATE, SUSPEND, REJECT, SUSTAINED_BY_SPECIFIC_EVIDENCE) com próximo teste empírico (INV-08). Se suspender, exige condições de retomada (INV-06)."},
            {"inputSchema", {
                {"type", "object"},
                {"required", {"session_id", "decision", "justification"}},
                {"properties", {
                    {"session_id", {{"type", "string"}}},
                    {"decision", {
                        {"type", "string"},
                        {"enum", {"DEVELOP", "REFORMULATE", "SUSPEND", "REJECT", "SUSTAINED_BY_SPECIFIC_EVIDENCE"}},
                        {"description", "Decisão epistemológica de reposicionamento"}
                    }},
                    {"justification", {{"type", "string"}, {"description", "Justificativa metodológica da decisão frente ao confronto"}}},
                    {"next_empirical_test", {{"type", "string"}, {"description", "Próximo teste empírico obrigatório para continuidade da investigação (INV-08)"}}},
                    {"required_knowledge", {{"type", "string"}, {"description", "Conhecimento teórico adicional necessário"}}},
                    {"resumption_criteria", {{"type", "string"}, {"description", "Critérios formais para desengavetar a investigação (obrigatório se decision for SUSPEND - INV-06)"}}},
                    {"author", {{"type", "string"}}}
                }}
            }}
        },
        {
            {"name", "verify_audit_ledger"},
            {"description", "Auditoria de Integridade Criptográfica (INV-03): Verifica matematicamente todos os hashes encadeados SHA-256 do ledger de eventos da sessão, acusando qualquer tentativa de adulteração de histórico."},
            {"inputSchema", {
                {"type", "object"},
                {"required", {"session_id"}},
                {"properties", {
                    {"session_id", {{"type", "string"}}}
                }}
            }}
        }
    });
}

json ExecuteTool(Database& db, const std::string& name, const json& args) {
    if (name == "list_sessions") {
        auto list_exp = db.ListSessions();
        if (!list_exp) return MakeToolError(list_exp.error());
        json j = json::array();
        for (const auto& s : *list_exp) {
            j.push_back({
                {"session_id", s.session_id},
                {"project_id", s.project_id},
                {"title", s.title},
                {"cycle", s.current_cycle}
            });
        }
        return MakeToolSuccess(j.dump(2));
    }

    if (name == "get_session") {
        std::string sid = args.value("session_id", "");
        auto s_exp = db.LoadSession(sid);
        if (!s_exp) return MakeToolError("Sessão não encontrada: " + s_exp.error());

        json j = {
            {"session_id", s_exp->session_id},
            {"project_id", s_exp->project_id},
            {"title", s_exp->title},
            {"entrance_door", ToString(s_exp->entrance_door)},
            {"current_cycle", s_exp->current_cycle},
            {"phenomenon", s_exp->phenomenon_description.text},
            {"phenomenon_author", s_exp->phenomenon_description.author},
            {"spatial_boundary", s_exp->spatial_boundary},
            {"temporal_boundary", s_exp->temporal_boundary},
            {"research_context", s_exp->research_context}
        };

        if (s_exp->shift.has_value()) {
            j["shift"] = {
                {"initial", s_exp->shift->initial_framework},
                {"shifted", s_exp->shift->shifted_framework},
                {"dimension", ToString(s_exp->shift->dimension)},
                {"rationale", s_exp->shift->rationale.text}
            };
        }

        json relations_j = json::array();
        for (const auto& r : s_exp->relations) {
            relations_j.push_back({
                {"id", r.id},
                {"term_a", r.term_a},
                {"term_b", r.term_b},
                {"type", ToString(r.type)},
                {"status", ToString(r.status)},
                {"description", r.description.text}
            });
        }
        j["relations"] = relations_j;

        json questions_j = json::array();
        for (const auto& q : s_exp->questions) {
            questions_j.push_back({
                {"id", q.id},
                {"relation_id", q.relation_id},
                {"question", q.question_text.text},
                {"evidence_path", q.required_evidence_path}
            });
        }
        j["questions"] = questions_j;

        if (s_exp->confrontation.has_value()) {
            json rivals = json::array();
            for (const auto& r : s_exp->confrontation->rival_explanations) rivals.push_back(r.text);
            json risks = json::array();
            for (const auto& r : s_exp->confrontation->epistemic_risks) risks.push_back(r.text);
            j["confrontation"] = {
                {"rival_explanations", rivals},
                {"epistemic_risks", risks},
                {"is_concluded", s_exp->confrontation->is_concluded}
            };
        }

        return MakeToolSuccess(j.dump(2));
    }

    if (name == "create_session") {
        SessionAggregate s;
        s.session_id = args.value("session_id", std::format("SES-{}", std::chrono::system_clock::now().time_since_epoch().count() % 1000000));
        s.project_id = args.value("project_id", "PRJ-DEFAULT");
        s.title = args.value("title", "Sessão MPRR");

        std::string door_str = args.value("entrance_door", "PATH_B_PHENOMENON_TO_KNOWLEDGE");
        s.entrance_door = (door_str == "PATH_A_KNOWLEDGE_TO_PHENOMENON")
            ? EntranceDoor::PathA_KnowledgeToPhenomenon
            : EntranceDoor::PathB_PhenomenonToKnowledge;

        s.spatial_boundary = args.value("spatial_boundary", "");
        s.temporal_boundary = args.value("temporal_boundary", "");
        s.research_context = args.value("research_context", "");

        s.phenomenon_description.text = args.value("phenomenon_text", "");
        s.phenomenon_description.author = args.value("author", "Pesquisador (MCP)");
        s.phenomenon_description.origin = ProvenanceOrigin::Human;

        auto save_res = db.SaveSessionWithEvent(s, "SESSION_CREATED_VIA_MCP", args.dump(), s.phenomenon_description.author);
        if (!save_res) return MakeToolError("Falha na persistência atômica da sessão: " + save_res.error());

        json out = {
            {"status", "created"},
            {"session_id", s.session_id},
            {"merkle_hash", *save_res}
        };
        return MakeToolSuccess(out.dump(2));
    }

    if (name == "shift_perspective") {
        std::string sid = args.value("session_id", "");
        auto s_exp = db.LoadSession(sid);
        if (!s_exp) return MakeToolError("Sessão não encontrada");

        ProvenancedText rationale{
            .text = args.value("rationale", ""),
            .origin = args.value("is_assisted", false) ? ProvenanceOrigin::Assisted : ProvenanceOrigin::Human,
            .author = args.value("author", "Agente MCP"),
            .model_digest = args.value("model_digest", "")
        };

        auto shift_res = s_exp->SetPerspectiveShift(
            args.value("initial_framework", ""),
            args.value("shifted_framework", ""),
            ShiftDimension::TimeHorizon,
            rationale
        );

        if (!shift_res) {
            return MakeToolError(std::format("[{}] {} (Sugestão: {})", 
                shift_res.error().invariant_id, shift_res.error().message, shift_res.error().corrective_hint));
        }

        auto tx_res = db.SaveSessionWithEvent(*s_exp, "M1_PERSPECTIVE_SHIFT_MCP", args.dump(), rationale.author);
        if (!tx_res) return MakeToolError("Falha ao salvar transação: " + tx_res.error());

        return MakeToolSuccess(json({{"status", "ok"}, {"merkle_hash", *tx_res}}).dump(2));
    }

    if (name == "add_candidate_relation") {
        std::string sid = args.value("session_id", "");
        auto s_exp = db.LoadSession(sid);
        if (!s_exp) return MakeToolError("Sessão não encontrada");

        ProvenancedText desc{
            .text = args.value("description", ""),
            .origin = args.value("is_assisted", false) ? ProvenanceOrigin::Assisted : ProvenanceOrigin::Human,
            .author = args.value("author", "Agente MCP"),
            .model_digest = args.value("model_digest", "")
        };

        auto rel_res = s_exp->AddCandidateRelation(
            args.value("term_a", ""),
            args.value("term_b", ""),
            RelationType::ConjecturedMechanism,
            desc
        );

        if (!rel_res) {
            return MakeToolError(std::format("[{}] {} (Sugestão: {})",
                rel_res.error().invariant_id, rel_res.error().message, rel_res.error().corrective_hint));
        }

        auto tx_res = db.SaveSessionWithEvent(*s_exp, "M2_CANDIDATE_RELATION_MCP", args.dump(), desc.author);
        if (!tx_res) return MakeToolError("Falha ao salvar transação: " + tx_res.error());

        return MakeToolSuccess(json({{"status", "ok"}, {"relation_id", s_exp->relations.back().id}, {"merkle_hash", *tx_res}}).dump(2));
    }

    if (name == "formulate_question") {
        std::string sid = args.value("session_id", "");
        auto s_exp = db.LoadSession(sid);
        if (!s_exp) return MakeToolError("Sessão não encontrada");

        std::string rel_id = args.value("relation_id", "");
        if (rel_id.empty() && !s_exp->relations.empty()) {
            rel_id = s_exp->relations.back().id;
        }

        ProvenancedText qtext{
            .text = args.value("question_text", ""),
            .origin = args.value("is_assisted", false) ? ProvenanceOrigin::Assisted : ProvenanceOrigin::Human,
            .author = args.value("author", "Agente MCP"),
            .model_digest = args.value("model_digest", "")
        };

        auto q_res = s_exp->AddScientificQuestion(rel_id, qtext, args.value("evidence_path", ""));
        if (!q_res) {
            return MakeToolError(std::format("[{}] {} (Sugestão: {})",
                q_res.error().invariant_id, q_res.error().message, q_res.error().corrective_hint));
        }

        auto tx_res = db.SaveSessionWithEvent(*s_exp, "M3_SCIENTIFIC_QUESTION_MCP", args.dump(), qtext.author);
        if (!tx_res) return MakeToolError("Falha ao salvar transação: " + tx_res.error());

        return MakeToolSuccess(json({{"status", "ok"}, {"question_id", s_exp->questions.back().id}, {"merkle_hash", *tx_res}}).dump(2));
    }

    if (name == "record_confrontation") {
        std::string sid = args.value("session_id", "");
        auto s_exp = db.LoadSession(sid);
        if (!s_exp) return MakeToolError("Sessão não encontrada");

        std::string author = args.value("author", "Revisor Crítico (MCP)");
        std::vector<ProvenancedText> rivals;
        if (args.contains("rival_explanations")) {
            for (const auto& item : args["rival_explanations"]) {
                rivals.push_back({
                    .text = item.is_string() ? item.get<std::string>() : item.value("text", ""),
                    .origin = ProvenanceOrigin::Human,
                    .author = author
                });
            }
        }

        std::vector<ProvenancedText> risks;
        if (args.contains("epistemic_risks")) {
            for (const auto& item : args["epistemic_risks"]) {
                risks.push_back({
                    .text = item.is_string() ? item.get<std::string>() : item.value("text", ""),
                    .origin = ProvenanceOrigin::Human,
                    .author = author
                });
            }
        }

        auto conf_res = s_exp->RecordConfrontation(rivals, risks, {});
        if (!conf_res) {
            return MakeToolError(std::format("[{}] {} (Sugestão: {})",
                conf_res.error().invariant_id, conf_res.error().message, conf_res.error().corrective_hint));
        }

        auto tx_res = db.SaveSessionWithEvent(*s_exp, "C4_CONFRONTATION_RECORDED_MCP", args.dump(), author);
        if (!tx_res) return MakeToolError("Falha ao salvar transação: " + tx_res.error());

        return MakeToolSuccess(json({{"status", "ok"}, {"merkle_hash", *tx_res}}).dump(2));
    }

    if (name == "consolidate_reposition") {
        std::string sid = args.value("session_id", "");
        auto s_exp = db.LoadSession(sid);
        if (!s_exp) return MakeToolError("Sessão não encontrada");

        std::string dec_str = args.value("decision", "DEVELOP");
        HypothesisStatus status = HypothesisStatus::Develop;
        if (dec_str == "REFORMULATE") status = HypothesisStatus::Reformulate;
        else if (dec_str == "SUSPEND") status = HypothesisStatus::Suspend;
        else if (dec_str == "REJECT") status = HypothesisStatus::Reject;
        else if (dec_str == "SUSTAINED_BY_SPECIFIC_EVIDENCE") status = HypothesisStatus::SustainedBySpecificEvidence;

        std::string author = args.value("author", "Pesquisador (MCP)");
        ProvenancedText just{
            .text = args.value("justification", ""),
            .origin = ProvenanceOrigin::Human,
            .author = author
        };

        auto rep_res = s_exp->ConsolidateReposition(
            status,
            just,
            args.value("next_empirical_test", ""),
            args.value("required_knowledge", ""),
            args.value("resumption_criteria", "")
        );

        if (!rep_res) {
            return MakeToolError(std::format("[{}] {} (Sugestão: {})",
                rep_res.error().invariant_id, rep_res.error().message, rep_res.error().corrective_hint));
        }

        auto tx_res = db.SaveSessionWithEvent(*s_exp, "C5_REPOSITION_CONSOLIDATED_MCP", args.dump(), author);
        if (!tx_res) return MakeToolError("Falha ao salvar transação: " + tx_res.error());

        return MakeToolSuccess(json({{"status", "ok"}, {"new_cycle", s_exp->current_cycle}, {"merkle_hash", *tx_res}}).dump(2));
    }

    if (name == "verify_audit_ledger") {
        std::string sid = args.value("session_id", "");
        auto verify_exp = db.VerifyLedgerIntegrity(sid);
        if (!verify_exp) return MakeToolError("Erro ao verificar integridade: " + verify_exp.error());

        auto ledger_exp = db.GetEventLedger(sid);
        size_t count = ledger_exp ? ledger_exp->size() : 0;
        std::string head = (ledger_exp && !ledger_exp->empty()) ? ledger_exp->back().current_hash : "";

        json out = {
            {"session_id", sid},
            {"is_cryptographically_valid", *verify_exp},
            {"events_count", count},
            {"head_merkle_hash", head}
        };
        return MakeToolSuccess(out.dump(2));
    }

    return MakeToolError("Ferramenta desconhecida: " + name);
}

} // namespace

int main(int argc, char** argv) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string db_path = GetDefaultDbPath();
    if (argc > 1) {
        db_path = argv[1];
    }

    Database db;
    auto op = db.Open(db_path);
    if (!op) {
        std::cerr << "[MIRANTE-MCP] Erro fatal ao abrir banco SQLite: " << op.error() << std::endl;
        return 1;
    }

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line == "\r") continue;

        try {
            json req = json::parse(line);
            json req_id = req.contains("id") ? req["id"] : json(nullptr);
            std::string method = req.value("method", "");

            if (method == "initialize") {
                json resp = {
                    {"jsonrpc", "2.0"},
                    {"id", req_id},
                    {"result", {
                        {"protocolVersion", "2024-11-05"},
                        {"capabilities", {
                            {"tools", json::object()},
                            {"resources", json::object()},
                            {"prompts", json::object()}
                        }},
                        {"serverInfo", {
                            {"name", "mirante-mcp"},
                            {"version", "0.1.0"},
                            {"description", "Servidor MCP nativo do MIRANTE — Experiência Assistida de Percepção Relacional"}
                        }}
                    }}
                };
                std::cout << resp.dump() << "\n" << std::flush;
                continue;
            }

            if (method == "notifications/initialized") {
                // Notificação não requer resposta
                continue;
            }

            if (method == "tools/list") {
                json resp = {
                    {"jsonrpc", "2.0"},
                    {"id", req_id},
                    {"result", {
                        {"tools", GetToolDefinitions()}
                    }}
                };
                std::cout << resp.dump() << "\n" << std::flush;
                continue;
            }

            if (method == "tools/call") {
                json params = req.value("params", json::object());
                std::string tool_name = params.value("name", "");
                json tool_args = params.value("arguments", json::object());

                json res = ExecuteTool(db, tool_name, tool_args);
                json resp = {
                    {"jsonrpc", "2.0"},
                    {"id", req_id},
                    {"result", res}
                };
                std::cout << resp.dump() << "\n" << std::flush;
                continue;
            }

            if (method == "resources/list") {
                json resp = {
                    {"jsonrpc", "2.0"},
                    {"id", req_id},
                    {"result", {
                        {"resources", json::array({
                            {
                                {"uri", "mirante://sessions"},
                                {"name", "Sessões Ativas do MIRANTE"},
                                {"mimeType", "application/json"}
                            },
                            {
                                {"uri", "mirante://protocol/invariants"},
                                {"name", "Invariantes Normativos do MPRR"},
                                {"mimeType", "application/json"}
                            }
                        })}
                    }}
                };
                std::cout << resp.dump() << "\n" << std::flush;
                continue;
            }

            if (method == "resources/read") {
                std::string uri = req["params"].value("uri", "");
                json content_item;
                if (uri == "mirante://protocol/invariants") {
                    content_item = {
                        {"uri", uri},
                        {"mimeType", "application/json"},
                        {"text", "INV-01 (Proibição de Fato Demonstrado Prematuro)\nINV-02 (Contraditório Obrigatório)\nINV-03 (Merkle Ledger)\nINV-04 (Proveniência Transparente)\nINV-05 (Autonomia do Núcleo)\nINV-06 (Conservação de Incerteza)\nINV-07 (Rastreabilidade de Deslocamento)\nINV-08 (Próximo Passo Epistêmico)"}
                    };
                } else {
                    auto list_exp = db.ListSessions();
                    json j = json::array();
                    if (list_exp) {
                        for (const auto& s : *list_exp) j.push_back({{"id", s.session_id}, {"title", s.title}});
                    }
                    content_item = {
                        {"uri", uri},
                        {"mimeType", "application/json"},
                        {"text", j.dump(2)}
                    };
                }

                json resp = {
                    {"jsonrpc", "2.0"},
                    {"id", req_id},
                    {"result", {
                        {"contents", json::array({content_item})}
                    }}
                };
                std::cout << resp.dump() << "\n" << std::flush;
                continue;
            }

            if (method == "prompts/list") {
                json resp = {
                    {"jsonrpc", "2.0"},
                    {"id", req_id},
                    {"result", {
                        {"prompts", json::array({
                            {
                                {"name", "confront_hypothesis"},
                                {"description", "Assume a persona de Revisor Crítico / Advogado do Diabo sob o protocolo MPRR para buscar falhas e hipóteses rivais."},
                                {"arguments", json::array({{{"name", "hypothesis"}, {"required", true}}})}
                            },
                            {
                                {"name", "divergent_shift"},
                                {"description", "Propõe deslocamentos conceituais não-óbvios de escala, horizonte temporal e variáveis privilegiadas."},
                                {"arguments", json::array({{{"name", "phenomenon"}, {"required", true}}})}
                            }
                        })}
                    }}
                };
                std::cout << resp.dump() << "\n" << std::flush;
                continue;
            }

            // Método desconhecido
            json err_resp = {
                {"jsonrpc", "2.0"},
                {"id", req_id},
                {"error", {
                    {"code", -32601},
                    {"message", "Método não encontrado: " + method}
                }}
            };
            std::cout << err_resp.dump() << "\n" << std::flush;
        } catch (const std::exception& e) {
            json err_resp = {
                {"jsonrpc", "2.0"},
                {"id", nullptr},
                {"error", {
                    {"code", -32700},
                    {"message", std::string("JSON parse error: ") + e.what()}
                }}
            };
            std::cout << err_resp.dump() << "\n" << std::flush;
        }
    }

    return 0;
}
