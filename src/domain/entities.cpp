#include "mirante/domain/entities.hpp"
#include <format>

namespace mirante::domain {

std::expected<void, InvariantViolation> SessionAggregate::SetPerspectiveShift(
    const std::string& initial_fw,
    const std::string& shifted_fw,
    ShiftDimension dim,
    const ProvenancedText& rationale
) {
    auto val_shift = InvariantValidator::ValidatePerspectiveShift(initial_fw, shifted_fw, rationale.text);
    if (!val_shift) return val_shift;

    auto val_prov = InvariantValidator::ValidateProvenance(rationale);
    if (!val_prov) return val_prov;

    shift = PerspectiveShift{
        .id = std::format("SHIFT-{}-{}", session_id, current_cycle),
        .initial_framework = initial_fw,
        .shifted_framework = shifted_fw,
        .dimension = dim,
        .rationale = rationale,
        .state = MovementState::ConsolidatedCycle,
        .created_at_iso = rationale.timestamp_iso
    };

    if (state == SessionState::InitialDraft) {
        state = SessionState::InExploration;
    }
    return {};
}

std::expected<void, InvariantViolation> SessionAggregate::AddCandidateRelation(
    const std::string& term_a,
    const std::string& term_b,
    RelationType rel_type,
    const ProvenancedText& description
) {
    auto val_status = InvariantValidator::ValidateRelationStatus(RelationStatus::Candidate, description.text);
    if (!val_status) return val_status;

    auto val_prov = InvariantValidator::ValidateProvenance(description);
    if (!val_prov) return val_prov;

    relations.push_back(CandidateRelation{
        .id = std::format("REL-{}-{}", session_id, relations.size() + 1),
        .term_a = term_a,
        .term_b = term_b,
        .type = rel_type,
        .status = RelationStatus::Candidate,
        .description = description,
        .state = MovementState::ConsolidatedCycle,
        .created_at_iso = description.timestamp_iso
    });

    return {};
}

std::expected<void, InvariantViolation> SessionAggregate::AddScientificQuestion(
    const std::string& relation_id,
    const ProvenancedText& question_text,
    const std::string& evidence_path
) {
    auto val_prov = InvariantValidator::ValidateProvenance(question_text);
    if (!val_prov) return val_prov;

    std::string q_id = std::format("QST-{}-{}", session_id, questions.size() + 1);

    questions.push_back(ScientificQuestion{
        .id = q_id,
        .relation_id = relation_id,
        .question_text = question_text,
        .required_evidence_path = evidence_path,
        .intelligibility_note = "",
        .contestability_note = "",
        .state = MovementState::ConsolidatedCycle,
        .created_at_iso = question_text.timestamp_iso
    });

    // Gera hipótese candidata associada à questão
    hypotheses.push_back(Hypothesis{
        .id = std::format("HIP-{}-{}", session_id, hypotheses.size() + 1),
        .question_id = q_id,
        .formulation = question_text,
        .status = HypothesisStatus::NewCandidate,
        .cycle_version = current_cycle,
        .created_at_iso = question_text.timestamp_iso
    });

    if (state == SessionState::InExploration) {
        state = SessionState::AwaitingConfrontation;
    }
    return {};
}

std::expected<void, InvariantViolation> SessionAggregate::RecordConfrontation(
    const std::vector<ProvenancedText>& rivals,
    const std::vector<ProvenancedText>& risks,
    const std::vector<ProvenancedText>& counters
) {
    auto val_conf = InvariantValidator::ValidateConfrontationCompleteness(rivals.size(), risks.size());
    if (!val_conf) return val_conf;

    for (const auto& r : rivals) {
        auto v = InvariantValidator::ValidateProvenance(r);
        if (!v) return v;
    }
    for (const auto& r : risks) {
        auto v = InvariantValidator::ValidateProvenance(r);
        if (!v) return v;
    }

    std::string target_hip = hypotheses.empty() ? "" : hypotheses.back().id;

    confrontation = ConfrontationRecord{
        .id = std::format("CNF-{}-{}", session_id, current_cycle),
        .hypothesis_id = target_hip,
        .rival_explanations = rivals,
        .epistemic_risks = risks,
        .counter_arguments = counters,
        .omitted_actors_or_disciplines = {},
        .is_concluded = true
    };

    state = SessionState::ReadyForReposition;
    return {};
}

std::expected<void, InvariantViolation> SessionAggregate::ConsolidateReposition(
    HypothesisStatus decision,
    const ProvenancedText& justification,
    const std::string& next_step_test,
    const std::string& req_knowledge,
    const std::string& resumption_crit
) {
    if (!confrontation.has_value() || !confrontation->is_concluded) {
        return std::unexpected(InvariantViolation{
            .code = InvariantErrorCode::INV02_MissingConfrontationRequirements,
            .invariant_id = "INV-02",
            .message = "Tentativa de reposicionamento sem registro de confronto prévio.",
            .corrective_hint = "Acesse a Arena de Confronto e registre as explicações rivais e riscos antes de concluir o ciclo."
        });
    }

    auto val_step = InvariantValidator::ValidateRepositionNextStep(next_step_test, req_knowledge);
    if (!val_step) return val_step;

    auto val_susp = InvariantValidator::ValidateSuspensionCriteria(decision, resumption_crit);
    if (!val_susp) return val_susp;

    auto val_prov = InvariantValidator::ValidateProvenance(justification);
    if (!val_prov) return val_prov;

    reposition_history.push_back(RepositionDecision{
        .id = std::format("REP-{}-{}", session_id, current_cycle),
        .session_id = session_id,
        .cycle_version = current_cycle,
        .decision = decision,
        .verdict_justification = justification,
        .next_empirical_test = next_step_test,
        .required_knowledge = req_knowledge,
        .resumption_criteria = resumption_crit,
        .decided_at_iso = justification.timestamp_iso
    });

    if (!hypotheses.empty()) {
        hypotheses.back().status = decision;
    }

    state = SessionState::CycleConsolidated;
    return {};
}

} // namespace mirante::domain
