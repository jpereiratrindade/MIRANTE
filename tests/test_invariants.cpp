#include "mirante/domain/invariants.hpp"
#include "mirante/domain/entities.hpp"
#include "mirante/storage/database.hpp"
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <sqlite3.h>

#define MIRANTE_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << " [FALHA] " << __FILE__ << ":" << __LINE__ << " -> " << (msg) << std::endl; \
            std::abort(); \
        } \
    } while (false)

using namespace mirante::domain;
using namespace mirante::storage;

void TestInv01_PrematureCertainty() {
    std::cout << "[TESTE] INV-01: Proibição de Fato Demonstrado Prematuro (Domínio & Tipagem)..." << std::endl;
    
    // Status não-candidata é rejeitado pelo domínio
    auto res_bad = InvariantValidator::ValidateRelationStatus(static_cast<RelationStatus>(999));
    MIRANTE_CHECK(!res_bad.has_value(), "INV-01 deveria rejeitar status diferente de Candidate");
    MIRANTE_CHECK(res_bad.error().code == InvariantErrorCode::INV01_PrematureCertainty, "Código deve ser INV01_PrematureCertainty");

    // Status Candidata é o único aceito na Bancada
    auto res_good = InvariantValidator::ValidateRelationStatus(RelationStatus::Candidate);
    MIRANTE_CHECK(res_good.has_value(), "INV-01 deve aceitar status Candidate");
    std::cout << "  -> OK!" << std::endl;
}

void TestInv02_ConfrontationCompleteness() {
    std::cout << "[TESTE] INV-02: Obrigatoriedade do Contraditório no Reposicionamento..." << std::endl;
    // Sem rivais nem riscos
    auto res_bad = InvariantValidator::ValidateConfrontationCompleteness(0, 0);
    MIRANTE_CHECK(!res_bad.has_value(), "Deveria falhar sem rivais e riscos");
    MIRANTE_CHECK(res_bad.error().code == InvariantErrorCode::INV02_MissingConfrontationRequirements, "Código de erro incorreto");

    // Com 1 rival mas sem risco
    auto res_bad2 = InvariantValidator::ValidateConfrontationCompleteness(1, 0);
    MIRANTE_CHECK(!res_bad2.has_value(), "Deveria falhar sem riscos");

    // Com 0 rival mas com 1 risco
    auto res_bad3 = InvariantValidator::ValidateConfrontationCompleteness(0, 1);
    MIRANTE_CHECK(!res_bad3.has_value(), "Deveria falhar sem rivais");

    // Completo: 1 rival e 1 risco
    auto res_good = InvariantValidator::ValidateConfrontationCompleteness(1, 1);
    MIRANTE_CHECK(res_good.has_value(), "Deveria ser aceito com 1 rival e 1 risco");
    std::cout << "  -> OK!" << std::endl;
}

void TestInv04_ProvenanceTransparency() {
    std::cout << "[TESTE] INV-04: Transparência Categórica de Proveniência..." << std::endl;

    // Texto vazio
    ProvenancedText empty_text{.text = "", .origin = ProvenanceOrigin::Human, .author = "Pesquisador"};
    auto r1 = InvariantValidator::ValidateProvenance(empty_text);
    MIRANTE_CHECK(!r1.has_value(), "Texto vazio deve falhar");
    MIRANTE_CHECK(r1.error().code == InvariantErrorCode::INV04_AnonymousProvenance, "Código deve ser INV04_AnonymousProvenance");

    // Autor vazio
    ProvenancedText no_author{.text = "Mecanismo biofísico", .origin = ProvenanceOrigin::Human, .author = ""};
    auto r2 = InvariantValidator::ValidateProvenance(no_author);
    MIRANTE_CHECK(!r2.has_value(), "Autor anônimo deve falhar");

    // Assistida por IA sem digest do modelo
    ProvenancedText ai_no_digest{.text = "Proposta alternativa", .origin = ProvenanceOrigin::Assisted, .author = "qwen2.5:14b", .model_digest = ""};
    auto r3 = InvariantValidator::ValidateProvenance(ai_no_digest);
    MIRANTE_CHECK(!r3.has_value(), "IA sem digest SHA-256 deve falhar");

    // Humana válida
    ProvenancedText human_ok{.text = "Hipótese válida", .origin = ProvenanceOrigin::Human, .author = "José Pedro"};
    auto r4 = InvariantValidator::ValidateProvenance(human_ok);
    MIRANTE_CHECK(r4.has_value(), "Proveniência humana válida deve passar");

    // Assistida válida
    ProvenancedText ai_ok{.text = "Hipótese assistida", .origin = ProvenanceOrigin::Assisted, .author = "qwen2.5:14b", .model_digest = "sha256:e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"};
    auto r5 = InvariantValidator::ValidateProvenance(ai_ok);
    MIRANTE_CHECK(r5.has_value(), "Proveniência assistida com digest deve passar");
    std::cout << "  -> OK!" << std::endl;
}

