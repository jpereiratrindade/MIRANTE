# Especificação MPRR-UX-001
## Ecossistema SisTer — MIRANTE
### Experiência Assistida de Percepção Relacional

```text
Identificador:   MPRR-UX-001
Versão:          0.1.0
Data de Edição:  23 de outubro de 2026
Status:          Normativo / Especificação de Referência de UX e Interface
Protocolo Base:  MPRR-FOUNDATION-001 @ v0.1.0
Compatibilidade: SisTer-Git v1.x / MORFO UI Tokens / C++26 HTTP Backend
```

> **Assinatura:**  
> **MIRANTE**  
> *Deslocar perspectivas. Reconhecer relações. Construir questões.*  
>  
> **Máxima Arquitetural:**  
> *"Automatizar a exploração. Preservar a dúvida. Registrar o percurso. Submeter a descoberta à crítica."*  
>  
> **Lema Operacional:**  
> *"Sempre pronto. Sempre incompleto."*

---

## 1. Constituição do Sistema

### 1.1 Missão e Escopo
O **MIRANTE** é o ambiente computacional oficial do Ecossistema SisTer dedicado a apoiar pesquisadores na execução rigorosa do **Método de Percepção Relacional da Realidade (MPRR)**.

O sistema existe para conduzir o cientista por um percurso investigativo **documentável, criticável e reprodutível**, no qual se registram fenômenos, deslocam-se pontos de vista, reconhecem-se conexões hipotéticas, formulam-se questões empíricas, confrontam-se explicações rivais e reposiciona-se o conhecimento adquirido.

### 1.2 Princípio Fundamental da Não-Substituição
> **O MIRANTE não descobre a verdade, não certifica teorias e não substitui o julgamento científico.**

O sistema atua como uma **prótese cognitiva reflexiva**: automatiza a ampliação de repertório, sugere perguntas e exercita o papel de "advogado do diabo", mas toda decisão substantiva, juízo de pertinência e validação material permanecem sob inteira responsabilidade dos pesquisadores humanos.

### 1.3 As Três Diretrizes Prioritárias da Arquitetura
1. **Movimentos Revisáveis com Consolidação Verificável**: O processo não segue uma esteira linear cega (*waterfall*). Os movimentos analíticos são tratados como funções iterativas, permitindo retornos, desvios e bifurcações, desde que os requisitos de consolidação de cada movimento sejam auditados pelo sistema antes do fechamento do ciclo.
2. **Exploração e Crítica Nativas com Núcleo Autônomo**: A automação heurística (sugestão divergente de perspectivas) e a automação cética (confrontação convergente de hipóteses) integram o fluxo funcional básico desde o primeiro dia. Todavia, o núcleo de domínio do software é estritamente desacoplado, sendo 100% utilizável em modo estritamente humano, sem rede e sem dependência de qualquer modelo externo de inteligência artificial.
3. **Contratos Epistêmicos e Proveniência Categórica**: O sistema estabelece distinção inegociável entre **integridade procedimental** (atestada matematicamente pelo software através de encadeamento de eventos e hashes SHA-256) e **validação científica** (que depende de evidências materiais externas ao ambiente digital).

---

## 2. Transcrição Controlada do Protocolo MPRR

A interface e as regras do MIRANTE incorporam literalmente as definições, formulações e salvaguardas consolidadas no caderno de pesquisa `MPRR-FOUNDATION-001@0.1.0`.

### 2.1 Tese Orientadora Literal
> *"A percepção relacional da realidade consiste em deslocar deliberadamente os pontos de observação de um fenômeno para reconhecer propriedades e relações que não se apresentam com a mesma evidência sob uma perspectiva habitual. Essas relações constituem possibilidades explicativas que, quando convertidas em questões científicas e submetidas à crítica e à investigação empírica, podem ampliar o conhecimento sobre a realidade observada.*  
>  
> *O procedimento admite dois caminhos complementares: mobilizar conhecimentos distintos para produzir novas interpretações e reconhecer relações que revelem a necessidade de novos conhecimentos."*

### 2.2 As Cinco Distinções Epistêmicas Invioláveis (D1–D5)
A interface do MIRANTE é construída com travas de vocabulário e avisos explícitos para impedir que os usuários e o assistente computacional apaguem as seguintes fronteiras conceituais:

