#include "mirante/assist/ollama_client.hpp"
#include <httplib/httplib.h>
#include <nlohmann/json.hpp>
#include <format>

using json = nlohmann::json;

namespace mirante::assist {

static const char* kMiranteSystemPrompt = R"(
Você é o Assistente Socrático do sistema MIRANTE (Ecossistema SisTer), atuando estritamente sob o Método de Percepção Relacional da Realidade (MPRR v0.1.0).

DEVERES EPISTÊMICOS OBRIGATÓRIOS:
1. Jamais afirme que uma hipótese ou relação está "provada" ou "demonstrada". Trate tudo como possibilidade, conjectura ou relação candidata.
2. Jamais invente dados empíricos, números fictícios ou fontes falsas.
3. Use tom científico, sóbrio, objetivo e direto ao ponto. Sem elogios ou floreios.
4. Responda em português claro e rigoroso.
)";

OllamaClient::OllamaClient(std::string host, int port)
    : host_(std::move(host)), port_(port) {}

bool OllamaClient::IsOnline() {
    httplib::Client cli(host_, port_);
    cli.set_connection_timeout(std::chrono::seconds(1));
    cli.set_read_timeout(std::chrono::seconds(1));
    auto res = cli.Get("/api/tags");
    return res && res->status == 200;
}

std::vector<OllamaModelInfo> OllamaClient::ListModels() {
    std::vector<OllamaModelInfo> models;
    httplib::Client cli(host_, port_);
    cli.set_connection_timeout(std::chrono::seconds(2));
    cli.set_read_timeout(std::chrono::seconds(3));
    auto res = cli.Get("/api/tags");
    if (!res || res->status != 200) {
        return models;
    }

    try {
        json j = json::parse(res->body);
        if (j.contains("models")) {
            for (const auto& m : j["models"]) {
                std::string name = m.value("name", "");
                // Filtra modelos de embedding puros que não geram texto
                if (name.find("embed") != std::string::npos) {
                    continue;
                }
                models.push_back(OllamaModelInfo{
                    .name = name,
                    .digest = m.value("digest", "").substr(0, 12),
                    .size_bytes = m.value("size", 0ULL)
                });
            }
        }
    } catch (...) {}

    return models;
}

std::expected<std::string, std::string> OllamaClient::Generate(
    const std::string& model, 
    const std::string& prompt,
    const std::string& system_prompt,
    float temperature,
    bool json_format
) {
    httplib::Client cli(host_, port_);
    cli.set_connection_timeout(std::chrono::seconds(5));
    cli.set_read_timeout(std::chrono::seconds(120)); // LLM local pode demorar

    json payload;
    payload["model"] = model;
    payload["prompt"] = prompt;
    payload["system"] = system_prompt.empty() ? kMiranteSystemPrompt : system_prompt;
    payload["stream"] = false;
    if (json_format) {
        payload["format"] = "json";
    }
    payload["options"] = {
        {"temperature", temperature},
        {"top_p", 0.9}
    };

    auto res = cli.Post("/api/generate", payload.dump(), "application/json");
    if (!res) {
        return std::unexpected("Falha de conexão com o daemon Ollama em " + host_ + ":" + std::to_string(port_));
    }
    if (res->status != 200) {
        return std::unexpected(std::format("Ollama retornou HTTP {}: {}", res->status, res->body));
    }

    try {
        json resp_j = json::parse(res->body);
        return resp_j.value("response", "");
    } catch (const std::exception& e) {
        return std::unexpected(std::string("Erro no parse da resposta: ") + e.what());
    }
}

std::expected<nlohmann::json, std::string> OllamaClient::GenerateJson(
    const std::string& model,
    const std::string& prompt,
    const std::string& system_prompt,
    float temperature
) {
    auto res = Generate(model, prompt, system_prompt, temperature, true);
    if (!res) return std::unexpected(res.error());

    std::string text = *res;
    size_t first = text.find('{');
    size_t last = text.rfind('}');
    if (first != std::string::npos && last != std::string::npos && last >= first) {
        text = text.substr(first, last - first + 1);
    }

    try {
        return json::parse(text);
    } catch (const std::exception& e) {
        return std::unexpected(std::string("Falha ao analisar JSON retornado pelo modelo local: ") + e.what() + " | Conteúdo: " + *res);
    }
}

