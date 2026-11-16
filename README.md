# CLA-MIX-DOWN

Base técnica para uma implementação VST3/Standalone em C++/JUCE, com controles
orientados pelo manual e pelos XMLs fornecidos pelo usuário.

Estado atual: protótipo JUCE com controles automatizáveis, oito presets,
processamento aproximado e workflow de compilação Windows x64. A interface
é desenhada em código; ainda não reproduz o painel de referência. Não há
equivalência sonora demonstrada com o produto original.

- [Compilação e download do VST3 Windows](docs/WINDOWS.md)

- [Análise dos presets e controles](docs/ANALISE_PRESETS.md)
- [Tabela dos oito presets](docs/analysis/presets.csv)
- [Extração completa e fontes com SHA-256](docs/analysis/reference-analysis.json)

```sh
python3 tools/analyze_presets.py /workspace/attachments --output docs/analysis
python3 -m unittest discover -s Tests -v
```

Imagens existentes são imutáveis. O PNG original está em
`/workspace/generated_images/exec-a5bf11cb-f48d-464e-862a-d64669108d61.png`,
com SHA-256 `915abf671b0a41b716282ad2fd4eeed8b47011dcdb38f0c00430b02cea562a86`.
Nenhuma imagem foi copiada, renomeada, convertida, recortada ou substituída
durante a análise.
