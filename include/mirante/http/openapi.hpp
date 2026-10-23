#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace mirante::http {

inline nlohmann::json GetOpenApiSpec(const std::string& host_url) {
    using json = nlohmann::json;

    return json{
        {"openapi", "3.1.0"},
        {"info", {
            {"title", "MIRANTE API — Experiência Assistida de Percepção Relacional"},
            {"version", "0.1.0"},
            {"description", "API científica de alta fidelidade ao Método de Percepção Relacional da Realidade (MPRR v0.1.0). "
                            "Operacionalizada em C++26 com persistência transacional SQLite3, encadeamento de eventos Merkle SHA-256 (INV-03) "
                            "e integração desacoplada a modelos locais (Ollama). Pronta para interação com ChatGPT Custom Actions, Claude e agentes autônomos."},
            {"contact", {
                {"name", "José Pedro Trindade / SisTer Labeco"},
                {"url", "https://github.com/jpereiratrindade/MIRANTE"}
            }},
            {"license", {
                {"name", "GPL v3.0"},
                {"url", "https://www.gnu.org/licenses/gpl-3.0.html"}
            }}
        }},
        {"servers", {
            {{"url", host_url}, {"description", "Servidor MIRANTE Local/Rede"}},
            {{"url", "http://127.0.0.1:8088"}, {"description", "Loopback Local"}}
        }},
        {"paths", {
            {"/api/health", {
                {"get", {
                    {"summary", "Verificação de Saúde do Sistema"},
                    {"description", "Retorna status do servidor C++26 e conectividade com o daemon local Ollama."},
                    {"responses", {
                        {"200", {
                            {"description", "Servidor operacional"},
                            {"content", {
                                {"application/json", {
                                    {"schema", {
                                        {"type", "object"},
                                        {"properties", {
                                            {"status", {{"type", "string"}, {"example", "ok"}}},
                                            {"system", {{"type", "string"}, {"example", "MIRANTE"}}},
                                            {"version", {{"type", "string"}, {"example", "0.1.0"}}},
                                            {"ollama_online", {{"type", "boolean"}}}
                                        }}
                                    }}
                                }}
                            }}
                        }}
                    }}
                }}
            }},
            {"/api/invariants", {
                {"get", {
                    {"summary", "Catálogo Canônico dos 8 Invariantes Metodológicos"},
                    {"description", "Lista os 8 invariantes normativos do protocolo MPRR impostos pelo software (INV-01 a INV-08)."},
                    {"responses", {
                        {"200", {
                            {"description", "Lista dos 8 invariantes normativos"},
                            {"content", {{"application/json", {}}}}
                        }}
                    }}
                }}
            }},
            {"/api/sessions", {
                {"get", {
                    {"summary", "Listar Sessões de Investigação"},
                    {"description", "Recupera todas as sessões científicas registradas no banco de dados."},
                    {"responses", {
                        {"200", {
                            {"description", "Array de resumos de sessões"},
                            {"content", {
                                {"application/json", {
                                    {"schema", {{"type", "array"}}}
                                }}
                            }}
                        }}
                    }}
                }},
                {"post", {
                    {"summary", "Criar Nova Sessão MPRR"},
                    {"description", "Inicia uma nova sessão definindo a porta de entrada (Caminho A ou B), o fenômeno de interesse e delimitações contextuais."},
                    {"requestBody", {
                        {"required", true},
                        {"content", {
                            {"application/json", {
                                {"schema", {
                                    {"type", "object"},
                                    {"required", {"title", "phenomenon_text"}},
                                    {"properties", {
                                        {"session_id", {{"type", "string"}, {"description", "Identificador único da sessão (ex.: SES-001)"}}},
                                        {"project_id", {{"type", "string"}, {"description", "Código do projeto de pesquisa"}}},
                                        {"title", {{"type", "string"}, {"description", "Título da sessão de percepção relacional"}}},
                                        {"entrance_door", {
                                            {"type", "string"},
                                            {"enum", {"PATH_A_KNOWLEDGE_TO_PHENOMENON", "PATH_B_PHENOMENON_TO_KNOWLEDGE"}},
                                            {"description", "Caminho A (do conhecimento à realidade) ou Caminho B (da realidade ao conhecimento)"}
                                        }},
                                        {"phenomenon_text", {{"type", "string"}, {"description", "Descrição factual e rica do fenômeno empírico"}}},
                                        {"spatial_boundary", {{"type", "string"}, {"description", "Delimitação espacial/geográfica"}}},
                                        {"temporal_boundary", {{"type", "string"}, {"description", "Delimitação temporal"}}},
                                        {"research_context", {{"type", "string"}, {"description", "Contexto científico da investigação"}}},
                                        {"author", {{"type", "string"}, {"description", "Nome do pesquisador ou responsável"}}}
                                    }}
                                }}
                            }}
                        }}
                    }},
                    {"responses", {
                        {"201", {{"description", "Sessão criada e selada no ledger Merkle"}}},
                        {"400", {{"description", "Payload inválido"}}},
                        {"422", {{"description", "Violação de invariante metodológico"}}}
                    }}
                }}
            }},
            {"/api/sessions/{session_id}", {
                {"get", {
                    {"summary", "Obter Detalhes da Sessão"},
                    {"description", "Retorna a estrutura completa da sessão: fenômeno, deslocamento (M1), relações (M2), questões (M3), confronto (C4) e reposicionamento (C5)."},
                    {"parameters", {
                        {{"name", "session_id"}, {"in", "path"}, {"required", true}, {"schema", {{"type", "string"}}}}
                    }},
                    {"responses", {
                        {"200", {{"description", "Dados completos da sessão"}}},
                        {"404", {{"description", "Sessão não encontrada"}}}
                    }}
                }}
            }},
            {"/api/sessions/{session_id}/shift", {
                {"post", {
                    {"summary", "Movimento 1: Registrar Deslocamento de Perspectiva"},
                    {"description", "Registra a perspectiva inicial, a perspectiva deslocada, a dimensão de variação e a justificativa metodológica (INV-07)."},
                    {"parameters", {
                        {{"name", "session_id"}, {"in", "path"}, {"required", true}, {"schema", {{"type", "string"}}}}
                    }},
                    {"requestBody", {
                        {"required", true},
                        {"content", {
                            {"application/json", {
                                {"schema", {
                                    {"type", "object"},
                                    {"required", {"initial_framework", "shifted_framework", "rationale"}},
                                    {"properties", {
                                        {"initial_framework", {{"type", "string"}}},
                                        {"shifted_framework", {{"type", "string"}}},
                                        {"rationale", {{"type", "string"}}},
                                        {"author", {{"type", "string"}}},
                                        {"is_assisted", {{"type", "boolean"}}},
                                        {"model_digest", {{"type", "string"}}}
                                    }}
                                }}
                            }}
                        }}
                    }},
                    {"responses", {
                        {"200", {{"description", "Deslocamento registrado e integrado à cadeia Merkle"}}},
                        {"422", {{"description", "Violação do invariante INV-07"}}}
                    }}
                }}
            }},
            {"/api/sessions/{session_id}/relation", {
                {"post", {
                    {"summary", "Movimento 2: Adicionar Relação Candidata"},
                    {"description", "Registra relação entre dois termos (Termo A e Termo B). Toda relação nasce compulsoriamente como CANDIDATA (INV-01)."},
                    {"parameters", {
                        {{"name", "session_id"}, {"in", "path"}, {"required", true}, {"schema", {{"type", "string"}}}}
                    }},
                    {"requestBody", {
                        {"required", true},
                        {"content", {
                            {"application/json", {
                                {"schema", {
                                    {"type", "object"},
                                    {"required", {"term_a", "term_b", "description"}},
                                    {"properties", {
                                        {"term_a", {{"type", "string"}}},
                                        {"term_b", {{"type", "string"}}},
                                        {"description", {{"type", "string"}}},
                                        {"author", {{"type", "string"}}},
                                        {"is_assisted", {{"type", "boolean"}}},
                                        {"model_digest", {{"type", "string"}}}
                                    }}
                                }}
                            }}
                        }}
                    }},
                    {"responses", {
                        {"200", {{"description", "Relação candidata registrada"}}},
                        {"422", {{"description", "Violação do invariante INV-01 ou INV-04"}}}
                    }}
                }}
            }},
            {"/api/sessions/{session_id}/question", {
                {"post", {
                    {"summary", "Movimento 3: Adicionar Questão Científica Investigável"},
                    {"description", "Formula questão científica diretamente atrelada a uma relação, especificando a via de evidência necessária para teste empírico."},
                    {"parameters", {
                        {{"name", "session_id"}, {"in", "path"}, {"required", true}, {"schema", {{"type", "string"}}}}
                    }},
                    {"requestBody", {
                        {"required", true},
                        {"content", {
                            {"application/json", {
                                {"schema", {
                                    {"type", "object"},
                                    {"required", {"question_text"}},
                                    {"properties", {
                                        {"relation_id", {{"type", "string"}}},
                                        {"question_text", {{"type", "string"}}},
                                        {"evidence_path", {{"type", "string"}}},
                                        {"author", {{"type", "string"}}},
                                        {"is_assisted", {{"type", "boolean"}}},
                                        {"model_digest", {{"type", "string"}}}
                                    }}
                                }}
                            }}
                        }}
                    }},
                    {"responses", {
                        {"200", {{"description", "Questão científica registrada"}}}
                    }}
                }}
            }},
            {"/api/sessions/{session_id}/confront", {
                {"post", {
                    {"summary", "Controle Crítico C4: Registrar Confronto da Hipótese"},
                    {"description", "Submete a hipótese formulada ao contraditório obrigatório. Exige no mínimo 1 explicação rival e 1 risco epistêmico (INV-02)."},
                    {"parameters", {
                        {{"name", "session_id"}, {"in", "path"}, {"required", true}, {"schema", {{"type", "string"}}}}
                    }},
                    {"requestBody", {
                        {"required", true},
                        {"content", {
                            {"application/json", {
                                {"schema", {
                                    {"type", "object"},
                                    {"properties", {
                                        {"rivals", {
                                            {"type", "array"},
                                            {"items", {
                                                {"type", "object"},
                                                {"properties", {
                                                    {"text", {{"type", "string"}}},
                                                    {"author", {{"type", "string"}}}
                                                }}
                                            }}
                                        }},
                                        {"risks", {
                                            {"type", "array"},
                                            {"items", {
                                                {"type", "object"},
                                                {"properties", {
                                                    {"text", {{"type", "string"}}},
                                                    {"author", {{"type", "string"}}}
                                                }}
                                            }}
                                        }}
                                    }}
                                }}
                            }}
                        }}
                    }},
                    {"responses", {
                        {"200", {{"description", "Confronto crítico consolidado"}}},
                        {"422", {{"description", "Violação do contraditório (INV-02)"}}}
                    }}
                }}
            }},
            {"/api/sessions/{session_id}/reposition", {
                {"post", {
                    {"summary", "Controle Crítico C5: Consolidar Reposicionamento Epistêmico"},
                    {"description", "Consolida a decisão do ciclo (DEVELOP, REFORMULATE, SUSPEND, REJECT, SUSTAINED_BY_SPECIFIC_EVIDENCE) com justificativa e próximo passo empírico (INV-08). Se suspender, exige condições de retomada (INV-06)."},
                    {"parameters", {
                        {{"name", "session_id"}, {"in", "path"}, {"required", true}, {"schema", {{"type", "string"}}}}
                    }},
                    {"requestBody", {
                        {"required", true},
                        {"content", {
                            {"application/json", {
                                {"schema", {
                                    {"type", "object"},
                                    {"required", {"decision", "justification"}},
                                    {"properties", {
                                        {"decision", {
                                            {"type", "string"},
                                            {"enum", {"DEVELOP", "REFORMULATE", "SUSPEND", "REJECT", "SUSTAINED_BY_SPECIFIC_EVIDENCE"}}
                                        }},
                                        {"justification", {{"type", "string"}}},
                                        {"next_empirical_test", {{"type", "string"}}},
                                        {"required_knowledge", {{"type", "string"}}},
                                        {"resumption_criteria", {{"type", "string"}}},
                                        {"author", {{"type", "string"}}}
                                    }}
                                }}
                            }}
                        }}
                    }},
                    {"responses", {
                        {"200", {{"description", "Reposicionamento consolidado e ciclo avançado"}}},
                        {"422", {{"description", "Violação de INV-06 ou INV-08"}}}
                    }}
                }}
            }},
            {"/api/sessions/{session_id}/audit", {
                {"get", {
                    {"summary", "Auditar Trilha Criptográfica de Eventos (Merkle Ledger)"},
                    {"description", "Executa a verificação completa da integridade SHA-256 de todos os eventos da sessão e retorna o histórico sequencial auditável (INV-03)."},
                    {"parameters", {
                        {{"name", "session_id"}, {"in", "path"}, {"required", true}, {"schema", {{"type", "string"}}}}
                    }},
                    {"responses", {
                        {"200", {
                            {"description", "Resultado da auditoria criptográfica e cadeia Merkle"},
                            {"content", {
                                {"application/json", {
                                    {"schema", {
                                        {"type", "object"},
                                        {"properties", {
                                            {"session_id", {{"type", "string"}}},
                                            {"is_valid", {{"type", "boolean"}}},
                                            {"events_count", {{"type", "integer"}}},
                                            {"head_hash", {{"type", "string"}}},
                                            {"events", {{"type", "array"}}}
                                        }}
                                    }}
                                }}
                            }}
                        }}
                    }}
                }}
            }}
        }}
    };
}