std::expected<std::string, std::string> OllamaClient::SuggestShift(
    const std::string& phenomenon, 
    const std::string& current_framework,
    const std::string& model
) {
    std::string prompt = std::format(
R"(Considere o seguinte fenômeno em investigação:
"{}"

Enquadramento habitual / perspectiva inicial:
"{}"

No protocolo MPRR (Movimento 1 — Deslocar), varie deliberadamente as condições de observação. Proponha 2 a 3 deslocamentos de perspectiva concretos (ex: variar escala temporal, unidade de análise, história de legados, atores sociais ou processos ecossistêmicos não privilegiados).
Para cada deslocamento, forneça:
1. Nova dimensão de observação
2. O que se torna observável a partir dela
3. Justificativa metodológica da mudança)",
        phenomenon, current_framework
    );

    return Generate(model, prompt, kMiranteSystemPrompt, 0.65f);
}

std::expected<std::string, std::string> OllamaClient::SuggestRelations(
    const std::string& phenomenon,
    const std::string& shifted_framework,
    const std::string& model
) {
    std::string prompt = std::format(
R"(Fenômeno: "{}"
Perspectiva deslocada de observação: "{}"

No protocolo MPRR (Movimento 2 — Relacionar), que relações candidatas se tornam mais nítidas sob essa nova perspectiva?
Proponha 2 relações candidatas explicitando:
- Termo A e Termo B
- Natureza do vínculo (ex: Mecanismo Conjecturado, Dependência de Contexto, Mediação ou Feedback)
- Descrição da relação (lembre-se: marcar como candidata, nunca como fato demonstrado))",
        phenomenon, shifted_framework
    );

    return Generate(model, prompt, kMiranteSystemPrompt, 0.60f);
}

std::expected<std::string, std::string> OllamaClient::SuggestQuestions(
    const std::string& relation_text,
    const std::string& model
) {
    std::string prompt = std::format(
R"(Relação candidata proposta:
"{}"

No protocolo MPRR (Movimento 3 — Problematizar), converta essa relação em questões científicas investigáveis:
1. Formule uma questão interrogativa clara.
2. Indique que tipo de evidência empírica ou dado seria necessário para responder à pergunta.
3. Avalie brevemente a contestabilidade da questão.)",
        relation_text
    );

    return Generate(model, prompt, kMiranteSystemPrompt, 0.40f);
}

std::expected<std::string, std::string> OllamaClient::ConfrontHypothesis(
    const std::string& hypothesis_formulation,
    const std::string& model
) {
    std::string prompt = std::format(
R"(Hipótese sob escrutínio:
"{}"

Atue como ADVOGADO DO DIABO no protocolo MPRR (Controle 4 — Confrontar).
Submeta essa hipótese a escrutínio rigoroso:
1. Formule ao menos UMA Explicação Rival plausível (outra causa que explicaria os mesmos fatos).
2. Aponte ao menos UM Risco Epistêmico de Invalidação (risco de sobre-interpretação, correlação espúria, causalidade reversa ou mistura indevida de escalas).
3. Especifique que observação concreta falsearia a proposição.)",
        hypothesis_formulation
    );

    return Generate(model, prompt, kMiranteSystemPrompt, 0.25f);
}

// ----------------------------------------------------------------------
// Métodos de Preenchimento Direto Estruturado (JSON)
// ----------------------------------------------------------------------

