#pragma once

#include "mirante/domain/types.hpp"
#include <string>
#include <vector>
#include <expected>
#include <functional>

namespace mirante::assist {

struct OllamaModelInfo {
    std::string name;
    std::string digest;
    uint64_t size_bytes;
};

class OllamaClient {
public:
    explicit OllamaClient(std::string host = "127.0.0.1", int port = 11434);
    ~OllamaClient() = default;

    bool IsOnline();
    std::vector<OllamaModelInfo> ListModels();

    // M1: Sugestão de Deslocamento
    std::expected<std::string, std::string> SuggestShift(
        const std::string& phenomenon, 
        const std::string& current_framework,
        const std::string& model = "qwen2.5:14b"
    );

    // M2: Sugestão de Relações
    std::expected<std::string, std::string> SuggestRelations(
        const std::string& phenomenon,
        const std::string& shifted_framework,
        const std::string& model = "qwen2.5:14b"
    );

    // M3: Sugestão de Problematização
    std::expected<std::string, std::string> SuggestQuestions(
        const std::string& relation_text,
        const std::string& model = "qwen2.5:14b"
    );

    // C4: Advogado do Diabo / Explicações Rivais
    std::expected<std::string, std::string> ConfrontHypothesis(
        const std::string& hypothesis_formulation,
        const std::string& model = "qwen2.5:14b"
    );

private:
    std::string host_;
    int port_;

    std::expected<std::string, std::string> Generate(
        const std::string& model, 
        const std::string& prompt,
        const std::string& system_prompt,
        float temperature
    );
};

} // namespace mirante::assist