| Distinção | Enunciado Canônico do Protocolo | Salvaguarda de Interface no MIRANTE |
|---|---|---|
| **D1** | **Realidade e Representação**: Mudar a perspectiva altera o enquadramento interpretativo, não necessariamente o estado do objeto. | Alerta contextual permanente: *"A alteração registrada incide sobre o modelo de observação; não prova mutação física no fenômeno."* |
| **D2** | **Relação Percebida e Relação Demonstrada**: Uma conexão inicialmente formulada permanece hipótese até adquirir suporte adequado. | Proibição de rotular vínculos como "fato comprovado". Toda relação nasce compulsoriamente com o selo visual e semântico `CANDIDATA`. |
| **D3** | **Correlação e Mecanismo**: A covariação entre indicadores, por si só, não esclarece como um efeito é produzido. | Campo explícito e obrigatório na formulação da relação: exige discriminar se o vínculo é associação estatística, dependência de contexto ou mecanismo causal conjecturado. |
| **D4** | **Geração e Avaliação de Hipóteses**: Construir explicações e julgá-las requerem operações distintas, que podem ocorrer de forma iterativa. | Separação espacial rígida na interface: o ambiente de formulação heurística (Bancada) é isolado da Arena de Confronto e Crítica. |
| **D5** | **Explicação e Intervenção**: Compreender uma relação pode orientar ação, mas não garante que agir produzirá o efeito esperado. | Ações de manejo ou políticas públicas geradas em sessões são marcadas como `PROPOSTA_DE_INTERVENÇÃO`, com advertência de efeitos colaterais sistêmicos. |

### 2.3 Os Três Movimentos Heurísticos de Descoberta

#### Movimento 1 — Deslocar
- **Operação**: Registrar formalmente o enquadramento inicial e alterá-lo conscientemente em ao menos uma dimensão concreta:
  - Escala espacial (ex.: parcela $\leftrightarrow$ bacia $\leftrightarrow$ território);
  - Horizonte temporal (ex.: safra $\leftrightarrow$ decênio $\leftrightarrow$ ciclo secular);
  - Unidade de análise (ex.: planta individual $\leftrightarrow$ consórcio forrageiro $\leftrightarrow$ família produtora);
  - Papel do observador (ex.: olhar do agrônomo externo $\leftrightarrow$ vivência do pecuarista familiar);
  - Nível de organização sistêmica (ex.: biológico $\leftrightarrow$ ecossistêmico $\leftrightarrow$ institucional);
  - Variável ou processo privilegiado (ex.: volume de precipitação $\leftrightarrow$ tempo de retenção de umidade no solo);
  - Disciplina mobilizada (ex.: pedologia $\leftrightarrow$ sociologia econômica $\leftrightarrow$ teoria da informação);
  - Ator social considerado (ex.: produtores consolidados $\leftrightarrow$ jovens sucessores rurais).
- **Pergunta-Guia Canônica**:  
  *«O que se torna observável ou relevante se a mesma situação for interrogada a partir de outra perspectiva?»*
- **Produto Mínimo Verificável**:  
  Descrição comparativa explícita contendo: (a) Enquadramento Inicial, (b) Perspectiva Deslocada, (c) Variável/Dimensão alterada e (d) Justificativa epistêmica do deslocamento.

#### Movimento 2 — Relacionar
- **Operação**: Propor relações candidatas entre elementos postos em evidência pelo novo enquadramento, explicitando a natureza hipotética do vínculo.
- **Tipos de Natureza Relacional Catalogados**:
  1. `Associação`: covariação ou coexistência temporal/espacial;
  2. `Mediação`: efeito de X sobre Y intermediado pelo fator M;
  3. `Dependência de Contexto`: relação condicionada à presença de fatores de contorno;
  4. `Retroalimentação (Feedback)`: circuitos de reforço positivo ou amortecimento negativo;
  5. `Sequência Temporal`: assimetria temporal entre eventos antecedentes e consequentes;
  6. `Condição Necessária Possível`: fator sem o qual o estado final não se manifesta;
  7. `Mecanismo Causal Conjecturado`: descrição do processo físico/biológico/social mediador;
  8. `Analogia Heurística`: metáfora conceitual explicitamente marcada como não-mecanismo.
- **Pergunta-Guia Canônica**:  
  *«Quais relações se tornam mais nítidas e por que a nova perspectiva permite percebê-las?»*
- **Produto Mínimo Verificável**:  
  Proposição relacional clara enunciando os termos conectados, seu tipo relacional formal e a proibição de metáforas disfarçadas de relações empíricas.

#### Movimento 3 — Problematizar
- **Operação**: Converter a proposição relacional em questão investigável e, quando cabível, em hipóteses falseáveis ou critérios de elucidação.
- **Critérios de Qualidade Heurística (Qualitativos e Revisáveis)**:
  - *Inteligibilidade*: termos e conceitos semanticamente unívocos;
  - *Pertinência*: aderência genuína ao fenômeno concreto de partida;
  - *Distintividade*: representa avanço real e não mero jogo de sinônimos;
  - *Contestabilidade*: susceptibilidade de ser contrariada por dados teóricos ou empíricos;
  - *Viabilidade Investigativa*: existência de horizonte prático de coleta ou observação de evidências.
