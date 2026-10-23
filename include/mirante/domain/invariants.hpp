#pragma once

#include "types.hpp"
#include <string>
#include <expected>
#include <vector>

namespace mirante::domain {

enum class InvariantErrorCode {
    Success = 0,
    INV01_PrematureCertainty = 0x1001,
    INV02_MissingConfrontationRequirements = 0x1002,
    INV03_MutationForbidden = 0x1003,
    INV04_AnonymousProvenance = 0x1004,
    INV05_AiCouplingDetected = 0x1005,
    INV06_AmbiguousSuspension = 0x1006,
    INV07_IncompleteShift = 0x1007,
    INV08_DeadEndReposition = 0x1008
};

struct InvariantViolation {
    InvariantErrorCode code;
    std::string invariant_id;
    std::string message;
    std::string corrective_hint;
};

class InvariantValidator {
public:
    // INV-01: Proibição de Fato Demonstrado Prematuro
    static std::expected<void, InvariantViolation> ValidateRelationStatus(RelationStatus status, const std::string& formulation);

    // INV-02: Obrigatoriedade do Contraditório no Reposicionamento
    static std::expected<void, InvariantViolation> ValidateConfrontationCompleteness(
        size_t rival_explanations_count, 
        size_t epistemic_risks_count
    );

    // INV-04: Transparência Categórica de Proveniência
    static std::expected<void, InvariantViolation> ValidateProvenance(const ProvenancedText& item);

    // INV-06: Conservação de Incerteza (Suspensão exige condições de retomada)
    static std::expected<void, InvariantViolation> ValidateSuspensionCriteria(
        HypothesisStatus decision, 
        const std::string& resumption_criteria
    );

    // INV-07: Rastreabilidade Obrigatória de Enquadramento no Deslocamento
    static std::expected<void, InvariantViolation> ValidatePerspectiveShift(
        const std::string& initial_framework,
        const std::string& shifted_framework,
        const std::string& rationale
    );

    // INV-08: Obrigatoriedade de Próximo Passo Epistêmico
    static std::expected<void, InvariantViolation> ValidateRepositionNextStep(
        const std::string& next_empirical_test,
        const std::string& required_knowledge
    );
};

} // namespace mirante::domain