void TestInv05_AutonomousCore() {
    std::cout << "[TESTE] INV-05: Autonomia Operacional do Núcleo (Sem IA obrigatória)..." << std::endl;

    // Constrói ciclo completo de pesquisa no agregado 100% humano
    SessionAggregate s;
    s.session_id = "SES-AUTONOMOUS-001";
    s.project_id = "PRJ-ECOL-01";
    s.title = "Dinâmica de Raízes em Bioma Pampa";
    s.phenomenon_description = {.text = "Acúmulo diferencial de biomassa radicular", .origin = ProvenanceOrigin::Human, .author = "Pesquisador"};

    ProvenancedText shift_rat{.text = "Deslocamento da escala fisiológica pontual para escala ecossistêmica plurianual", .origin = ProvenanceOrigin::Human, .author = "Pesquisador"};
    auto shift_res = s.SetPerspectiveShift("Fisiologia vegetal estrita", "Ecologia funcional de ecossistemas", ShiftDimension::SpatialScale, shift_rat);
    MIRANTE_CHECK(shift_res.has_value(), "Deslocamento puramente humano deve ser aceito sem IA");

    ProvenancedText rel_desc{.text = "Associação micorrízica potencializa retenção de carbono em horizontes profundos", .origin = ProvenanceOrigin::Human, .author = "Pesquisador"};
    auto rel_res = s.AddCandidateRelation("Micorrizas arbusculares", "Retenção de C no solo", RelationType::ConjecturedMechanism, rel_desc);
    MIRANTE_CHECK(rel_res.has_value(), "Relação candidata puramente humana deve ser aceita");

    ProvenancedText q_desc{.text = "Qual a taxa de turnover radicular em profundidades superiores a 1 metro sob pastejo contínuo?", .origin = ProvenanceOrigin::Human, .author = "Pesquisador"};
    auto q_res = s.AddScientificQuestion(s.relations[0].id, q_desc, "Minirhizotron em campo");
    MIRANTE_CHECK(q_res.has_value(), "Questão científica formulada por humano deve ser aceita");

    std::vector<ProvenancedText> rivals = {{.text = "Aporte radicular pode ser decorrente de decomposição bacteriana lenta por anaerobiose temporária", .origin = ProvenanceOrigin::Human, .author = "Revisor Humano"}};
    std::vector<ProvenancedText> risks = {{.text = "Incerteza na amostragem profunda pode introduzir artefato de compactação mecânica", .origin = ProvenanceOrigin::Human, .author = "Revisor Humano"}};
    auto conf_res = s.RecordConfrontation(rivals, risks, {});
    MIRANTE_CHECK(conf_res.has_value(), "Confronto crítico realizado por humanos deve ser aceito");

    ProvenancedText repo_just{.text = "A hipótese sobrevive aos contra-argumentos iniciais mas requer mensuração direta de fluxo de CO2 edáfico", .origin = ProvenanceOrigin::Human, .author = "Pesquisador"};
    auto repo_res = s.ConsolidateReposition(HypothesisStatus::Develop, repo_just, "Instalação de câmaras automáticas de respiração de solo", "Taxonomia metagenômica fúngica", "");
    MIRANTE_CHECK(repo_res.has_value(), "Reposicionamento puramente humano consolidado com sucesso");

    MIRANTE_CHECK(s.current_cycle == 2, "Ciclo deve avançar após reposicionamento");
    std::cout << "  -> OK! Núcleo do MIRANTE opera integralmente sem IA." << std::endl;
}