- **Pergunta-Guia Canônica**:  
  *«O que precisaríamos descobrir para saber se essa relação ajuda de fato a explicar o fenômeno?»*
- **Produto Mínimo Verificável**:  
  Ao menos uma formulação interrogativa inequívoca acompanhada da indicação clara de quais dados, evidências ou fontes permitiriam responder à pergunta.

### 2.4 Os Dois Controles de Crítica e Consolidação

#### Controle 4 — Confrontar
- **Operação**: Submeter a hipótese ao contraditório sistemático, buscando ativamente razões para refutá-la ou mitigar seu entusiasmo explicativo.
- **Vigilância Contra Falhas Previsíveis**:
  - *Analogia excessiva*: confundir homologia conceitual com processo idêntico;
  - *Sobre-interpretação*: inferir causalidade onde há apenas plausibilidade estética;
  - *Seleção confirmatória*: registrar apenas dados alinhados à hipótese;
  - *Inflação de perguntas*: proliferação ociosa de questionamentos sem ancoragem;
  - *Ilusão de originalidade*: reinventar a roda renomeando conceitos tradicionais;
  - *Anacronismo e mistura de escalas*: justapor séries históricas ou unidades espaciais incomparáveis.
- **Pergunta-Guia Canônica**:  
  *«Que observação, argumento ou condição enfraqueceria nossa proposição?»*
- **Produto Mínimo Verificável**:  
  Obrigatoriedade de registro de no mínimo **uma Explicação Rival plausível** e no mínimo **um Risco Epistêmico de Invalidação**. Nenhuma hipótese pode ser reposicionada sem esse confronto registrado.

#### Controle 5 — Reposicionar
- **Operação**: Retornar ao ponto de observação e deliberar explicitamente sobre o estatuto da hipótese e as novas exigências de conhecimento.
- **Taxonomia Canônica dos Estados da Hipótese**:
  - `DESENVOLVER`: hipótese promissora, demanda desdobramento e testes operacionais;
  - `REFORMULAR`: proposição com falhas conceituais ou de escala, bifurca em novo ciclo;
  - `SUSPENDER`: dados insuficientes no momento, preserva-se em estado de dúvida explícita;
  - `REJEITAR`: hipótese refutada pelo confronto inicial de fontes ou contradição lógica insuperável;
  - `SUSTENTAR_POR_EVIDÊNCIAS_ESPECÍFICAS`: hipótese amparada preliminarmente por acervo documental/empírico auditado.
- **Pergunta-Guia Canônica**:  
  *«O que passamos a poder perguntar e o que ainda não temos condições de afirmar?»*
- **Produto Mínimo Verificável**:  
  Decisão fundamentada sobre o estado da hipótese e indicação mandatória do **Próximo Teste Empírico** ou **Conhecimento Necessário** a ser adquirido.

---

## 3. Modelo de Movimentos Revisáveis e Consolidação Verificável

Em superação aos modelos de etapas sequenciais rígidas (estilo *wizard* fechado), o MIRANTE adota uma **arquitetura de estados flexíveis com guardas de consolidação** (*Verifiable Consolidation Gates*).

```text
       ┌────────────────────────────────────────────────────────┐
       │                 FENÔMENO E SITUAÇÃO                     │
       └──────────────┬──────────────────────────┬──────────────┘
                      │                          │
              [Caminho A: Conhecimento]   [Caminho B: Relação]
                      │                          │
       ┌──────────────┴──────────────────────────┴──────────────┐
       │                   BANCADA MPRR                         │
       │                                                        │
       │   [ M1: Deslocar ]  ◄──┐                               │
       │          │             │  Retornos e                   │
       │          ▼             │  Bifurcações                  │
       │   [ M2: Relacionar ] ──┤  Auditadas                    │
       │          │             │                               │
       │          ▼             │                               │
       │   [ M3: Problematizar ]│                               │
       └──────────────┬─────────┴───────────────────────────────┘
                      │ Requisito: M1 + M2 + M3 preenchidos
                      ▼
       ┌────────────────────────────────────────────────────────┐
       │               C4: ARENA DE CONFRONTO                   │
       │  - Explicações Rivais (mínimo 1)                       │
       │  - Riscos Epistêmicos e Invalidação (mínimo 1)          │
       └──────────────┬─────────────────────────────────────────┘
                      │ Requisito: Confronto Completo
                      ▼
       ┌────────────────────────────────────────────────────────┐
       │             C5: MESA DE REPOSICIONAMENTO               │
       │  - Decisão: Desenvolver / Reformular / Suspender / ... │
       │  - Próximo Teste ou Conhecimento Necessário            │
       └──────────────┬─────────────────────────────────────────┘
                      │ Fechamento de Ciclo
                      ▼
       ┌────────────────────────────────────────────────────────┐
       │     CICLO N CONSOLIDADO -> NOVO CICLO N+1 (Bifurcação) │
       └────────────────────────────────────────────────────────┘
```

