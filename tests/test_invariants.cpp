#include "mirante/domain/invariants.hpp"
#include "mirante/domain/entities.hpp"
#include "mirante/storage/database.hpp"
#include <iostream>
#include <cassert>
#include <filesystem>

using namespace mirante::domain;
using namespace mirante::storage;

void TestInv01_PrematureCertainty() {
    std::cout << "[TESTE] INV-01: Proibição de Fato Demonstrado Prematuro..." << std::endl;
    auto res_bad = InvariantValidator::ValidateRelationStatus(RelationStatus::Candidate, "Esta relação está comprovada e é um fato incontestável.");
    assert(!res_bad.has_value());
    assert(res_bad.error().code == InvariantErrorCode::INV01_PrematureCertainty);

    auto res_good = InvariantValidator::ValidateRelationStatus(RelationStatus::Candidate, "Sugere-se um mecanismo biológico mediador entre solo e raiz.");
    assert(res_good.has_value());
    std::cout << "  -> OK!" << std::endl;
}

void TestInv02_ConfrontationCompleteness() {
    std::cout << "[TESTE] INV-02: Obrigatoriedade do Contraditório..." << std::endl;
    // Sem rivais nem riscos
    auto res_bad = InvariantValidator::ValidateConfrontationCompleteness(0, 0);
    assert(!res_bad.has_value());
    assert(res_bad.error().code == InvariantErrorCode::INV02_MissingConfrontationRequirements);

    // Com 1 rival mas sem risco
    auto res_bad2 = InvariantValidator::ValidateConfrontationCompleteness(1, 0);
    assert(!res_bad2.has_value());

    // Completo: 1 rival e 1 risco
    auto res_good = InvariantValidator::ValidateConfrontationCompleteness(1, 1);
    assert(res_good.has_value());
    std::cout << "  -> OK!" << std::endl;
}

void TestInv07_PerspectiveShift() {
    std::cout << "[TESTE] INV-07: Rastreabilidade de Enquadramento no Deslocamento..." << std::endl;
    auto res_bad = InvariantValidator::ValidatePerspectiveShift("", "Perspectiva B", "");
    assert(!res_bad.has_value());
    assert(res_bad.error().code == InvariantErrorCode::INV07_IncompleteShift);

    auto res_good = InvariantValidator::ValidatePerspectiveShift("Enquadramento A", "Perspectiva B", "Variar escala temporal decenal");
    assert(res_good.has_value());
    std::cout << "  -> OK!" << std::endl;
}

void TestInv08_RepositionNextStep() {
    std::cout << "[TESTE] INV-08: Obrigatoriedade de Próximo Passo Epistêmico..." << std::endl;
    auto res_bad = InvariantValidator::ValidateRepositionNextStep("", "");
    assert(!res_bad.has_value());
    assert(res_bad.error().code == InvariantErrorCode::INV08_DeadEndReposition);

    auto res_good = InvariantValidator::ValidateRepositionNextStep("Coleta de solo estratificada por pastagem", "");
    assert(res_good.has_value());
    std::cout << "  -> OK!" << std::endl;
}

void TestMerkleLedgerIntegrity() {
    std::cout << "[TESTE] INV-03: Merkle-Chain Audit Ledger..." << std::endl;
    std::string test_db = "test_audit.sqlite3";
    if (std::filesystem::exists(test_db)) std::filesystem::remove(test_db);

    Database db;
    auto op = db.Open(test_db);
    assert(op.has_value());

    // Cria sessão
    SessionAggregate s;
    s.session_id = "SES-TEST-001";
    s.project_id = "PRJ-001";
    s.title = "Teste de Integridade";
    s.phenomenon_description = {.text = "Fenômeno Teste", .author = "Pesquisador"};
    auto save_op = db.SaveSession(s);
    assert(save_op.has_value());

    // Grava 3 eventos
    auto ev1 = db.AppendEvent("SES-TEST-001", "SESSION_CREATED", "{}", "Pesquisador");
    assert(ev1.has_value());
    auto ev2 = db.AppendEvent("SES-TEST-001", "M1_SHIFT", "{\"dim\": \"time\"}", "Pesquisador");
    assert(ev2.has_value());
    auto ev3 = db.AppendEvent("SES-TEST-001", "C4_CONFRONT", "{\"rivals\": 1}", "Revisor");
    assert(ev3.has_value());

    // Verifica integridade
    auto verify_op = db.VerifyLedgerIntegrity("SES-TEST-001");
    assert(verify_op.has_value());
    assert(*verify_op == true);

    db.Close();
    if (std::filesystem::exists(test_db)) std::filesystem::remove(test_db);
    std::cout << "  -> OK! Cadeia Merkle validada." << std::endl;
}

int main() {
    std::cout << "=== EXECUTANDO TESTES DE INVARIANTES DO MIRANTE ===" << std::endl;
    TestInv01_PrematureCertainty();
    TestInv02_ConfrontationCompleteness();
    TestInv07_PerspectiveShift();
    TestInv08_RepositionNextStep();
    TestMerkleLedgerIntegrity();
    std::cout << "=== TODOS OS TESTES PASSARAM COM SUCESSO! ===" << std::endl;
    return 0;
}
