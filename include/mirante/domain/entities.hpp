#pragma once

#include "types.hpp"
#include "invariants.hpp"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <optional>
#include <expected>

namespace mirante::domain {

// M1: Deslocamento de Perspectiva
struct PerspectiveShift {
    std::string id;
    std::string initial_framework;
    std::string shifted_framework;
    ShiftDimension dimension{ShiftDimension::TimeHorizon};
    ProvenancedText rationale;
    MovementState state{MovementState::Draft};
    std::string created_at_iso;
};

// M2: Relação Candidata
struct CandidateRelation {
    std::string id;
    std::string term_a;
    std::string term_b;
    RelationType type{RelationType::ConjecturedMechanism};
    RelationStatus status{RelationStatus::Candidate}; // INV-01: Sempre Candidate
    ProvenancedText description;
    MovementState state{MovementState::Draft};
    std::string created_at_iso;
};

// M3: Problematização e Questão Científica
struct ScientificQuestion {
    std::string id;
    std::string relation_id;
    ProvenancedText question_text;
    std::string required_evidence_path; // Indicadores ou caminhos empíricos
    std::string intelligibility_note;
    std::string contestability_note;
    MovementState state{MovementState::Draft};
    std::string created_at_iso;
};

// Hipótese
struct Hypothesis {
    std::string id;
    std::string question_id;
    ProvenancedText formulation;
    HypothesisStatus status{HypothesisStatus::NewCandidate};
    uint32_t cycle_version{1};
    std::string created_at_iso;
};

// C4: Confronto Crítico
struct ConfrontationRecord {
    std::string id;
    std::string hypothesis_id;
    std::vector<ProvenancedText> rival_explanations; // Mínimo 1 (INV-02)
    std::vector<ProvenancedText> epistemic_risks;     // Mínimo 1 (INV-02)
    std::vector<ProvenancedText> counter_arguments;
    std::vector<std::string> omitted_actors_or_disciplines;
    bool is_concluded{false};
};

// C5: Decisão de Reposicionamento
struct RepositionDecision {
    std::string id;
    std::string session_id;
    uint32_t cycle_version{1};
    HypothesisStatus decision{HypothesisStatus::InAnalysis};
    ProvenancedText verdict_justification;
    std::string next_empirical_test;   // Requisito INV-08
    std::string required_knowledge;    // Requisito INV-08
    std::string resumption_criteria;   // Requisito INV-06 se for SUSPENDER
    std::string decided_at_iso;
};

// Evento Imutável no Ledger da Sessão (Merkle-Chain)
struct SessionEvent {
    std::string event_id;
    std::string session_id;
    uint64_t sequence_number{0};
    std::string previous_hash;
    std::string event_type;
    std::string payload_json;
    std::string author;
    std::string timestamp_iso;
    std::string current_hash;
};

// Agregado Raiz da Sessão MPRR
class SessionAggregate {
public:
    std::string session_id;
    std::string project_id;
    std::string title;
    EntranceDoor entrance_door{EntranceDoor::PathB_PhenomenonToKnowledge};
    SessionState state{SessionState::InitialDraft};
    uint32_t current_cycle{1};

    // Dados do Fenômeno e Ficha
    ProvenancedText phenomenon_description;
    std::string spatial_boundary;
    std::string temporal_boundary;
    std::string research_context;
    std::vector<std::string> initial_sources;
    std::vector<std::string> participants;

    // Núcleo Heurístico (M1, M2, M3)
    std::optional<PerspectiveShift> shift;
    std::vector<CandidateRelation> relations;
    std::vector<ScientificQuestion> questions;
    std::vector<Hypothesis> hypotheses;

    // Controles Críticos (C4, C5)
    std::optional<ConfrontationRecord> confrontation;
    std::vector<RepositionDecision> reposition_history;

    // Métodos de Negócio
    std::expected<void, InvariantViolation> SetPerspectiveShift(
        const std::string& initial_fw,
        const std::string& shifted_fw,
        ShiftDimension dim,
        const ProvenancedText& rationale
    );

    std::expected<void, InvariantViolation> AddCandidateRelation(
        const std::string& term_a,
        const std::string& term_b,
        RelationType rel_type,
        const ProvenancedText& description
    );

    std::expected<void, InvariantViolation> AddScientificQuestion(
        const std::string& relation_id,
        const ProvenancedText& question_text,
        const std::string& evidence_path
    );

    std::expected<void, InvariantViolation> RecordConfrontation(
        const std::vector<ProvenancedText>& rivals,
        const std::vector<ProvenancedText>& risks,
        const std::vector<ProvenancedText>& counters
    );

    std::expected<void, InvariantViolation> ConsolidateReposition(
        HypothesisStatus decision,
        const ProvenancedText& justification,
        const std::string& next_step_test,
        const std::string& req_knowledge,
        const std::string& resumption_crit = ""
    );
};

} // namespace mirante::domain
