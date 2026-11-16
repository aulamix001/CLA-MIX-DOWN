# CLA-MIX-DOWN

Base técnica para uma implementação VST3/Standalone em C++/JUCE, com controles
orientados pelo manual e pelos XMLs fornecidos pelo usuário.

Estado atual: painel em alta resolução baseado na referência, com metal,
faders em relevo, botões iluminados, VUs e LEDs animados. A arte de 1592 × 988
é incorporada sem modificar o PNG. Controles automatizáveis, oito presets,
comparação A/B, toolbar e compilação VST3/Standalone Windows x64 estão incluídos.
O DSP continua sendo uma aproximação independente; não há equivalência sonora
demonstrada com o produto original.

- [Compilação e download do VST3 Windows](docs/WINDOWS.md)
- [Origem e proteção da arte](docs/ASSETS.md)

- [Análise dos presets e controles](docs/ANALISE_PRESETS.md)
- [Tabela dos oito presets](docs/analysis/presets.csv)
- [Extração completa e fontes com SHA-256](docs/analysis/reference-analysis.json)

```sh
python3 tools/analyze_presets.py /workspace/attachments --output docs/analysis
python3 -m unittest discover -s Tests -v
```

Imagens existentes são imutáveis. O PNG original continua em
`/workspace/generated_images/exec-a5bf11cb-f48d-464e-862a-d64669108d61.png`,
com SHA-256 `915abf671b0a41b716282ad2fd4eeed8b47011dcdb38f0c00430b02cea562a86`.
Para distribuir o plugin, uma cópia byte a byte, com o mesmo nome e hash,
está versionada em `assets/`. Não houve edição, reexportação, substituição,
compressão ou alteração dos metadados internos da imagem. CMake verifica o
hash antes de compilar e incorpora os bytes originais no binário.

Testes JUCE: `ctest --test-dir build -C Release --output-on-failure`.
No Linux, o teste de interface precisa de um display X11, por exemplo Xvfb.