void TestInv06_UncertaintyConservation() {
    std::cout << "[TESTE] INV-06: Conservação de Incerteza (Suspensão exige Retomada)..." << std::endl;

    // Suspensão sem critérios de retomada -> ERRO
    auto res_bad = InvariantValidator::ValidateSuspensionCriteria(HypothesisStatus::Suspend, "");
    MIRANTE_CHECK(!res_bad.has_value(), "Suspensão sem retomada deve falhar");
    MIRANTE_CHECK(res_bad.error().code == InvariantErrorCode::INV06_AmbiguousSuspension, "Código deve ser INV06_AmbiguousSuspension");

    // Suspensão com critérios claros de retomada -> OK
    auto res_good = InvariantValidator::ValidateSuspensionCriteria(HypothesisStatus::Suspend, "Retomar quando houver espectrometria de massa de alta resolução disponível.");
    MIRANTE_CHECK(res_good.has_value(), "Suspensão com critério explícito de retomada deve ser aceita");

    // Outros status não exigem critérios de retomada
    auto res_dev = InvariantValidator::ValidateSuspensionCriteria(HypothesisStatus::Develop, "");
    MIRANTE_CHECK(res_dev.has_value(), "Desenvolver não exige critério de retomada");
    std::cout << "  -> OK!" << std::endl;
}

void TestInv07_PerspectiveShift() {
    std::cout << "[TESTE] INV-07: Rastreabilidade de Enquadramento no Deslocamento..." << std::endl;
    auto res_bad = InvariantValidator::ValidatePerspectiveShift("", "Perspectiva B", "");
    MIRANTE_CHECK(!res_bad.has_value(), "Deslocamento incompleto deve falhar");
    MIRANTE_CHECK(res_bad.error().code == InvariantErrorCode::INV07_IncompleteShift, "Código de erro incorreto");

    auto res_good = InvariantValidator::ValidatePerspectiveShift("Enquadramento A", "Perspectiva B", "Variar escala temporal decenal");
    MIRANTE_CHECK(res_good.has_value(), "Deslocamento completo deve passar");
    std::cout << "  -> OK!" << std::endl;
}

void TestInv08_RepositionNextStep() {
    std::cout << "[TESTE] INV-08: Obrigatoriedade de Próximo Passo Epistêmico..." << std::endl;
    auto res_bad = InvariantValidator::ValidateRepositionNextStep("", "");
    MIRANTE_CHECK(!res_bad.has_value(), "Reposicionamento em ponto morto deve falhar");
    MIRANTE_CHECK(res_bad.error().code == InvariantErrorCode::INV08_DeadEndReposition, "Código de erro incorreto");

    auto res_good = InvariantValidator::ValidateRepositionNextStep("Coleta de solo estratificada por pastagem", "");
    MIRANTE_CHECK(res_good.has_value(), "Reposicionamento com próximo teste deve passar");
    std::cout << "  -> OK!" << std::endl;
}

