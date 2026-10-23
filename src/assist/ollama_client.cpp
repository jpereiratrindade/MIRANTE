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
    float temperature
) {
    httplib::Client cli(host_, port_);
    cli.set_connection_timeout(std::chrono::seconds(5));
    cli.set_read_timeout(std::chrono::seconds(120)); // LLM local pode demorar

    json payload;
    payload["model"] = model;
    payload["prompt"] = prompt;
    payload["system"] = system_prompt.empty() ? kMiranteSystemPrompt : system_prompt;
    payload["stream"] = false;
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

} // namespace mirante::assist