std::expected<nlohmann::json, std::string> OllamaClient::FillM1(
    const std::string& phenomenon,
    const std::string& current_framework,
    const std::string& model
) {
    std::string prompt = std::format(
R"PROMPT(Você é um assistente científico de alto nível operando no protocolo MPRR (Método de Percepção Relacional e Redescrição).
O pesquisador deseja que você preencha DIRETAMENTE o formulário do Movimento 1 (M1 — Deslocamento Epistêmico).

Fenômeno investigado: "{}"
Enquadramento habitual / perspectiva inicial: "{}"

Gere uma resposta ESTRITAMENTE em formato JSON (sem markdown, sem preâmbulo) com a estrutura:
{{
  "initial_framework": "descrição concisa e situada do enquadramento habitual e pressupostos dominantes",
  "shifted_framework": "nova perspectiva de observação explicitamente deslocada da convencional",
  "dimension": "SCALE",
  "rationale": "justificativa metodológica concisa de por que essa perspectiva revela aspectos ocultos"
}}

Para o campo "dimension", escolha exatamente um de: "SCALE", "TEMPORAL", "AGENT", "BOUNDARY", "PRIVILEGED_VARIABLE".)PROMPT",
        phenomenon.empty() ? "Dinâmica sistêmica em estudo" : phenomenon,
        current_framework.empty() ? "Enquadramento convencional da literatura" : current_framework
    );

    return GenerateJson(model, prompt, kMiranteSystemPrompt, 0.50f);
}

std::expected<nlohmann::json, std::string> OllamaClient::FillM2(
    const std::string& phenomenon,
    const std::string& shifted_framework,
    const std::string& model
) {
    std::string prompt = std::format(
R"PROMPT(Você é um assistente científico no protocolo MPRR (Movimento 2 — Exploração de Relações Candidatas).
O pesquisador deseja que você preencha DIRETAMENTE o formulário do Movimento 2 (M2). Toda relação nasce compulsoriamente como CANDIDATA.

Fenômeno: "{}"
Perspectiva deslocada de observação (M1): "{}"

Gere uma resposta ESTRITAMENTE em formato JSON com a estrutura:
{{
  "term_a": "Componente ou termo A observado",
  "term_b": "Componente ou termo B correlacionado",
  "rel_type": "FEEDBACK_LOOP",
  "description": "descrição relacional conjeturada conectando os termos sob a nova perspectiva"
}}

Para o campo "rel_type", escolha exatamente um de: "MODULATION", "FEEDBACK_LOOP", "THRESHOLD", "TRADE_OFF", "CAUSAL_HYPOTHESIS".)PROMPT",
        phenomenon.empty() ? "Fenômeno em estudo" : phenomenon,
        shifted_framework.empty() ? "Perspectiva sistêmica relacional" : shifted_framework
    );

    return GenerateJson(model, prompt, kMiranteSystemPrompt, 0.50f);
}

std::expected<nlohmann::json, std::string> OllamaClient::FillM3(
    const std::string& relation_text,
    const std::string& model
) {
    std::string prompt = std::format(
R"PROMPT(Você é um assistente científico no protocolo MPRR (Movimento 3 — Problematização Heurística).
O pesquisador deseja que você preencha DIRETAMENTE o formulário do Movimento 3 (M3), transformando a relação em pergunta investigável e falseável.

Relação candidata: "{}"

Gere uma resposta ESTRITAMENTE em formato JSON com a estrutura:
{{
  "question": "Enunciado interrogativo claro, contestável e empiricamente investigável",
  "evidence_hint": "morfo://amostras/protocolo-teste.csv ou tipo de rastro empírico exigido"
}})PROMPT",
        relation_text.empty() ? "Relação candidata entre componentes sistêmicos" : relation_text
    );

    return GenerateJson(model, prompt, kMiranteSystemPrompt, 0.40f);
}