void TestInv03_MerkleLedgerAndAtomicTransactions() {
    std::cout << "[TESTE] INV-03: Merkle-Chain Audit Ledger e Transação Atômica..." << std::endl;
    std::string test_db = "test_audit.sqlite3";
    if (std::filesystem::exists(test_db)) std::filesystem::remove(test_db);

    Database db;
    auto op = db.Open(test_db);
    MIRANTE_CHECK(op.has_value(), "Abertura de SQLite falhou");

    // Cria sessão com evento atômico
    SessionAggregate s;
    s.session_id = "SES-TEST-001";
    s.project_id = "PRJ-001";
    s.title = "Teste de Integridade Criptográfica";
    s.phenomenon_description = {.text = "Fenômeno de Teste", .author = "Pesquisador"};

    auto tx_res1 = db.SaveSessionWithEvent(s, "SESSION_CREATED", "{\"title\": \"Teste\"}", "Pesquisador");
    MIRANTE_CHECK(tx_res1.has_value(), "Transação atômica 1 falhou");
    MIRANTE_CHECK(tx_res1->length() == 64, "Merkle hash SHA-256 deve ter 64 caracteres");

    // Adiciona evento via transação atômica
    s.current_cycle = 2;
    auto tx_res2 = db.SaveSessionWithEvent(s, "M1_PERSPECTIVE_SHIFT", "{\"shift\": \"temporal\"}", "Pesquisador");
    MIRANTE_CHECK(tx_res2.has_value(), "Transação atômica 2 falhou");
    MIRANTE_CHECK(*tx_res1 != *tx_res2, "Hashes encadeados subsequentes devem ser distintos");

    // Adiciona evento avulso
    auto ev3 = db.AppendEvent("SES-TEST-001", "C4_CONFRONT", "{\"rivals\": 1}", "Revisor");
    MIRANTE_CHECK(ev3.has_value(), "AppendEvent falhou");

    // Verifica integridade inicial
    auto verify_op = db.VerifyLedgerIntegrity("SES-TEST-001");
    MIRANTE_CHECK(verify_op.has_value(), "Verificação de integridade falhou");
    MIRANTE_CHECK(*verify_op == true, "Cadeia Merkle legítima deve ser íntegra");

    // Simula adulteração maliciosa direta no SQLite (quebra de integridade INV-03)
    sqlite3* raw_db = nullptr;
    sqlite3_open(test_db.c_str(), &raw_db);
    sqlite3_exec(raw_db, "UPDATE mprr_session_events SET payload_json = '{\"tampered\": true}' WHERE sequence_number = 2;", nullptr, nullptr, nullptr);
    sqlite3_close(raw_db);

    // Reabre e verifica que a violação é detectada infalivelmente
    auto verify_tampered = db.VerifyLedgerIntegrity("SES-TEST-001");
    MIRANTE_CHECK(verify_tampered.has_value(), "Verificação pós-adulteração deve retornar resultado");
    MIRANTE_CHECK(*verify_tampered == false, "Violação de adulteração na cadeia Merkle DEVE ser detectada como falsa!");

    db.Close();
    if (std::filesystem::exists(test_db)) std::filesystem::remove(test_db);
    std::cout << "  -> OK! Cadeia Merkle validada e adulteração detectada com sucesso." << std::endl;
}

