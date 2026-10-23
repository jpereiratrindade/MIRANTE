# Especificação MPRR-INV-001
## Ecossistema SisTer — MIRANTE
### Invariantes Metodológicos, Contratos de Domínio e Modelo de Integridade

```text
Identificador:   MPRR-INV-001
Versão:          0.1.0
Data de Edição:  23 de outubro de 2026
Status:          Normativo / Contratos Inegociáveis de Domínio e Epistemologia
Protocolo Base:  MPRR-FOUNDATION-001 @ v0.1.0
Implementação:   mirante::domain (C++26)
```

> **Assinatura:**  
> **MIRANTE**  
> *Deslocar perspectivas. Reconhecer relações. Construir questões.*  
>  
> **Máxima Arquitetural:**  
> *"Automatizar a exploração. Preservar a dúvida. Registrar o percurso. Submeter a descoberta à crítica."*

---

## 1. Definição de Invariante de Domínio

No MIRANTE, um **Invariante de Domínio** é uma regra conceitual, lógica ou metodológica derivada diretamente do protocolo MPRR que **não pode ser violada** por nenhuma camada do sistema:
- O banco de dados rejeita persistências que violem o invariante;
- A API HTTP retorna erro `422 Unprocessable Entity` se uma transação tentar quebrar a regra;
- A interface gráfica oculta ou desabilita ações que induzam à violação;
- Nenhum assistente de inteligência artificial tem privilégios para suspender ou relaxar os invariantes.

---

## 2. O Grande Contrato: Integridade Procedimental vs. Validação Científica

