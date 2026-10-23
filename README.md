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

O backend é modularizado em bibliotecas C++26 com servidor HTTP/WebSocket embutido (Boost.Asio/Beast) e persistência auditável via SQLite3:

```text
MIRANTE/
├── CMakeLists.txt
├── README.md
├── docs/
│   ├── MPRR-UX-001.md
│   ├── MPRR-INV-001.md
│   └── MPRR-AST-001.md
├── include/
│   └── mirante/
│       ├── domain/        (Entidades, agregados, invariantes do MPRR)
│       ├── application/   (Casos de uso: condução de sessão, consolidação, confronto)
│       ├── storage/       (Repositório SQLite3 com ledger de eventos)
│       ├── assist/        (Interface desacoplada de IA: IAssistProvider)
│       ├── export/        (Exportadores JSON canônico, HTML e PDF)
│       └── http/          (Endpoints REST/JSON e static file server)
├── src/
│   ├── domain/
│   ├── application/
│   ├── storage/
│   ├── assist/
│   ├── export/
│   ├── http/
│   └── main.cpp
├── web/                   (Interface web SisTer: HTML5, CSS3 puro, ES6 modular)
│   ├── index.html
│   ├── style.css
│   └── js/
└── tests/                 (Testes unitários de invariantes e testes de integração)
```

---

## 6. Licença e Autoria

- **Copyright (C) 2026 José Pedro Trindade / Ecossistema SisTer**
- Software Científico Livre — Licença GPL v3.0.