void TestInv01_ReferentialIntegrity() {
    std::cout << "[TESTE] INV-01: Integridade Referencial Estrita (Relação Obrigatória e Existente)..." << std::endl;
    SessionAggregate s;
    s.session_id = "SES-REF-001";
    s.project_id = "PRJ-BIO";
    s.title = "Teste de Vínculo Referencial";
    s.phenomenon_description = {.text = "Fixação biológica de N2", .author = "Pesquisador"};

    // 1. Tentar adicionar questão sem nenhuma relação cadastrada
    ProvenancedText q1{.text = "Como a nodulação varia com o pH?", .origin = ProvenanceOrigin::Human, .author = "Pesquisador"};
    auto r1 = s.AddScientificQuestion("REL-001", q1, "Ensaio em vaso");
    MIRANTE_CHECK(!r1.has_value(), "Adicionar questão com relação inexistente DEVE falhar");
    MIRANTE_CHECK(r1.error().code == InvariantErrorCode::INV01_PrematureCertainty, "Erro deve ser INV01_PrematureCertainty");

    // 2. Tentar adicionar questão com relation_id vazio
    auto r2 = s.AddScientificQuestion("", q1, "Ensaio em vaso");
    MIRANTE_CHECK(!r2.has_value(), "Adicionar questão com relation_id vazio DEVE falhar");

    // 3. Cadastrar relação candidata legítima
    ProvenancedText rdesc{.text = "Rhizobium em simbiose com raiz", .origin = ProvenanceOrigin::Human, .author = "Pesquisador"};
    auto rel_res = s.AddCandidateRelation("Rhizobium", "Massa nodular", RelationType::ConjecturedMechanism, rdesc);
    MIRANTE_CHECK(rel_res.has_value(), "Cadastro de relação candidata deve ter sucesso");
    std::string valid_rel_id = s.relations.back().id;

    // 4. Tentar com ID fantasma (ex: o antigo default REL-001) quando o ID real é REL-SES-REF-001-1
    auto r3 = s.AddScientificQuestion("REL-001", q1, "Ensaio em vaso");
    MIRANTE_CHECK(!r3.has_value(), "ID fantasma 'REL-001' deve ser categoricamente rejeitado");

    // 5. Vincular com o ID real existente
    auto r4 = s.AddScientificQuestion(valid_rel_id, q1, "Ensaio em vaso");
    MIRANTE_CHECK(r4.has_value(), "Adicionar questão vinculada a relation_id real deve ter sucesso");
    MIRANTE_CHECK(s.questions[0].relation_id == valid_rel_id, "relation_id armazenado deve bater exatamente com a relação candidata");

    std::cout << "  -> OK! Relações inexistentes rejeitadas e vínculo referencial verificado." << std::endl;
}