### 3.1 Estados de Consolidação de Cada Movimento
Cada movimento possui seu próprio indicador de completude:
- `EM_EDICAO` (Rascunho aberto): permite modificação livre de campos;
- `PENDENTE_CRITICA`: produto mínimo rascunhado, aguardando exame de contraponto;
- `CONSOLIDADO_CICLO`: campos mínimos satisfeitos e assinados pelo pesquisador para o ciclo corrente;
- `REABERTO_REVISAO`: o pesquisador retornou de um passo posterior para reescrever premissas; gera automaticamente uma nova ramificação na árvore de proveniência (`branch_id`).

### 3.2 Regras de Navegação e Não-Bloqueio
- O pesquisador pode transitar livremente entre M1, M2 e M3 a qualquer instante para ajustar formulações.
- O avanço para C4 (Confronto) é permitido assim que houver ao menos um rascunho de relação e de questão formulada.
- O fechamento em C5 (Reposicionamento) requer que C4 tenha cumprido seus invariantes (ao menos 1 rival e 1 risco cadastrados).
- Ao decidir por `REFORMULAR` em C5, o sistema cria o **Ciclo vN+1**, preservando o Ciclo vN imutável e abrindo a Bancada com as premissas herdadas.

---

## 4. Fluxo das Duas Portas de Entrada

O MIRANTE formaliza a bipartição das rotas heurísticas descritas no protocolo:

```text
                                  FENÔMENO
                                     │
           ┌─────────────────────────┴─────────────────────────┐
           ▼                                                   ▼
     [ CAMINHO A ]                                       [ CAMINHO B ]
"Do Conhecimento à Realidade"                       "Da Realidade ao Conhecimento"
           │                                                   │
   Mobiliza-se um corpo                                Um padrão relacional
  de conhecimento prévio                             intrigante é identificado
  (ex.: Teoria de Redes,                              diretamente no fenômeno
   Ecologia Funcional)                                 (ex.: disparidade temporal
           │                                          entre seca e falência rural)
           ▼                                                   ▼
Que novas facetas desse                             Que novos conhecimentos
fenômeno essa lente teórica                         precisamos importar para
  permite enxergar?                                   conseguir explicá-lo?
           │                                                   │
           └─────────────────────────┬─────────────────────────┘
                                     ▼
                          QUESTÕES INVESTIGÁVEIS
```

- **Invariante da Porta**: A escolha de uma porta inicial (A ou B) serve para orientar o painel assistivo e os formulários de enquadramento, mas **nunca restringe** a alternância durante a sessão. O pesquisador pode documentar que a investigação começou pelo Caminho A e, no meio da Bancada, incorporou indagações típicas do Caminho B.

---

## 5. Mapa de Telas e Navegação