O MIRANTE formaliza uma separação categórica que deve constar em todos os relatórios, metadados e telas da aplicação:

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                       CONTRATO FUNDAMENTAL DO SISTEMA                       │
├─────────────────────────────────────────┬───────────────────────────────────┤
│    INTEGRIDADE PROCEDIMENTAL            │     VALIDAÇÃO CIENTÍFICA          │
│    (Jurisdição do Software MIRANTE)    │     (Jurisdição da Ciência Real)  │
├─────────────────────────────────────────┼───────────────────────────────────┤
│ • Atesta que o percurso seguiu o        │ • Atesta a verdade fática ou a    │
│   protocolo MPRR fielmente.             │   precisão empírica da hipótese.  │
│ • Garante que houve deslocamento        │ • Depende de experimentos, coletas│
│   explícito documentado.                │   de campo e dados materiais.     │
│ • Garante que houve confronto           │ • Requer escrutínio por pares na   │
│   obrigatório com explicações rivais.   │   comunidade científica.          │
│ • Garante proveniência auditável        │ • NENHUM SOFTWARE É CAPAZ DE      │
│   com hashes criptográficos SHA-256.    │   EMITIR OU GARANTIR ESSA ATESTAÇÃO│
└─────────────────────────────────────────┴───────────────────────────────────┘
```

> **Cláusula de Garantia**: O MIRANTE nunca assina ou rotula uma hipótese como "cientificamente provada pelo sistema". O selo do software certifica exclusivamente: *"Sessão executada em estrita conformidade com a Integridade Procedimental do Protocolo MPRR v0.1.0"*.

---

## 3. Catálogo dos Oito Invariantes Inegociáveis (INV-01 a INV-08)

### INV-01: Proibição de Fato Demonstrado Prematuro
- **Enunciado**: Nenhuma relação formulada nos movimentos M1 (Deslocar), M2 (Relacionar) ou M3 (Problematizar) pode ser gravada ou apresentada com o estado de fato demonstrado ou verdade causal comprovada.
- **Implementação no Domínio**: A entidade `CandidateRelation` tem tipo de união estrita e nasce compulsoriamente com o atributo `status = RelationStatus::Candidate`. Não existe na máquina de estados a transição para `Demonstrated` dentro da bancada de descoberta.
- **Erro de Violação**: `MIRANTE_ERR_PREMATURE_CERTAINTY (0x1001)`.

### INV-02: Obrigatoriedade do Contraditório no Reposicionamento
- **Enunciado**: Nenhuma sessão pode concluir um ciclo analítico ou receber uma decisão de reposicionamento (Controle 5) sem que a Arena de Confronto (Controle 4) contenha:
  1. No mínimo **uma Explicação Rival plausível** registrada;
  2. No mínimo **um Risco Epistêmico de Invalidação** registrado.
- **Implementação no Domínio**: O método `SessionAggregate::ConsolidateCycle(RepositionDecision decision)` avalia:
  ```cpp
  if (confrontation.rival_explanations.empty() || 
      confrontation.epistemic_risks.empty()) {
      return std::unexpected(DomainError::MissingConfrontationRequirements);
  }
  ```
- **Erro de Violação**: `MIRANTE_ERR_UNCONFRONTED_HYPOTHESIS (0x1002)`.

### INV-03: Não-Sobrescrita Silenciosa (Append-Only Audit Ledger)
- **Enunciado**: Qualquer alteração em um campo de uma sessão consolidada, qualquer revisão de formulação e qualquer feedback do usuário gera um novo evento imutável com carimbo de data/hora e hash do evento anterior. O histórico nunca é apagado ou truncado.
- **Implementação no Domínio**: A persistência é orientada a *Event Sourcing*. A tabela `mprr_session_events` aceita apenas operações `INSERT`.
- **Erro de Violação**: `MIRANTE_ERR_MUTATION_FORBIDDEN (0x1003)`.

### INV-04: Transparência Categórica de Proveniência
- **Enunciado**: Toda contribuição de texto, ideia ou contraponto deve possuir exatamente uma atribuição de origem primária:
  - `HUMANO` (pesquisador identificado);
  - `ASSISTIDO` (modelo e versão registrados, com aprovação humana explícita);
  - `IMPORTADO` (fonte externa citada);
  - `EVIDÊNCIA` (dado empírico referenciado);
  - `INFERÊNCIA` (dedução coletiva em sessão).
- **Implementação no Domínio**: Nenhum campo de texto livre pode ser instanciado sem o invólucro de domínio `ProvenancedText { std::string content; ProvenanceOrigin origin; AgentId author; Timestamp ts; }`.
- **Erro de Violação**: `MIRANTE_ERR_ANONYMOUS_PROVENANCE (0x1004)`.

### INV-05: Autonomia Operacional do Núcleo sem Inteligência Artificial
- **Enunciado**: O núcleo de domínio, os serviços de aplicação, a camada de persistência e a interface gráfica do MIRANTE devem funcionar plenamente sem conexão à internet e sem qualquer modelo de IA ativo.
- **Implementação no Domínio**: A interface `IAssistProvider` é injetada opcionalmente como ponte nula (`NullAssistProvider`). As rotas principais da aplicação não chamam o provedor de assistência para validar ou avançar o estado das sessões.
- **Erro de Violação**: `MIRANTE_ERR_AI_COUPLING_DETECTED (0x1005)`.

### INV-06: Conservação de Incerteza e Dúvida Científica
- **Enunciado**: Uma hipótese que enfrente dados inconclusivos ou divergência durante o confronto não pode ser auto-descartada como nula nem promovida arbitrariamente; seu estado mandatório é `SUSPENSA`, com declaração explícita da incerteza pendente.
- **Implementação no Domínio**: O reposicionamento do tipo `SUSPENDER` exige o preenchimento de `condicoes_para_retomada`.
- **Erro de Violação**: `MIRANTE_ERR_AMBIGUOUS_SUSPENSION (0x1006)`.

### INV-07: Rastreabilidade Obrigatória de Enquadramento no Deslocamento
- **Enunciado**: Um deslocamento de perspectiva (M1) é inválido se não declarar simultaneamente:
  1. A perspectiva inicial de referência;
  2. A perspectiva deslocada resultante;
  3. A dimensão precisa que foi alterada (escala, tempo, nível, ator, etc.);
  4. A justificativa metodológica da alteração.
- **Implementação no Domínio**: Validação estrita no construtor da entidade `PerspectiveShift`.
- **Erro de Violação**: `MIRANTE_ERR_INCOMPLETE_SHIFT (0x1007)`.

### INV-08: Obrigatoriedade de Próximo Passo Epistêmico
- **Enunciado**: Não existe reposicionamento sem ação futura de investigação. Ao encerrar um ciclo em C5, é mandatório registrar o próximo teste empírico ou o próximo corpo de conhecimento a ser buscado.
- **Implementação no Domínio**: Validação no agregado de reposicionamento exigindo `next_empirical_test` ou `required_knowledge_domain`.
- **Erro de Violação**: `MIRANTE_ERR_DEAD_END_REPOSITION (0x1008)`.

---

## 4. Máquina de Estados da Sessão e da Hipótese

```text
[ ESTADOS DA SESSÃO ]

  (Início)
     │
     ▼
 [ RASCUNHO_INICIAL ]
     │  Preenchimento da Ficha e Porta (A ou B)
     ▼
 [ EM_EXPLORACAO ] ◄──────────────┐
     │                             │
     │  M1 + M2 + M3 consolidados  │ Reabertura para
     ▼                             │ revisão (bifurcação)
 [ AGUARDANDO_CONFRONTO ]          │
     │                             │
     │  C4 preenchido (Rival+Risco)│
     ▼                             │
 [ PRONTA_REPOSICIONAMENTO ]       │
     │                             │
     │  C5 deliberado              │
     ▼                             │
 [ CICLO_CONSOLIDADO ] ────────────┘
     │
     ▼  (Exportação de Dossiê e Selagem Criptográfica)
 [ ARQUIVADA_VERSIONADA ]
```

```text
[ ESTADOS DA HIPÓTESE ]

                [ NOVA_CANDIDATA ]
                        │
                        ▼
                  [ EM_ANÁLISE ]
                        │
      ┌─────────────────┼─────────────────┐
      ▼                 ▼                 ▼
[ DESENVOLVER ]   [ REFORMULAR ]    [ SUSPENDER ]
      │                 │                 │
      ▼                 ▼                 ▼
(Próximo teste)   (Gera Ciclo N+1)  (Preserva Dúvida)
      │
      ├───────────────────────────────────┐
      ▼                                   ▼
[ SUSTENTADA_POR_EVIDÊNCIAS ]       [ REJEITADA ]
(Com suporte empírico auditado)     (Refutada no confronto)
```

---

## 5. Estrutura Criptográfica do Event Ledger (Merkle-Chain)

Cada evento da sessão no MIRANTE possui a seguinte estrutura de integridade:

$$\text{Hash}_n = \text{SHA-256}\Big(\text{Hash}_{n-1} \,\|\, \text{Timestamp} \,\|\, \text{SessionID} \,\|\, \text{EventType} \,\|\, \text{PayloadCanonicalJSON}\Big)$$

Desta forma, se qualquer caractere de uma formulação, autor ou intervenção de IA for adulterado retroativamente no banco de dados SQLite, a cadeia de hashes torna-se inválida e o sistema expõe publicamente o selo de `CORRUPÇÃO DE AUDITORIA`.