void TestMultiCycleRepositionPersistence() {
    std::cout << "[TESTE] Epistemológico: Persistência e Reconstituição de Reposicionamentos Multiciclo..." << std::endl;
    std::string test_db = "test_persistence.sqlite3";
    if (std::filesystem::exists(test_db)) std::filesystem::remove(test_db);

    Database db;
    auto op = db.Open(test_db);
    MIRANTE_CHECK(op.has_value(), "Falha ao abrir SQLite de teste");

    SessionAggregate s;
    s.session_id = "SES-CYCLE-TEST";
    s.project_id = "PRJ-ECOL";
    s.title = "Sessão Multiciclo";
    s.phenomenon_description = {.text = "Fenômeno multiciclo", .author = "Pesquisador"};

    ProvenancedText s_rat{.text = "Mudança de escala", .author = "Pesquisador"};
    auto shift_ok = s.SetPerspectiveShift("Escala micro", "Escala macro", ShiftDimension::SpatialScale, s_rat);
    MIRANTE_CHECK(shift_ok.has_value(), "Falha ao definir deslocamento");

    ProvenancedText r_desc{.text = "Relação candidata A-B", .author = "Pesquisador"};
    auto rel_ok = s.AddCandidateRelation("Termo A", "Termo B", RelationType::ConjecturedMechanism, r_desc);
    MIRANTE_CHECK(rel_ok.has_value(), "Falha ao adicionar relação candidata");
    std::string test_rel_id = s.relations.back().id;

    ProvenancedText q_desc{.text = "Questão investigável 1", .author = "Pesquisador"};
    auto q_ok = s.AddScientificQuestion(test_rel_id, q_desc, "Lab test");
    MIRANTE_CHECK(q_ok.has_value(), "Falha ao adicionar questão investigável");

    // Ciclo 1 -> Confronto e Reposicionamento DEVELOP
    std::vector<ProvenancedText> rivals1 = {{.text = "Rival 1", .author = "Revisor"}};
    std::vector<ProvenancedText> risks1 = {{.text = "Risco 1", .author = "Revisor"}};
    auto conf1_ok = s.RecordConfrontation(rivals1, risks1, {});
    MIRANTE_CHECK(conf1_ok.has_value(), "Falha no confronto ciclo 1");

    ProvenancedText rep_just1{.text = "Decisão de desenvolver no ciclo 1", .author = "Pesquisador"};
    auto repo1_ok = s.ConsolidateReposition(HypothesisStatus::Develop, rep_just1, "Próximo ensaio 1", "Novo dado 1", "");
    MIRANTE_CHECK(repo1_ok.has_value(), "Falha na consolidação ciclo 1");
    MIRANTE_CHECK(s.current_cycle == 2, "Ciclo deve avançar para 2");

    // Ciclo 2 -> Confronto e Reposicionamento REFORMULATE
    std::vector<ProvenancedText> rivals2 = {{.text = "Rival 2", .author = "Revisor"}};
    std::vector<ProvenancedText> risks2 = {{.text = "Risco 2", .author = "Revisor"}};
    auto conf2_ok = s.RecordConfrontation(rivals2, risks2, {});
    MIRANTE_CHECK(conf2_ok.has_value(), "Falha no confronto ciclo 2");

    ProvenancedText rep_just2{.text = "Decisão de reenquadrar no ciclo 2", .author = "Pesquisador"};
    auto repo2_ok = s.ConsolidateReposition(HypothesisStatus::Reformulate, rep_just2, "Próximo ensaio 2", "Novo dado 2", "");
    MIRANTE_CHECK(repo2_ok.has_value(), "Falha na consolidação ciclo 2");
    MIRANTE_CHECK(s.current_cycle == 3, "Ciclo deve avançar para 3");

    // Salva no banco de dados com evento atômico
    auto save_res = db.SaveSessionWithEvent(s, "C5_REPOSITION_CYCLE_2", "{\"cycle\": 2}", "Pesquisador");
    MIRANTE_CHECK(save_res.has_value(), "Falha ao salvar sessão multiciclo no banco");

    // Recarrega do banco
    auto loaded_opt = db.LoadSession("SES-CYCLE-TEST");
    MIRANTE_CHECK(loaded_opt.has_value(), "Falha ao recarregar sessão multiciclo");
    const auto& loaded = *loaded_opt;

    MIRANTE_CHECK(loaded.current_cycle == 3, "Ciclo recarregado deve ser 3");
    MIRANTE_CHECK(loaded.reposition_history.size() == 2, "Histórico recarregado deve conter exatamente 2 reposicionamentos");
    MIRANTE_CHECK(loaded.reposition_history[0].decision == HypothesisStatus::Develop, "Primeiro reposicionamento deve ser DEVELOP");
    MIRANTE_CHECK(loaded.reposition_history[0].verdict_justification.text == "Decisão de desenvolver no ciclo 1", "Justificativa do ciclo 1 deve ser preservada");
    MIRANTE_CHECK(loaded.reposition_history[1].decision == HypothesisStatus::Reformulate, "Segundo reposicionamento deve ser REFORMULATE");
    MIRANTE_CHECK(loaded.reposition_history[1].verdict_justification.text == "Decisão de reenquadrar no ciclo 2", "Justificativa do ciclo 2 deve ser preservada");

    db.Close();
    if (std::filesystem::exists(test_db)) std::filesystem::remove(test_db);
    std::cout << "  -> OK! Trajetória multiciclo preservada integralmente através do storage." << std::endl;
}

int main() {
    std::cout << "=========================================================" << std::endl;
    std::cout << "  MIRANTE — BATERIA DE TESTES DE INVARIANTES DO PROTOCOLO" << std::endl;
    std::cout << "=========================================================" << std::endl;
    TestInv01_PrematureCertainty();
    TestInv01_ReferentialIntegrity();
    TestInv02_ConfrontationCompleteness();
    TestInv04_ProvenanceTransparency();
    TestInv05_AutonomousCore();
    TestInv06_UncertaintyConservation();
    TestInv07_PerspectiveShift();
    TestInv08_RepositionNextStep();
    TestInv03_MerkleLedgerAndAtomicTransactions();
    TestMultiCycleRepositionPersistence();
    std::cout << "=========================================================" << std::endl;
    std::cout << "  TODOS OS INVARIANTES E TESTES ADVERSARIAIS PASSARAM!   " << std::endl;
    std::cout << "=========================================================" << std::endl;
    return 0;
}