A interface web do MIRANTE adota uma navegação persistente lateral, cabeçalho de proveniência com breadcrumbs, área central de trabalho com abas analíticas e painel retrátil de assistência e contraponto.

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│ ECOSISTEMA SISTER  |  MIRANTE v0.1.0        Sessão: [SES-2026-10-001 v1] ⦿  │
├──────────────┬──────────────────────────────────────────────┬───────────────┤
│ [SISTEMA]    │ BREADCRUMB: Projeto > Sessão > Bancada MPRR  │ [ASSISTÊNCIA] │
│ • Início     ├──────────────────────────────────────────────┤ Modo: Ativo   │
│ • Sessões    │                                              │ Motor: Local  │
│ • Nova Exp.  │   [M1: Deslocar] [M2: Relacionar] [M3: Prob] │               │
│              │   ────────────────────────────────────────── │ Sugestões     │
│ [ANÁLISE]    │                                              │ divergentes   │
│ • Fenômenos  │   ÁREA DE CONSTRUÇÃO HUMANA                  │ e testes de   │
│ • Relações   │   (Editor formal do pesquisador com          │ consistência  │
│ • Questões   │    marcação estrita de proveniência)         │               │
│ • Evidências │                                              │ [Contraponto] │
│ • Confronto  │   ────────────────────────────────────────── │ [Aceitar]     │
│ • Reposic.   │   ALERTAS EPISTÊMICOS NATIVOS (D1-D5)        │ [Rejeitar]    │
│              │                                              │               │
│ [GOVERNANÇA] ├──────────────────────────────────────────────┴───────────────┤
│ • Dossiê     │ RODAPÉ: SHA-256: 4f9e...3a12 | Estado: EM_EXPLORAÇÃO         │
│ • Proveniência│ Status de Validação: INTEGRIDADE PROCEDIMENTAL ATIVA         │
└──────────────┴──────────────────────────────────────────────────────────────┘
```

### 5.1 Telas Canônicas

1. **Painel Inicial (`/dashboard`)**:
   - Resumo de sessões ativas, fenômenos em observação e distribuição das hipóteses por estado (`DESENVOLVER`, `REFORMULAR`, `SUSPENSA`, `REJEITADA`).
   - Fila de auditoria crítica: lista de hipóteses formuladas que aguardam confronto obrigatório.
   - Indicador de status do motor de assistência (Modo Humano Puro vs. Assistência Ativa).
2. **Nova Sessão (`/sessions/new`)**:
   - Registro da Ficha Canônica: identificador, data, autores/participantes, contexto do projeto e fontes iniciais.
   - Escolha da Porta de Entrada prioritária (Caminho A ou Caminho B).
3. **Bancada MPRR (`/sessions/:id/workbench`)**:
   - Ambiente unificado dos três movimentos de descoberta:
     - Aba M1: Enquadramento inicial vs. Perspectiva deslocada.
     - Aba M2: Construtor relacional com seletor de natureza de vínculo.
     - Aba M3: Matriz de problematização e critérios qualitativos (inteligibilidade, pertinência, etc.).
   - Painel lateral de Assistência Desacoplada e Alertas Epistêmicos em tempo real.
4. **Arena de Confronto (`/sessions/:id/confrontation`)**:
   - Construtor obrigatório de Explicações Rivais e Riscos de Invalidação.
   - Painel de contra-argumentação assistida ("Advogado do Diabo").
5. **Mesa de Reposicionamento (`/sessions/:id/reposition`)**:
   - Julgamento do ciclo: atribuição do estado da hipótese, registro do próximo teste empírico e geração de ramificação para novo ciclo.
6. **Dossiê Científico e Exportação (`/sessions/:id/dossier`)**:
   - Visualização integral da ficha de percurso do protocolo, histórico de versões com diffs de texto, proveniência detalhada de cada bloco e exportação para JSON canônico (.sister-research.json), HTML científico e PDF de arquivamento.
7. **Trilha de Proveniência e Auditoria (`/sessions/:id/provenance`)**:
   - Grafo de eventos imutáveis com carimbo de tempo, autor, hashes SHA-256 e ligações às fontes primárias.

---

## 6. Modelo de Proveniência, Campos e Estados

### 6.1 Taxonomia de Origem das Informações
Cada fragmento textual ou entidade no MIRANTE carrega compulsoriamente um dos seguintes rótulos de proveniência:

```text
[HUMANO]     -> Redigido ou modificado diretamente pelo pesquisador.
[ASSISTIDO]  -> Gerado por modelo de IA e incorporado com revisão humana.
[IMPORTADO]  -> Proveniente de fonte bibliográfica externa, com citação expressa.
[EVIDÊNCIA]  -> Registro factual de observação empírica ou base de dados.
[INFERÊNCIA] -> Conclusão lógica derivada durante a sessão pelo coletivo de autores.
```

### 6.2 Entidades de Domínio e Ficha de Rastreabilidade

| Entidade | Campos Canônicos Mínimos | Identificador de Exemplo |
|---|---|---|
| **Projeto** | `id`, `titulo`, `area_conhecimento`, `criado_em` | `PRJ-RESILIENCIA` |
| **Sessão MPRR** | `id`, `projeto_id`, `titulo`, `porta_entrada`, `estado`, `versao_ciclo` | `SES-2026-10-001` |
| **Fenômeno** | `descricao_evento`, `recorte_espacial`, `recorte_temporal`, `contexto` | `FEN-001` |
| **Enquadramento** | `ponto_observacao`, `escala`, `atores_considerados`, `supostos_explicitos` | `ENQ-001-INI` |
| **Deslocamento** | `dimensao_alterada`, `justificativa`, `perspectiva_resultante` | `DES-001` |
| **Relação Candidata**| `termo_a`, `termo_b`, `natureza_relacional`, `justificativa_heuristica` | `REL-001` |
| **Questão Científica**| `enunciado_pergunta`, `criterios_qualidade`, `fontes_elucidacao` | `QST-001` |
| **Hipótese** | `proposicao_explicativa`, `estado`, `ciclo_versao` | `HIP-001` |
| **Confronto** | `explicacao_rival`, `risco_epistemico`, `condicao_invalidacao` | `CNF-001` |
| **Reposicionamento**| `decisao_estado`, `proximo_passo`, `justificativa_epistemica` | `REP-001` |
| **Evento Auditável**| `id_evento`, `sessao_id`, `hash_anterior`, `hash_conteudo`, `autor`, `timestamp` | `EVT-0042` |

---

## 7. Regras e Alertas Epistêmicos Nativos

O motor de integridade epistêmica avalia passivamente as formulações do usuário e emite alertas visuais não-obstrutivos, funcionando como um supervisor metodológico:

```text
  [!] ALERTA EPISTÊMICO (D2: Relação Candidata vs. Demonstrada)
  O texto inserido contém o termo "comprova". No protocolo MPRR, uma relação
  recém-formulada no Movimento 2 é estritamente CANDIDATA.
  -> Ação sugerida: Substituir por "sugere", "indica possibilidade de" ou "abre hipótese para".
