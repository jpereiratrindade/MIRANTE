# Governança leve do MIRANTE

O MIRANTE distingue integridade procedimental de validação científica substantiva. Esta governança torna essa separação rastreável em cada mudança.

## Fluxo mínimo

Uma mudança relevante usa Issue, branch e Pull Request. A Issue registra problema, hipótese, invariantes afetadas, critérios de aceite e evidência esperada. O PR liga a implementação à Issue e registra a verificação executada.

## Estados independentes

- Engenharia: `proposto` → `implementado` → `testado` → `publicado`.
- Ciência: `hipótese` → `verificado computacionalmente` → `validado empiricamente`, ou `limitado/refutado`.

Os fluxos não são equivalentes. Uma invariante aprovada pela suíte demonstra conformidade do software ao contrato; não valida, sozinha, uma afirmação científica.

## Definição de concluído

Issue e PR estão ligados; critérios de aceite foram demonstrados; `ctest --test-dir build --output-on-failure` passou; invariantes relevantes possuem cobertura; documentação e limitações foram atualizadas; estado científico está explícito; e uma publicação identifica versão e commit.

## Versionamento

O `project(... VERSION ...)` de `CMakeLists.txt` é a fonte da versão do código. Tags usam `vMAJOR.MINOR.PATCH`. A versão implantada é o tag ou commit efetivamente executado. Especificações metodológicas são versionadas por seus próprios identificadores documentais.

## Etiquetas recomendadas

Criar no GitHub: `status:proposto`, `status:implementado`, `status:testado`, `status:publicado`, `ciencia:hipotese`, `ciencia:computacional`, `ciencia:validado`, `ciencia:limitado`, `tipo:bug`, `tipo:funcionalidade`, `tipo:metodologia`, `invariante` e `bloqueado`.