inline std::string GetSwaggerHtml(const std::string& openapi_url) {
    return std::format(R"(<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <title>MIRANTE — Documentação da API Científica</title>
  <link rel="stylesheet" href="https://unpkg.com/swagger-ui-dist@5/swagger-ui.css" />
  <link rel="icon" type="image/png" href="data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><text y='.9em' font-size='90'>🔬</text></svg>">
  <style>
    body {{
      margin: 0;
      background-color: #0b1526;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    }}
    .swagger-ui .topbar {{ display: none; }}
    .mirante-banner {{
      background: linear-gradient(135deg, #071e38 0%, #0c2844 100%);
      color: #edf2f8;
      padding: 1.5rem 2rem;
      border-bottom: 2px solid #1a7dc4;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }}
    .mirante-banner h1 {{
      margin: 0;
      font-size: 1.4rem;
      letter-spacing: 0.05em;
    }}
    .mirante-banner p {{
      margin: 0.3rem 0 0 0;
      font-size: 0.85rem;
      color: #94a3b8;
    }}
    .mirante-badge {{
      background: #1a7dc4;
      color: white;
      padding: 0.35rem 0.8rem;
      border-radius: 9999px;
      font-size: 0.75rem;
      font-weight: 700;
      letter-spacing: 0.05em;
    }}
    .swagger-ui {{
      background: #ffffff;
      padding: 1rem 2rem;
      max-width: 1200px;
      margin: 1.5rem auto;
      border-radius: 8px;
      box-shadow: 0 10px 25px rgba(0,0,0,0.3);
    }}
  </style>
</head>
<body>
  <div class="mirante-banner">
    <div>
      <h1>Ecossistema SisTer — MIRANTE API</h1>
      <p>Experiência Assistida de Percepção Relacional • C++26 • Merkle-Chain SHA-256</p>
    </div>
    <span class="mirante-badge">MPRR v0.1.0</span>
  </div>
  <div id="swagger-ui"></div>
  <script src="https://unpkg.com/swagger-ui-dist@5/swagger-ui-bundle.js"></script>
  <script>
    window.onload = () => {{
      window.ui = SwaggerUIBundle({{
        url: '{0}',
        dom_id: '#swagger-ui',
        deepLinking: true,
        presets: [
          SwaggerUIBundle.presets.apis,
          SwaggerUIBundle.SwaggerUIStandalonePreset
        ],
        layout: "BaseLayout"
      }});
    }};
  </script>
</body>
</html>
)", openapi_url);
}

} // namespace mirante::http