```

### Catálogo de Alertas Automáticos

1. **Gatilho de Certeza Prematura (D2)**: Disparado quando termos como *"comprovado"*, *"demonstra categoricamente"* ou *"fato incontestável"* surgem nos movimentos M1, M2 ou M3.
2. **Gatilho de Correlação sem Mecanismo (D3)**: Disparado se o usuário conectar dois indicadores sem preencher a caixa de processo mediador.
3. **Gatilho de Monocultura Disciplinar**: Disparado quando todas as fontes e enquadramentos de uma sessão pertencem a uma única especialidade, convidando ao deslocamento por atores sociais ou ciências afins.
4. **Gatilho de Ausência de Objeção (C4)**: Bloqueia a consolidação do reposicionamento caso não haja ao menos uma explicação rival e uma condição de invalidação registrada.
5. **Gatilho de Mudança Oculta de Escala**: Disparado quando dados em nível micro (ex.: folha/planta) são transpostos imediatamente para afirmações em nível macro (ex.: bacia hidrográfica/mercado global) sem mediação explícita.

---

## 8. Fronteira e Protocolo da Assistência

### 8.1 Princípios de Blindagem do Assistente
O módulo de IA opera como um **assistente socrático desacoplado**.
- **O que o assistente pode fazer**:
  - Sugerir dimensões inovadoras de deslocamento de enquadramento;
  - Apontar ambiguidades ou termos conceituais vagos;
  - Enunciar explicações alternativas consagradas na literatura;
  - Desafiar a hipótese gerando argumentos rivais convincentes ("advogado do diabo");
  - Identificar atores ou componentes ecológicos omitidos na delimitação do sistema.
- **O que o assistente está expressamente proibido de fazer**:
  - Declarar qualquer relação como verdadeira ou confirmada;
  - Fabricar ou alucinar citações, fontes e referências bibliográficas;
  - Escrever ou substituir texto humano diretamente no editor sem autorização prévia;
  - Atribuir pontuações ou rankings matemáticos de "mérito científico".

### 8.2 Protocolo das Quatro Ações para Sugestões
Toda e qualquer intervenção sugerida pelo assistente na interface apresenta, compulsoriamente, quatro opções para o pesquisador:

```text
┌────────────────────────────────────────────────────────────────────────┐
│ SUGESTÃO DE DESLOCAMENTO ASSISTIDO [ASSISTIDO: Llama-3-70B-Local]       │
│ "Investigar o fenômeno a partir da assimetria entre o tempo ecológico   │
│ de recarga hídrica do solo e o ciclo financeiro de crédito rural."     │
├────────────────────────────────────────────────────────────────────────┤
│ [Aceitar como rascunho assistido]  -> Incorpora com tag [ASSISTIDO]    │
│ [Editar antes de incorporar]       -> Abre no editor humano para ajuste │
│ [Rejeitar]                         -> Descarta sem poluir o registro    │
│ [Solicitar contraponto]            -> Pede à IA razões contra a ideia  │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 9. Wireframes Conceituais

### 9.1 Wireframe: Bancada MPRR (Movimentos 1, 2 e 3)

