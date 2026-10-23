#!/bin/bash
set -e

HOST="http://127.0.0.1:8088"
echo "=== INICIANDO TESTE DE CONSUMO END-TO-END DO MIRANTE ==="
echo "Servidor alvo: $HOST"

# 1. Health & Invariants Check
echo -e "\n--- [PASSO 1] Verificação de Saúde e Invariantes Normativos ---"
curl -s -f "$HOST/api/health" | jq .
curl -s -f "$HOST/api/invariants" | jq '.[0:3]'

# 2. Criar Nova Sessão via Porta Caminho B
echo -e "\n--- [PASSO 2] Criando Sessão Científica MPRR (Caminho B) ---"
CREATE_PAYLOAD='{
  "session_id": "SES-E2E-BIO-2026",
  "project_id": "PRJ-PAMPA-BIODIV",
  "title": "Associação Micorrízica e Resiliência Hídrica em Pastagens Naturais",
  "entrance_door": "PATH_B_PHENOMENON_TO_KNOWLEDGE",
  "phenomenon_text": "Observa-se manutenção de turgescência foliar em gramíneas nativas durante estiagens prolongadas (45+ dias sem precipitação) exclusivamente em piquetes sob pastagem rotativa com oferta controlada de forragem, sem qualquer irrigação suplementar.",
  "spatial_boundary": "Bioma Pampa, bacia do Rio Santa Maria (RS, Brasil)",
  "temporal_boundary": "Séries temporais climáticas 2018-2026",
  "research_context": "Ecologia Funcional e Física do Solo do Labeco",
  "author": "Dr. Pesquisador Labeco"
}'

SESSION_RES=$(curl -s -X POST "$HOST/api/sessions" \
  -H "Content-Type: application/json" \
  -d "$CREATE_PAYLOAD")

echo "$SESSION_RES" | jq .
SID=$(echo "$SESSION_RES" | jq -r .session_id)
GENESIS_HASH=$(echo "$SESSION_RES" | jq -r .merkle_hash)
echo "Sessão criada: $SID (Hash Merkle Gênesis: $GENESIS_HASH)"

# 3. Movimento 1: Registrar Deslocamento de Perspectiva (M1)
echo -e "\n--- [PASSO 3] M1: Deslocamento de Perspectiva (INV-07) ---"
SHIFT_PAYLOAD='{
  "initial_framework": "Abordagem Agronômica Clássica de Extração: foca apenas na disponibilidade de água gravitacional e evapotranspiração do dossel aéreo imediato",
  "shifted_framework": "Abordagem Ecofisiológica Integrada Solo-Rizosfera: desloca o olhar para a malha micorrízica profunda como rede hidráulica redistributiva contínua",
  "dimension": "SPATIAL_SCALE",
  "rationale": "A escala agronômica tradicional ignora o volume edáfico abaixo de 40 cm e a colonização por fungos micorrízicos arbusculares na manutenção do potencial hídrico foliar noturno.",
  "author": "Dr. Pesquisador Labeco",
  "is_assisted": false
}'

SHIFT_RES=$(curl -s -X POST "$HOST/api/sessions/$SID/shift" \
  -H "Content-Type: application/json" \
  -d "$SHIFT_PAYLOAD")
echo "$SHIFT_RES" | jq .

# 4. Movimento 2: Formular Relação Candidata (M2)
echo -e "\n--- [PASSO 4] M2: Formular Relação Candidata (INV-01) ---"
RELATION_PAYLOAD='{
  "term_a": "Colonização micorrízica em horizontes profundos (>60cm)",
  "term_b": "Redistribuição hidráulica vertical e retenção de água rizosférica em estiagens",
  "type": "CONJECTURED_MECHANISM",
  "description": "Conjectura-se que a densidade de hifas extrarradiculares em camadas subsuperficiais funciona como conduto para transferência hídrica inversa durante a noite, mantendo o microbioma rizosférico e a viabilidade dos meristemas basais.",
  "author": "Dr. Pesquisador Labeco",
  "is_assisted": false
}'

REL_RES=$(curl -s -X POST "$HOST/api/sessions/$SID/relation" \
  -H "Content-Type: application/json" \
  -d "$RELATION_PAYLOAD")
echo "$REL_RES" | jq .

# 5. Movimento 3: Problematização e Questão Científica (M3)
echo -e "\n--- [PASSO 5] M3: Problematizar e Construir Questão Investigável ---"
QUESTION_PAYLOAD='{
  "relation_id": "REL-001",
  "question_text": "Qual a magnitude da contribuição hídrica transportada pela via micorrízica versus capilaridade da matriz edáfica em horizontes profundos sob déficit hídrico severo?",
  "evidence_path": "Instalação de psicrômetros de haste in situ e marcação com água deuterada (2H2O) injetada a 80 cm de profundidade com rastreamento no xilema ao amanhecer.",
  "author": "Dr. Pesquisador Labeco",
  "is_assisted": false
}'

