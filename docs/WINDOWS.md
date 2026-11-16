# Windows x64

Protótipo independente em JUCE 8.0.12. O DSP é uma aproximação, não o algoritmo
Waves. A interface é desenhada em código; as imagens originais permanecem
intactas. Esta versão não promete equivalência visual ou sonora.

No GitHub, abra Releases e baixe `MixDown-Study-Windows-x64.zip` na versão
Windows x64 prototype. O pacote é publicado somente após compilação e testes.

Alternativamente, abra Actions → Windows VST3 → execução concluída → Artifacts →
MixDown-Study-Windows-x64. É necessário login GitHub para baixar artifacts.
Descompacte o ZIP de artifacts e o ZIP do produto nele contido.

Copie a pasta completa `MixDown Study.vst3` para
`C:\Program Files\Common Files\VST3\`. Não extraia somente o binário interno.
Atualize a lista de plugins na DAW. A versão Standalone pode ser executada
diretamente pelo `MixDown Study.exe`.

Para compilar localmente, instale Visual Studio 2022 com C++ Desktop, SDK Windows
e CMake. Execute na raiz do checkout:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target MixDownStudy_VST3 MixDownStudy_Standalone --parallel 2
```

Os presets reproduzem os valores públicos dos XMLs. Input é preservado nas
trocas de preset; sessão da DAW restaura todos os parâmetros. O link inverso
atua durante arraste dos faders. O processamento não foi comparado ao original.