```text
+----------------------------------------------------------------------------------------------------+
| SISTER ECOSYSTEM  |  MIRANTE  |  Projeto Resiliência  |  Sessão: SES-2026-10-001  [Ciclo 1]        |
+----------------------------------------------------------------------------------------------------+
| NAVEGAÇÃO     | TRILHA: [Porta B: Emergente] -> [1. Deslocar] -> [2. Relacionar] -> [3. Problematizar]
|               +---------------------------------------------------+--------------------------------+
| - Painel      | ENQUADRAMENTO ATUAL:                              | ASSISTÊNCIA SOCRÁTICA          |
| - Sessões     | "Avaliação da seca pela taxa de lotação animal    | (Motor: Llama-Local v3.2)      |
| > Bancada MPRR|  por hectare no ano de 2025."                     |                                |
| - Confronto   +---------------------------------------------------+ Sugestão de Deslocamento:      |
| - Reposicionar| NOVO PONTO DE OBSERVAÇÃO (DESLOCAMENTO):          | "Considere deslocar da escala  |
| - Dossiê      | Dimensão: [ Temporalidade / História de Legados ] | de safra para a série histórica|
| - Proveniência|                                                   | de eventos climáticos prévios."|
|               | Justificativa do Deslocamento:                    |                                |
|               | [ A resposta à seca pode depender do manejo   ]   | [Aceitar como Rascunho]        |
|               | [ acumulado na década anterior, não só do ano ]   | [Editar antes de Inserir]      |
|               |                                                   | [Pedir Contraponto]            |
|               +---------------------------------------------------+--------------------------------+
|               | RELAÇÃO CANDIDATA:                                | ALERTAS EPISTÊMICOS NATIVOS    |
|               | Termo A: [ Legado de Matéria Orgânica do Solo ]   |                                |
|               | Conexão: [ Mecanismo Causal Conjecturado    v ]   | [D2] Relação marcada como      |
|               | Termo B: [ Tempo de Recuperação da Pastagem   ]   |      CANDIDATA (OK).           |
|               |                                                   | [D3] Mecanismo ecológico       |
|               | Descrição do Mecanismo Hipotético:                |      requer detalhamento da    |
|               | [ Solos com manejo conservacionista retêm     ]   |      retenção hídrica na raiz. |
|               | [ umidade e sustentam o banco de sementes.    ]   |                                |
+---------------+---------------------------------------------------+--------------------------------+
| AUDITORIA     | Hash Bloco: 8f4c...b912 | Origem: [HUMANO: J.P. Trindade] | Estado: CONSOLIDADO_CICLO_1  |
+----------------------------------------------------------------------------------------------------+
```

### 9.2 Wireframe: Arena de Confronto e Crítica (Controle 4)

```text
+----------------------------------------------------------------------------------------------------+
| SISTER ECOSYSTEM  |  MIRANTE  |  Arena de Confronto Crítico (Controle 4)                           |
+----------------------------------------------------------------------------------------------------+
| HIPÓTESE SUBMETIDA AO CONFRONTO:                                                                   |
| "A resiliência das pastagens depende primariamente da memória biológica do solo acumulada."       |
+--------------------------------------------------------------------+-------------------------------+
| EXPLICACÕES RIVAIS OBRIGATÓRIAS (Mínimo 1):                        | RISCOS EPISTÊMICOS & REFUTAÇÃO|
|                                                                    |                               |
| [ Rival 1: Acesso a Poços Artesianos / Irrigação Suplementar ]     | Condição de Invalidação:      |
| "A recuperação decorre da capacidade financeira de bombear água,   | "Se áreas degradadas com alto |
|  e não da matéria orgânica pretérita do solo."                     |  capital financeiro tiverem   |
|  Origem: [HUMANO: Revisor Crítico B]                               |  recuperação mais rápida que  |
|                                                                    |  solos conservados sem caixa."|
| [+ Adicionar Explicação Rival Adicional]                           |                               |
|                                                                    | Falha Previsível Vigida:      |
|                                                                    | [!] Risco de Sobre-interpre-  |
|                                                                    |     tação de correlação.      |
+--------------------------------------------------------------------+-------------------------------+
| ADVOGADO DO DIABO COMPUTACIONAL (Assistência Crítica Ativa)                                        |
| "Objeção: Você considerou a profundidade do lençol freático local? Em zonas de baixada, a umidade |
|  pode resultar de fluxo subsuperficial regional e não do manejo da propriedade."                   |
| [Incorporar como Risco de Invalidação]  [Ignorar Objeção com Justificativa]                        |
+----------------------------------------------------------------------------------------------------+
| STATUS DO CONFRONTO: [CONFORME] (1 Rival, 1 Risco Cadastrado) -> [Prosseguir para Reposicionamento]|
+----------------------------------------------------------------------------------------------------+
```

### 9.3 Wireframe: Mesa de Reposicionamento (Controle 5)