Q_RES=$(curl -s -X POST "$HOST/api/sessions/$SID/question" \
  -H "Content-Type: application/json" \
  -d "$QUESTION_PAYLOAD")
echo "$Q_RES" | jq .

# 6. Testar Barreira do Invariante INV-02 no Confronto (Tentativa com dados incompletos)
echo -e "\n--- [PASSO 6] Testando Barreira INV-02: Tentativa de reposicionar sem contraditório prévio ---"
BAD_REPO_PAYLOAD='{
  "decision": "DEVELOP",
  "justification": "Queremos pular o confronto crítico e aprovar direto.",
  "next_empirical_test": "Teste rápido"
}'
BAD_REPO_RES=$(curl -s -w "\nHTTP_STATUS:%{http_code}" -X POST "$HOST/api/sessions/$SID/reposition" \
  -H "Content-Type: application/json" \
  -d "$BAD_REPO_PAYLOAD")
echo "$BAD_REPO_RES"

# 7. Controle Crítico C4: Conduzir e Registrar Confronto Pleno
echo -e "\n--- [PASSO 7] C4: Arena de Confronto Crítico Pleno (INV-02) ---"
CONFRONT_PAYLOAD='{
  "rivals": [
    {
      "text": "Explicação Rival 1: A manutenção da turgescência pode decorrer de acúmulo passivo de solutos osmóticos (prolina/glicina-betaína) induzido pelo estresse anterior, sem participação líquida de fluxo hídrico via hifas.",
      "author": "Dra. Revisora Crítica (Fisiologia)"
    },
    {
      "text": "Explicação Rival 2: O sombreamento conferido pela maior biomassa residual do pastoreio rotativo reduziu a temperatura da superfície do solo, retardando a taxa de evaporação física direta.",
      "author": "Dr. Revisor Crítico (Micrometeorologia)"
    }
  ],
  "risks": [
    {
      "text": "Risco Epistêmico 1: Artefato metodológico na injeção de 2H2O com perturbação da estrutura dos microporos do solo ou contaminação por ascensão capilar do vapor dágua.",
      "author": "Dra. Revisora Crítica (Metodologia)"
    }
  ]
}'

CONF_RES=$(curl -s -X POST "$HOST/api/sessions/$SID/confront" \
  -H "Content-Type: application/json" \
  -d "$CONFRONT_PAYLOAD")
echo "$CONF_RES" | jq .

# 8. Controle Crítico C5: Consolidar Reposicionamento Epistêmico
echo -e "\n--- [PASSO 8] C5: Consolidar Reposicionamento (INV-08 & INV-06) ---"
REPOSITION_PAYLOAD='{
  "decision": "DEVELOP",
  "justification": "A hipótese de redistribuição micorrízica se sustenta como mecanismo prioritário porque as taxas de transpiração medidas superam a capacidade de amortecimento puramente osmótico, mas o desenho experimental precisa desacoplar o efeito de microclima térmico através de parcelas com cobertura morta artificial.",
  "next_empirical_test": "Experimento bifatorial de campo: Injeção isotópica 2H2O cruzada com aplicação localizada de fungicida sistêmico (benomil) para inibição seletiva de micorrizas em blocos casualizados.",
  "required_knowledge": "Dinâmica da biofísica de transporte em hifas e espectroscopia de aniquilação de pósitrons ou RMN de baixo campo para solos intactos.",
  "author": "Dr. Pesquisador Labeco"
}'

REPO_RES=$(curl -s -X POST "$HOST/api/sessions/$SID/reposition" \
  -H "Content-Type: application/json" \
  -d "$REPOSITION_PAYLOAD")
echo "$REPO_RES" | jq .

# 9. Inspeção da Sessão Completa
echo -e "\n--- [PASSO 9] Inspeção do Estado Consolidado da Sessão ---"
SESSION_DETAIL=$(curl -s "$HOST/api/sessions/$SID")
echo "$SESSION_DETAIL" | jq '{
  session_id: .session_id,
  current_cycle: .current_cycle,
  entrance_door: .entrance_door,
  shift_initial: .shift.initial,
  shift_shifted: .shift.shifted,
  relations_count: (.relations | length),
  questions_count: (.questions | length),
  confrontation_done: .confrontation_done,
  rivals_count: (.confrontation.rivals | length),
  risks_count: (.confrontation.risks | length)
}'

# 10. Auditoria Criptográfica Merkle-Chain (INV-03)
echo -e "\n--- [PASSO 10] Auditoria da Cadeia Merkle SHA-256 (INV-03) ---"
AUDIT_RES=$(curl -s "$HOST/api/sessions/$SID/audit")
echo "$AUDIT_RES" | jq '{
  session_id: .session_id,
  is_valid: .is_valid,
  events_count: .events_count,
  head_hash: .head_hash,
  event_types: [.events[].event_type]
}'

echo -e "\n=== TESTE DE CONSUMO END-TO-END CONCLUÍDO COM SUCESSO! ==="
