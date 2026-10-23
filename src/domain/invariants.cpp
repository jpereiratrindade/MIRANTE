#include "mirante/domain/invariants.hpp"
#include <expected>

namespace mirante::domain {

std::expected<void, InvariantViolation> InvariantValidator::ValidateRelationStatus(
    RelationStatus status
) {
    // INV-01: Proibição de Fato Demonstrado Prematuro
    // O sistema disciplina a máquina de estados no domínio, garantindo que
    // toda relação formulada na Bancada permaneça compulsoriamente como CANDIDATA.
    // A redação textual do pesquisador permanece livre e soberana, sem policiamento de vocabulário.
    if (status != RelationStatus::Candidate) {
        return std::unexpected(InvariantViolation{
            .code = InvariantErrorCode::INV01_PrematureCertainty,
            .invariant_id = "INV-01",
            .message = "Tentativa de registrar relação na Bancada com estado diferente de CANDIDATA.",
            .corrective_hint = "No protocolo MPRR, toda relação na Bancada nasce estritamente como CANDIDATA. A validação empírica ocorre fora do sistema."
        });
    }

    return {};
}

std::expected<void, InvariantViolation> InvariantValidator::ValidateConfrontationCompleteness(
    size_t rival_explanations_count, 
    size_t epistemic_risks_count
) {
    // INV-02: Obrigatoriedade do Contraditório no Reposicionamento
    if (rival_explanations_count < 1 || epistemic_risks_count < 1) {
        return std::unexpected(InvariantViolation{
            .code = InvariantErrorCode::INV02_MissingConfrontationRequirements,
            .invariant_id = "INV-02",
            .message = "O confronto crítico da hipótese está incompleto.",
            .corrective_hint = "É mandatório registrar no mínimo UMA Explicação Rival e no mínimo UM Risco Epistêmico de Invalidação antes de reposicionar."
        });
    }
    return {};
}

std::expected<void, InvariantViolation> InvariantValidator::ValidateProvenance(const ProvenancedText& item) {
    // INV-04: Transparência Categórica de Proveniência
    if (item.text.empty()) {
        return std::unexpected(InvariantViolation{
            .code = InvariantErrorCode::INV04_AnonymousProvenance,
            .invariant_id = "INV-04",
            .message = "Fragmento de texto vazio.",
            .corrective_hint = "Insira o conteúdo textual da formulação."
        });
    }

    if (item.author.empty()) {
        return std::unexpected(InvariantViolation{
            .code = InvariantErrorCode::INV04_AnonymousProvenance,
            .invariant_id = "INV-04",
            .message = "Origem autoral não identificada no registro de proveniência.",
            .corrective_hint = "Declare o autor humano ou a identificação do modelo de IA responsável pela proposição."
        });
    }

    if (item.origin == ProvenanceOrigin::Assisted && item.model_digest.empty()) {
        return std::unexpected(InvariantViolation{
            .code = InvariantErrorCode::INV04_AnonymousProvenance,
            .invariant_id = "INV-04",
            .message = "Intervenção assistida sem hash SHA-256 do modelo local.",
            .corrective_hint = "O registro de proveniência assistida deve conter o digest dos pesos do modelo para garantir reproducibilidade."
        });
    }

    return {};
}

std::expected<void, InvariantViolation> InvariantValidator::ValidateSuspensionCriteria(
    HypothesisStatus decision, 
    const std::string& resumption_criteria
) {
    // INV-06: Conservação de Incerteza
    if (decision == HypothesisStatus::Suspend && resumption_criteria.empty()) {
        return std::unexpected(InvariantViolation{
            .code = InvariantErrorCode::INV06_AmbiguousSuspension,
            .invariant_id = "INV-06",
            .message = "A hipótese foi suspensa sem definição das condições para retomada da investigação.",
            .corrective_hint = "Descreva expressamente que novos dados, evidências ou condições metodológicas justificariam reabrir a análise."
        });
    }
    return {};
}

std::expected<void, InvariantViolation> InvariantValidator::ValidatePerspectiveShift(
    const std::string& initial_framework,
    const std::string& shifted_framework,
    const std::string& rationale
) {
    // INV-07: Rastreabilidade Obrigatória de Enquadramento no Deslocamento
    if (initial_framework.empty() || shifted_framework.empty() || rationale.empty()) {
        return std::unexpected(InvariantViolation{
            .code = InvariantErrorCode::INV07_IncompleteShift,
            .invariant_id = "INV-07",
            .message = "Deslocamento de perspectiva incompleto.",
            .corrective_hint = "O Movimento 1 exige registrar formalmente a perspectiva inicial, a perspectiva deslocada e a justificativa da mudança."
        });
    }
    return {};
}

std::expected<void, InvariantViolation> InvariantValidator::ValidateRepositionNextStep(
    const std::string& next_empirical_test,
    const std::string& required_knowledge
) {
    // INV-08: Obrigatoriedade de Próximo Passo Epistêmico
    if (next_empirical_test.empty() && required_knowledge.empty()) {
        return std::unexpected(InvariantViolation{
            .code = InvariantErrorCode::INV08_DeadEndReposition,
            .invariant_id = "INV-08",
            .message = "Decisão de reposicionamento sem especificação de passos subsequentes.",
            .corrective_hint = "O MPRR não encerra a investigação no reposicionamento. Declare o próximo teste empírico ou o conhecimento teórico necessário."
        });
    }
    return {};
}

} // namespace mirante::domain