```text
+----------------------------------------------------------------------------------------------------+
| SISTER ECOSYSTEM  |  MIRANTE  |  Mesa de Reposicionamento e Decisão Epistêmica (Controle 5)        |
+----------------------------------------------------------------------------------------------------+
| SÍNTESE DO CICLO 1:                                                                                |
| Questão: "Em que medida legados de solo explicam a recuperação da pastagem pós-estiagem?"          |
+----------------------------------------------------------------------------------------------------+
| DELIBERAÇÃO DE STATUS DA HIPÓTESE:                                                                 |
| ( ) DESENVOLVER                     (o) REFORMULAR                     ( ) SUSPENDER               |
| ( ) REJEITAR                        ( ) SUSTENTAR POR EVIDÊNCIAS ESPECÍFICAS                       |
|                                                                                                    |
| Justificativa do Veredito:                                                                         |
| [ Diante do confronto sobre fluxo subsuperficial e irrigação de socorro, a pergunta original      ] |
| [ precisa isolar topografia e capacidade financeira antes de testar a memória biológica.         ] |
+----------------------------------------------------------------------------------------------------+
| REQUISITO OBRIGATÓRIO DE PROGRESSÃO:                                                               |
| Próximo Teste Empírico ou Conhecimento Necessário:                                                 |
| [ Mapeamento hidrogeológico das áreas amostrais e estratificação por classe de renda dos pecuaristas]|
+----------------------------------------------------------------------------------------------------+
| [BIFURCAR PARA CICLO v2] -> Cria Ciclo 2 preservando Ciclo 1 imutável e auditado.                 |
+----------------------------------------------------------------------------------------------------+
```

---

## 10. Identidade Visual e Tokens (SisTer / MORFO)

O visual do MIRANTE espelha rigor científico, estabilidade e clareza analítica, dispensando linguagens puramente cosméticas.

```css
:root {
  /* Paleta Base SisTer / MORFO */
  --sister-navy-deep:    #071e38;
  --sister-navy-surface: #0c2844;
  --sister-blue-primary: #1a7dc4;
  --sister-blue-sky:     #2ea8e8;
  --sister-teal-rel:     #087f8c;
  --sister-green-mech:   #14805e;
  --sister-amber-shift:  #ad7841;
  --sister-gold-doubt:   #e09f3e;
  --sister-red-risk:     #d9534f;
  --sister-bg-canvas:    #edf2f8;
  --sister-bg-card:      #ffffff;
  --sister-text-main:    #18334b;
  --sister-text-muted:   #58646a;
  --sister-border:       #c8d8d2;

  /* Tipografia */
  --font-ui:   "Plus Jakarta Sans", "Inter", -apple-system, sans-serif;
  --font-mono: "JetBrains Mono", "Fira Code", monospace;

  /* Raios e Transições */
  --radius-sm: 4px;
  --radius-md: 8px;
  --radius-lg: 12px;
  --transition: all 0.18s cubic-bezier(0.4, 0, 0.2, 1);
}
```

---

## 11. Critérios de Aceite da Experiência (UX Acceptance Criteria)

Uma sessão realizada no MIRANTE só é considerada completa e válida sob a ótica do sistema quando atender a todos os critérios abaixo:

1. **Reconstituição Independente do Percurso**: Qualquer pesquisador terceiro que acesse o dossiê da sessão consegue reconstruir a cadeia lógica completa: do fenômeno ao deslocamento, da relação à questão, do confronto ao reposicionamento.
2. **Explicitação de Mudança de Perspectiva**: O enquadramento inicial e o ponto de observação alternativo estão formalmente descritos, acompanhados da dimensão variada e da justificativa metodológica.
3. **Impossibilidade de Relação Factual Imediata**: Nenhuma relação gerada nos movimentos de descoberta pode assumir status de comprovação empírica ou verdade estabelecida dentro do sistema.
4. **Confronto Obrigatório com Contraditório Efetivo**: O sistema recusa o avanço para o reposicionamento caso o pesquisador não tenha registrado ao menos uma explicação rival plausível e uma condição clara de invalidação.
5. **Decisão Epistêmica com Próximo Passo**: Toda sessão encerra com uma decisão formal de reposicionamento (`DESENVOLVER`, `REFORMULAR`, `SUSPENDER`, `REJEITADA` ou `SUSTENTADA`) vinculada a um teste empírico ou conhecimento requerido.
6. **Autonomia Operacional sem IA**: Todas as telas, formulários, validações e exportações operam com integridade absoluta em modo humano puro, com o motor de assistência desativado.
7. **Rastreabilidade de Proveniência**: Cada parágrafo do relatório possui indicador inequívoco de sua fonte (`HUMANO`, `ASSISTIDO`, `IMPORTADO`, `EVIDÊNCIA` ou `INFERÊNCIA`), garantido por encadeamento criptográfico SHA-256.
8. **Imutabilidade Histórica**: Nenhuma edição sobrescreve silenciosamente um ciclo ou decisão anterior; alterações geram novas versões e ramificações versionadas.
