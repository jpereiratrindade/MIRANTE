#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <chrono>
#include <optional>
#include <expected>

namespace mirante::domain {

enum class EntranceDoor {
    PathA_KnowledgeToPhenomenon, // Mobilizar outro conhecimento -> reinterpretar realidade
    PathB_PhenomenonToKnowledge  // Relação emergente no fenômeno -> exigir novo conhecimento
};

inline std::string_view ToString(EntranceDoor door) {
    switch (door) {
        case EntranceDoor::PathA_KnowledgeToPhenomenon: return "PATH_A_KNOWLEDGE_TO_PHENOMENON";
        case EntranceDoor::PathB_PhenomenonToKnowledge: return "PATH_B_PHENOMENON_TO_KNOWLEDGE";
    }
    return "UNKNOWN";
}

enum class ShiftDimension {
    SpatialScale,
    TimeHorizon,
    UnitOfAnalysis,
    ObserverRole,
    OrganizationLevel,
    PrivilegedVariable,
    Discipline,
    SocialActor,
    Custom
};

inline std::string_view ToString(ShiftDimension dim) {
    switch (dim) {
        case ShiftDimension::SpatialScale: return "SPATIAL_SCALE";
        case ShiftDimension::TimeHorizon: return "TIME_HORIZON";
        case ShiftDimension::UnitOfAnalysis: return "UNIT_OF_ANALYSIS";
        case ShiftDimension::ObserverRole: return "OBSERVER_ROLE";
        case ShiftDimension::OrganizationLevel: return "ORGANIZATION_LEVEL";
        case ShiftDimension::PrivilegedVariable: return "PRIVILEGED_VARIABLE";
        case ShiftDimension::Discipline: return "DISCIPLINE";
        case ShiftDimension::SocialActor: return "SOCIAL_ACTOR";
        case ShiftDimension::Custom: return "CUSTOM";
    }
    return "CUSTOM";
}

enum class RelationType {
    Association,                  // Covariação observada
    Mediation,                    // Intermediação de efeito
    ContextDependency,            // Condicionada a fatores de contorno
    Feedback,                     // Retroalimentação
    TemporalSequence,             // Assimetria temporal
    PossibleNecessaryCondition,   // Condição necessária presumida
    ConjecturedMechanism,         // Mecanismo causal hipotético
    HeuristicAnalogy              // Metáfora (declarada como não-mecanismo)
};

inline std::string_view ToString(RelationType rt) {
    switch (rt) {
        case RelationType::Association: return "ASSOCIATION";
        case RelationType::Mediation: return "MEDIATION";
        case RelationType::ContextDependency: return "CONTEXT_DEPENDENCY";
        case RelationType::Feedback: return "FEEDBACK";
        case RelationType::TemporalSequence: return "TEMPORAL_SEQUENCE";
        case RelationType::PossibleNecessaryCondition: return "POSSIBLE_NECESSARY_CONDITION";
        case RelationType::ConjecturedMechanism: return "CONJECTURED_MECHANISM";
        case RelationType::HeuristicAnalogy: return "HEURISTIC_ANALOGY";
    }
    return "UNKNOWN";
}

enum class RelationStatus {
    Candidate // INV-01: Proibição de fato demonstrado prematuro na bancada
};

inline std::string_view ToString(RelationStatus) {
    return "CANDIDATE";
}

enum class HypothesisStatus {
    NewCandidate,
    InAnalysis,
    Develop,
    Reformulate,
    Suspend,
    Reject,
    SustainedBySpecificEvidence
};

inline std::string_view ToString(HypothesisStatus s) {
    switch (s) {
        case HypothesisStatus::NewCandidate: return "NEW_CANDIDATE";
        case HypothesisStatus::InAnalysis: return "IN_ANALYSIS";
        case HypothesisStatus::Develop: return "DEVELOP";
        case HypothesisStatus::Reformulate: return "REFORMULATE";
        case HypothesisStatus::Suspend: return "SUSPEND";
        case HypothesisStatus::Reject: return "REJECT";
        case HypothesisStatus::SustainedBySpecificEvidence: return "SUSTAINED_BY_SPECIFIC_EVIDENCE";
    }
    return "UNKNOWN";
}

enum class ProvenanceOrigin {
    Human,
    Assisted,
    Imported,
    Evidence,
    Inference
};

inline std::string_view ToString(ProvenanceOrigin o) {
    switch (o) {
        case ProvenanceOrigin::Human: return "HUMANO";
        case ProvenanceOrigin::Assisted: return "ASSISTIDO";
        case ProvenanceOrigin::Imported: return "IMPORTADO";
        case ProvenanceOrigin::Evidence: return "EVIDÊNCIA";
        case ProvenanceOrigin::Inference: return "INFERÊNCIA";
    }
    return "UNKNOWN";
}

enum class MovementState {
    Draft,
    PendingCritique,
    ConsolidatedCycle,
    ReopenedForRevision
};

inline std::string_view ToString(MovementState s) {
    switch (s) {
        case MovementState::Draft: return "DRAFT";
        case MovementState::PendingCritique: return "PENDING_CRITIQUE";
        case MovementState::ConsolidatedCycle: return "CONSOLIDATED_CYCLE";
        case MovementState::ReopenedForRevision: return "REOPENED_FOR_REVISION";
    }
    return "DRAFT";
}

enum class SessionState {
    InitialDraft,
    InExploration,
    AwaitingConfrontation,
    ReadyForReposition,
    CycleConsolidated,
    ArchivedVersioned
};

inline std::string_view ToString(SessionState s) {
    switch (s) {
        case SessionState::InitialDraft: return "INITIAL_DRAFT";
        case SessionState::InExploration: return "IN_EXPLORATION";
        case SessionState::AwaitingConfrontation: return "AWAITING_CONFRONTATION";
        case SessionState::ReadyForReposition: return "READY_FOR_REPOSITION";
        case SessionState::CycleConsolidated: return "CYCLE_CONSOLIDATED";
        case SessionState::ArchivedVersioned: return "ARCHIVED_VERSIONED";
    }
    return "UNKNOWN";
}

struct ProvenancedText {
    std::string text{};
    ProvenanceOrigin origin{ProvenanceOrigin::Human};
    std::string author{}; // Nome do pesquisador ou ID do modelo
    std::string model_digest{}; // SHA-256 dos pesos se for ASSISTIDO
    std::string citation{}; // Fonte se for IMPORTADO/EVIDÊNCIA
    std::string timestamp_iso{};
};

} // namespace mirante::domain
