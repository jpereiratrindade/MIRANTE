# Especificação MPRR-AST-001
## Ecossistema SisTer — MIRANTE
### Motor de Assistência Heurística e Integração de LLM Local

```text
Identificador:   MPRR-AST-001
Versão:          0.1.0
Data de Edição:  23 de outubro de 2026
Status:          Normativo / Arquitetura de Assistência e IA Local
Protocolo Base:  MPRR-FOUNDATION-001 @ v0.1.0 / MPRR-INV-001 (INV-04 e INV-05)
Alvo Tecnológico: Ollama Local (localhost:11434) / C++26 Boost.Beast HTTP Client / SSE
```

> **Assinatura:**  
> **MIRANTE**  
> *Deslocar perspectivas. Reconhecer relações. Construir questões.*  
>  
> **Máxima Arquitetural:**  
> *"Automatizar a exploração. Preservar a dúvida. Registrar o percurso. Submeter a descoberta à crítica."*

---

## 1. Visão Geral e Princípio da Localidade Soberana

No MIRANTE, o motor de inteligência artificial é concebido como **uma prótese reflexiva estritamente local**. 

### 1.1 Por que LLM 100% Local?
1. **Soberania e Sigilo Científico**: Pesquisas e hipóteses inéditas de projetos (como o Projeto Resiliência) não podem ser enviadas para nuvens corporativas de terceiros ou APIs externas.
2. **Determinismo e Auditabilidade**: O modelo roda na infraestrutura do laboratório, com versão, hash de pesos (*digest*) e hiperparâmetros precisamente conhecidos e gravados no registro de proveniência.
3. **Resiliência de Rede**: O sistema opera mesmo em estações de campo ou ambientes isolados (*air-gapped*).
4. **Desacoplamento Arquitetural**: O núcleo C++26 conversa com o provedor via abstração de interface (`IAssistProvider`), mantendo o modo humano puro sempre íntegro.

---