std::expected<nlohmann::json, std::string> OllamaClient::FillConfront(
    const std::string& hypothesis,
    const std::string& model
) {
    std::string prompt = std::format(
R"PROMPT(Você é o ADVOGADO DO DIABO no protocolo MPRR (Controle C4 — Arena de Confronto Crítico).
O pesquisador deseja que você preencha DIRETAMENTE a Arena de Confronto, gerando ao menos uma explicação rival e um risco epistêmico.

Hipótese científica formulada: "{}"

Gere uma resposta ESTRITAMENTE em formato JSON com a estrutura:
{{
  "rival_hypothesis": "Explicação rival plausível alternativa que explicaria os indícios sem invocar a hipótese proposta",
  "rival_author": "Ollama [Advogado do Diabo]",
  "risk_type": "CONFIRMATION_BIAS",
  "risk_description": "Descrição analítica do risco de erro epistêmico (ex: correlação espúria por variável de confusão não controlada)",
  "risk_mitigation": "Procedimento metodológico ou teste de contraste para mitigar o risco apontado"
}}

Para o campo "risk_type", escolha exatamente um de: "CONFIRMATION_BIAS", "OVERFITTING", "CONCEPTUAL_STRETCHING", "EQUIVOCATION", "DATA_SPARSITY".)PROMPT",
        hypothesis.empty() ? "Hipótese de relação sistêmica proposta" : hypothesis
    );

    return GenerateJson(model, prompt, kMiranteSystemPrompt, 0.30f);
}

std::expected<nlohmann::json, std::string> OllamaClient::FillReposition(
    const std::string& hypothesis,
    const std::string& rivals_summary,
    const std::string& risks_summary,
    const std::string& model
) {
    std::string prompt = std::format(
R"PROMPT(Você é o assistente de síntese metodológica no protocolo MPRR (Controle C5 — Reposicionamento).
O pesquisador deseja que você preencha DIRETAMENTE a deliberação de reposicionamento com base no confronto realizado.

Hipótese: "{}"
Rivais consideradas: "{}"
Riscos avaliados: "{}"

Gere uma resposta ESTRITAMENTE em formato JSON com a estrutura:
{{
  "decision": "DEVELOP",
  "justification": "Síntese rigorosa justificando a decisão metodológica",
  "next_step": "Próximo passo empírico ou protocolo concreto de teste",
  "required_knowledge": "Lacuna de conhecimento teórico ou empírico que precisa ser preenchida",
  "resumption_criteria": "Critério explícito para retomar caso seja suspensa, ou N/A se desenvolvida"
}}

Para "decision", escolha exatamente um de: "DEVELOP", "REFORMULATE", "SUSPEND", "REJECT", "SUSTAINED_BY_SPECIFIC_EVIDENCE".)PROMPT",
        hypothesis, rivals_summary, risks_summary
    );

    return GenerateJson(model, prompt, kMiranteSystemPrompt, 0.35f);
}

std::expected<nlohmann::json, std::string> OllamaClient::FillSession(
    const std::string& topic_hint,
    const std::string& model
) {
    std::string prompt = std::format(
R"PROMPT(Você é um planejador de pesquisa científica no protocolo MPRR do ecossistema SisTer / LABECO.
O pesquisador deseja preencher DIRETAMENTE uma nova investigação na bancada epistêmica.
Semente ou tema de interesse: "{}"

Gere uma resposta ESTRITAMENTE em formato JSON com a estrutura:
{{
  "project_id": "PRJ-SISTER",
  "title": "Título conciso e estimulante da investigação",
  "entrance_door": "PORTA_B",
  "phenomenon": "Descrição rica, situada e detalhada do fenômeno ou anomalia a investigar",
  "spatial": "Delimitação espacial ou do sistema sob observação",
  "temporal": "Delimitação temporal ou do horizonte histórico sob observação"
}}

Para "entrance_door", escolha: "PORTA_A" (anomalia empírica), "PORTA_B" (deslocamento conceitual) ou "PORTA_C" (tensão epistêmica).)PROMPT",
        topic_hint.empty() ? "Resiliência socioecológica e dinâmicas agroflorestais" : topic_hint
    );

    return GenerateJson(model, prompt, kMiranteSystemPrompt, 0.50f);
}

} // namespace mirante::assist
