# Ecossistema SisTer — MIRANTE
## Experiência Assistida de Percepção Relacional

> **MIRANTE**  
> *Deslocar perspectivas. Reconhecer relações. Construir questões.*  
>  
> **Máxima Arquitetural:**  
> *"Automatizar a exploração. Preservar a dúvida. Registrar o percurso. Submeter a descoberta à crítica."*  
>  
> **Lema Operacional:**  
> *"Sempre pronto. Sempre incompleto."*

---

## 1. Visão Geral

O **MIRANTE** é uma aplicação científica construída em C++26 com interface web integrada, projetada para operacionalizar o **Método de Percepção Relacional da Realidade (MPRR v0.1.0)** no âmbito do **Ecossistema SisTer**.

O sistema conduz pesquisadores e equipes científicas por sessões estruturadas, documentadas, criticáveis e reprodutíveis de formulação de problemas científicos, mantendo fidelidade estrita ao protocolo original e garantindo que a tecnologia assista a exploração heurística sem jamais substituir o julgamento científico humano.

## 2. Princípios Fundamentais e Invariantes

1. **Movimentos Revisáveis sobre Etapas Rígidas**: Os movimentos do MPRR (*Deslocar*, *Relacionar*, *Problematizar*, *Confrontar* e *Reposicionar*) funcionam como funções analíticas vivas, permitindo retornos, revisões e bifurcações através de requisitos de consolidação verificáveis.
2. **Automação Exploratória e Crítica com Núcleo Autônomo**: A assistência computacional atua na ampliação de perspectivas e no contraponto crítico rigoroso, mas o núcleo de domínio do sistema opera de modo 100% funcional mesmo sem qualquer motor de inteligência artificial ativado.
3. **Contratos Epistêmicos Inegociáveis**: Distinção rigorosa entre *integridade procedimental* (o sistema atesta o rigor com que o percurso foi percorrido) e *validação científica substantiva* (que pertence exclusivamente ao confronto empírico e à comunidade de pesquisadores).
4. **Proveniência Categórica**: Toda intervenção, seja de origem humana, assistida, importada ou deduzida, é selada com hashes criptográficos SHA-256 e registrada em eventos imutáveis auditáveis.

---

## 3. Especificações do Sistema

O primeiro marco formal do projeto é constituído pelo conjunto de especificações canônicas localizadas em `docs/`:

| Código | Documento | Finalidade |
|---|---|---|
| **[MPRR-UX-001](docs/MPRR-UX-001.md)** | **Especificação da Experiência e Interface** | Constituição do sistema, transcrição literal do protocolo, mapa de telas, fluxo das duas portas, ciclo de vida, wireframes e critérios de aceite. |
| **[MPRR-INV-001](docs/MPRR-INV-001.md)** | **Especificação de Invariantes e Contratos** | Os 8 invariantes ontológicos e metodológicos inegociáveis que o software impõe e garante matematicamente. |
| **[MPRR-AST-001](docs/MPRR-AST-001.md)** | **Especificação do Motor de Assistência** | Fronteira, papéis, salvaguardas, catálogo de funções de apoio e protocolo de interação homem-máquina para exploração e crítica. |

---

## 4. Identidade Visual (SisTer Git / MORFO)

O MIRANTE herda os tokens visuais consolidados na família de software SisTer:

- **Sidebar e Estrutura Profunda**: Navy e Deep Navy (`#071e38`, `#0c2844`)
- **Acentos Primários**: Azul Científico (`#1a7dc4`) e Céu (`#2ea8e8`)
- **Relações e Conexões**: Teal e Esmeralda (`#087f8c`, `#14805e`)
- **Perspectiva Deslocada e Alerta**: Âmbar e Dourado Metodológico (`#ad7841`, `#e09f3e`)
- **Invalidação e Risco**: Rubro de Contradição (`#d9534f`, `#b22222`)
- **Fundos e Contraste**: Cinza Científico Claro (`#edf2f8`, `#f0f4f6`) e Warm Light (`#fbf7f0`)
- **Tipografia**: *Plus Jakarta Sans* / *Inter* para interface e prosa; *JetBrains Mono* para identificadores, hashes e versões.

---

## 5. Arquitetura Técnica C++26

O backend é modularizado em bibliotecas C++26 com servidor HTTP embutido multithread (`cpp-httplib`), persistência transacional auditável via SQLite3 (com Merkle-chain hashing SHA-256) e integração assíncrona desacoplada ao daemon local Ollama:

```text
MIRANTE/
├── CMakeLists.txt
├── README.md
├── docs/
│   ├── MPRR-UX-001.md         (Especificação Canônica da Experiência e Interface)
│   ├── MPRR-INV-001.md        (Especificação Formal de Invariantes e Contratos)
│   └── MPRR-AST-001.md        (Especificação do Motor de Assistência Local)
├── include/
│   └── mirante/
│       ├── domain/            (Entidades, agregados, tipos e invariantes INV-01 a INV-08)
│       ├── storage/           (Repositório SQLite3 com ledger de eventos e cadeia Merkle)
│       ├── assist/            (Cliente Ollama desacoplado e prompts metodológicos)
│       └── http/              (Endpoints REST/JSON e servidor de arquivos web)
├── src/
│   ├── domain/
│   ├── storage/
│   ├── assist/
│   ├── http/
│   └── main.cpp
├── web/                       (Interface web SisTer: HTML5 semântico, CSS3 e JS nativo)
│   ├── index.html
│   ├── style.css
│   └── app.js
├── tests/                     (Testes de integridade, transações atômicas e invariantes)
└── third_party/               (cpp-httplib e nlohmann/json)
```

---

## 6. Compilação, Testes e Execução

### Requisitos
- Compilador C++26 (`g++` >= 14 ou `clang++` >= 18)
- CMake >= 3.25
- SQLite3 (`libsqlite3-dev` / `sqlite-devel`)
- OpenSSL (`libssl-dev` / `openssl-devel`)
- Ollama local (opcional, para assistência com IA local)

### Compilação
```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Executar a Bateria de Testes
```bash
./build/mirante-tests
```

### Iniciar o Servidor
```bash
# Execução padrão (escuta em 0.0.0.0:8088 para acesso em rede local):
./build/mirante-server --host 0.0.0.0 --port 8088

# Com token de autenticação opcional:
MIRANTE_API_TOKEN="meu-token-seguro" ./build/mirante-server --host 0.0.0.0 --port 8088
```

A interface pode ser acessada em qualquer navegador da rede em: `http://localhost:8088` ou `http://<ip-do-servidor>:8088`.
A documentação interativa Swagger UI está disponível em: `http://localhost:8088/docs`.

---

## 7. Integração com ChatGPT, Claude e Agentes (OpenAPI e MCP)

O MIRANTE disponibiliza duas vias soberanas para interação com modelos de linguagem externos (ChatGPT, Claude, Gemini) e agentes autônomos de pesquisa:

### 7.1. ChatGPT Custom Actions e APIs HTTP (OpenAPI 3.1.0)
- **Especificação OpenAPI**: `http://localhost:8088/api/openapi.json` (ou no IP de rede `http://<ip-servidor>:8088/api/openapi.json`)
- **Documentação Interativa (Swagger UI)**: `http://localhost:8088/docs`
- **Configuração no ChatGPT**:
  1. No ChatGPT, acesse **My GPTs** -> **Create a GPT** -> **Configure** -> **Actions** -> **Create new action**.
  2. Clique em **Import from URL** e insira `http://<seu-ip>:8088/api/openapi.json`.
  3. O ChatGPT importará automaticamente todas as ferramentas do protocolo MPRR (criar sessão, deslocar perspectiva, formular questões, registrar confronto e auditar o ledger).

### 7.2. Model Context Protocol (MCP) Nativo em C++26 (`mirante-mcp`)
O MIRANTE inclui o executável nativo `./build/mirante-mcp`, que implementa a especificação oficial **JSON-RPC 2.0 stdio** do MCP para comunicação direta com **Claude Desktop**, **Antigravity**, **Cursor** e orquestradores de agentes.

#### Ferramentas Expostas pelo Servidor MCP:
1. `list_sessions`: Lista todas as sessões registradas com ciclo atual.
2. `get_session`: Recupera o estado completo da investigação (M1 a C5).
3. `create_session`: Inicia sessão MPRR escolhendo Caminho A ou B e enquadrando o fenômeno.
4. `shift_perspective`: Registra deslocamento de perspectiva (M1, INV-07).
5. `add_candidate_relation`: Adiciona relação candidata na bancada (M2, INV-01).
6. `formulate_question`: Formula questão científica com via de evidência necessária (M3).
7. `record_confrontation`: Submete a hipótese a explicações rivais e riscos epistêmicos (C4, INV-02).
8. `consolidate_reposition`: Consolida decisão de reposicionamento e próximo teste empírico (C5, INV-06, INV-08).
9. `verify_audit_ledger`: Audita matematicamente a integridade criptográfica da cadeia Merkle SHA-256 (INV-03).

#### Exemplo de Configuração (`claude_desktop_config.json` ou `mcp_config.json`):
```json
{
  "mcpServers": {
    "mirante": {
      "command": "/caminho/para/MIRANTE/build/mirante-mcp",
      "args": ["/caminho/para/MIRANTE/mirante.sqlite3"]
    }
  }
}
```

---

## 8. Licença e Autoria

- **Copyright (C) 2026 José Pedro Trindade / Ecossistema SisTer**
- Software Científico Livre — Licença GPL v3.0.