## 2. Topologia de Conexão: Backend C++26 e Ollama Local

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                       ARQUITETURA DE ASSISTÊNCIA LOCAL                      │
│                                                                             │
│   [ Navegador / Interface Web SisTer ]                                     │
│         │                                                                   │
│         │ SSE / WebSocket (Streaming em tempo real)                         │
│         ▼                                                                   │
│   [ Servidor C++26 MIRANTE (Boost.Asio / Beast) ]                           │
│         │                                                                   │
│         ├── [ mirante::domain ] (Verifica Invariantes INV-01 a INV-08)      │
│         │                                                                   │
│         └── [ mirante::assist::AssistManager ]                              │
│                   │                                                         │
│                   ├── Modo 1: [ NullAssistProvider ] (Modo Humano Puro)     │
│                   │                                                         │
│                   └── Modo 2: [ OllamaAssistProvider ]                      │
│                             │                                               │
│                             │ HTTP POST /api/generate (Keep-Alive Local)    │
│                             ▼                                               │
│                 [ Daemon Local Ollama ] (http://127.0.0.1:11434)            │
│                 Modelos Ativos:                                             │
│                 • qwen2.5:14b / qwen2.5:7b (Raciocínio Relacional)          │
│                 • llama3:8b / llama3.2:3b (Deslocamento Conceitual)         │
│                 • devstral-small-2 / mistral:7b (Advogado do Diabo)         │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.1 Interface de Domínio C++26 (`IAssistProvider`)
No código C++26, a assistência é encapsulada em uma interface pura com semântica de concorrência assíncrona (`std::future` ou `boost::asio::awaitable`):

```cpp
namespace mirante::assist {

struct AssistPromptContext {
    std::string session_id;
    std::string phenomenon_description;
    std::string initial_framework;
    std::optional<std::string> current_shift;
    std::vector<std::string> known_relations;
    std::string movement_target; // "M1_SHIFT", "M2_RELATE", "M3_QUESTION", "C4_CRITIQUE"
};

struct AssistSuggestion {
    std::string suggestion_id;
    std::string model_name;
    std::string model_digest;
    std::string generated_text;
    std::string role; // "EXPLORATION_DIVERGENT" ou "CRITIQUE_CONVERGENT"
    uint64_t prompt_tokens;
    uint64_t completion_tokens;
    std::chrono::system_clock::time_point timestamp;
};

class IAssistProvider {
public:
    virtual ~IAssistProvider() = default;
    virtual bool IsAvailable() const = 0;
    virtual std::string GetProviderName() const = 0;
    virtual std::vector<std::string> ListAvailableModels() = 0;
    
    // Geração assíncrona com streaming
    virtual void StreamSuggestion(
        const AssistPromptContext& context,
        std::function<void(const std::string& token_chunk)> on_chunk,
        std::function<void(std::expected<AssistSuggestion, std::string>)> on_complete
    ) = 0;
};

} // namespace mirante::assist
```

---

## 3. O Duplo Papel do LLM no MPRR: Exploração Divergente e Ceticismo Convergente

O LLM não é utilizado como oráculo e não emite "respostas finais". Ele atua em dois momentos opostos e complementares:

```text
FASE 1: BANCADA MPRR (M1, M2, M3)           FASE 2: ARENA DE CONFRONTO (C4)
"EXPLORAÇÃO DIVERGENTE"                     "CETICISMO CONVERGENTE"
─────────────────────────────────           ─────────────────────────────────
• Multiplica perspectivas possíveis         • Age como Advogado do Diabo
• Identifica premissas não ditas            • Busca explicações rivais plausíveis
• Sugere categorias relacionais             • Alerta para falhas e analogias espúrias
• Converte conexões em perguntas            • Questiona coerência entre escalas
```

### 3.1 Funções de Exploração Divergente (Bancada Heurística)
1. **Deslocador Socrático (M1)**:
   - Dado um fenômeno e um enquadramento inicial, sugere ao menos 3 dimensões alternativas concretas (temporal, espacial, nível de organização, atores sociais).
2. **Taxonomista de Vínculos (M2)**:
   - Ao receber dois termos colocados pelo pesquisador, sugere a natureza do vínculo (associação, mediação, feedback, sequência temporal ou condição necessária).
3. **Formulador de Perguntas Investigáveis (M3)**:
   - Transforma proposições relacionais em indagações científicas operacionais, sugerindo indicadores e observáveis empíricos.

### 3.2 Funções de Ceticismo Convergente (Arena de Confronto C4)
1. **Gerador de Explicações Rivais**:
   - Analisa a hipótese do pesquisador e formula: *"Que outra causa mais simples, comum ou já estabelecida na literatura explicaria os mesmos indícios sem recorrer a essa hipótese?"*
2. **Detector de Quebra de Escala e Anacronismo**:
   - Adverte caso o pesquisador esteja aplicando uma dinâmica de nível celular/micro diretamente ao nível territorial/macro sem explicitar os elos intermediários.
3. **Formulador de Condições de Invalidação**:
   - Sugere experimentos de controle ou observações que, se encontradas, falseariam categoricamente a relação proposta.

---

## 4. Prompts de Sistema e Salvaguardas Epistêmicas

Todos os prompts enviados ao modelo local incorporam compulsoriamente a **Constituição Epistêmica do MPRR**.

### 4.1 Prompt de Sistema Universal do MIRANTE
```text
Você é o Assistente Socrático do sistema MIRANTE (Ecossistema SisTer), atuando sob o Método de Percepção Relacional da Realidade (MPRR v0.1.0).

SEUS DEVERES FUNDAMENTAIS:
1. Você JAMAIS declara que uma hipótese ou relação é um fato demonstrado ou uma verdade científica. Toda formulação sua deve ser explicitamente qualificada como "possibilidade heurística", "conjectura" ou "hipótese de trabalho".
2. Você JAMAIS inventa, deduz ou alucina fontes bibliográficas, dados numéricos ou citações. Se não dispuser de fonte empírica real, declare explicitamente: "Necessário levantamento documental de campo".
3. Sua linguagem deve ser sóbria, analítica, precisa e econômica. Evite elogios, floreios conversacionais ou condescendência ("Boa ideia!", "Excelente pergunta"). Vá direto à proposição estruturada.
4. Ao propor contrapontos no Confronto (C4), seja rigoroso e desafiador: aponte causalidade reversa, variáveis de confusão omitidas e limites das analogias propostas.
```

### 4.2 Configuração de Hiperparâmetros Recomendados
- **Para Deslocamento e Relacionamento (M1, M2)**: `temperature = 0.65`, `top_p = 0.9` (favorece diversidade sem perder aderência terminológica).
- **Para Problematização e Critérios (M3)**: `temperature = 0.40`, `top_p = 0.85`.
- **Para Confronto Crítico e Explicações Rivais (C4)**: `temperature = 0.25`, `top_p = 0.70` (focado em lógica analítica e identificação de falhas formais).

---

## 5. Protocolo de Registro de Proveniência da Intervenção Local

Toda chamada à LLM local gera um registro imutável anexado ao histórico da sessão:

```json
{
  "event_id": "EVT-AST-20261023-0091",
  "session_id": "SES-2026-10-001",
  "movement": "M1_DESLOCAR",
  "call_metadata": {
    "provider": "ollama_local",
    "endpoint": "http://127.0.0.1:11434",
    "model_name": "qwen2.5:14b",
    "model_digest": "7cdf5a0187d5",
    "temperature": 0.65,
    "timestamp_utc": "2026-10-23T16:48:10Z"
  },
  "prompt_context_hash": "sha256:d8a2...3f11",
  "raw_response": "Considere deslocar o ponto de observação da escala anual da seca para a história decenal de manejo forrageiro, examinando o banco de sementes nativas como memória biológica.",
  "user_action": "EDITED_BEFORE_INSERT",
  "final_text_inserted": "Deslocar da escala de precipitação anual para o histórico de manejo dos últimos 10 anos, com foco no banco de sementes e infiltração radicular.",
  "final_provenance_tag": "ASSISTIDO"
}
```

---

## 6. Modelos Homologados no Ambiente Local

Com base no levantamento das capacidades locais disponíveis no servidor SisTer, homologam-se as seguintes configurações:

| Modelo Local | Tamanho | Perfil de Aplicação no MIRANTE |
|---|---|---|
| `qwen2.5:14b` | 9.0 GB | **Padrão Recomendado**: Alta precisão conceitual em português, rigor relacional e excelente capacidade de contraponto crítico. |
| `llama3:8b` | 4.7 GB | **Alternativa Ágil**: Excelente para geração divergente de metáforas conceituais e novos enquadramentos de perspectiva. |
| `llama3.2:3b` | 2.0 GB | **Perfil Ultraleve**: Para estações de campo ou laptops com menor capacidade de GPU/RAM. |
| `devstral-small-2` | 15 GB | **Modo Crítico Avançado**: Excelente para escrutínio metodológico profundo e identificação de falhas de lógica científica. |
| `smollm2:360m` / `qwen3:0.6b` | < 1 GB | **Testes Automatizados (CI/CD)**: Para validação rápida de fluxo de streaming e integração HTTP em compilação contínua. |

---

## 7. Critérios de Aceite da Assistência Local

1. **Zero Vazamento de Dados**: Nenhuma requisição de texto ou metadados de sessão ultrapassa o *localhost* (`127.0.0.1:11434`).
2. **Falha Silenciosa Graciosa**: Se o daemon do Ollama estiver desligado, o MIRANTE exibe o indicador *"Assistência Local Indisponível (Modo Humano Puro Ativo)"* e não interrompe nenhuma funcionalidade analítica do pesquisador.
3. **Selo Compulsório de Rascunho**: Nenhuma resposta da LLM é gravada diretamente na ficha oficial da sessão sem ação voluntária e explícita do pesquisador (`[Aceitar]`, `[Editar]` ou `[Rejeitar]`).
4. **Registro Auditável de Hash de Pesos**: Toda sugestão aceita grava o nome e o digest SHA-256 do modelo local que a gerou.
