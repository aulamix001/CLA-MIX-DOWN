# Windows x64

Versão 0.2: painel com arte de 1592 × 988, metal texturizado, faders claros,
botões iluminados, LEDs estéreo e agulhas VU animadas. O PNG original é
incorporado byte a byte, sem redimensionamento ou edição do arquivo. Os
controles móveis são desenhados durante a execução. O DSP continua sendo
uma aproximação independente, não o algoritmo Waves.

No GitHub, abra Releases e baixe `MixDown-Study-Windows-x64.zip` na versão
Windows x64 visual panel. O pacote é publicado somente após compilação e testes.

Alternativamente, abra Actions → Windows VST3 → execução concluída → Artifacts →
MixDown-Study-Windows-x64. É necessário login GitHub para baixar artifacts.
Descompacte o ZIP de artifacts e o ZIP do produto nele contido.

Copie a pasta completa `MixDown Study.vst3` para
`C:\Program Files\Common Files\VST3\`. Não extraia somente o binário interno.
Atualize a lista de plugins na DAW. A versão Standalone pode ser executada
diretamente pelo `MixDown Study.exe`.

Se instalou a versão anterior, substitua a pasta `.vst3` completa com a DAW
fechada e refaça a busca de plugins. O identificador do plugin foi preservado.

A janela abre com 1060 pixels de largura e mantém a proporção do painel.
O menu superior permite 50%, 67% e 100% (1592 pixels). O arquivo de imagem
permanece na resolução original em todos os tamanhos de janela.

Controles: arraste os faders; Shift permite ajuste fino; duplo clique volta a
zero. IN/GR/OUT seleciona a fonte dos VUs; 1/2 seleciona o modo Glue. Arraste
os parafusos dos VUs para calibrar (8–24 dB); duplo clique retorna a 18 dB.
Clique nos medidores de LED para limpar o indicador de sobrecarga.
Undo/Redo, presets anterior/próximo, cópia e alternância A/B, Save e Load
estão disponíveis na toolbar. Save/Load usam arquivos `.mixdown`.

Para compilar localmente, instale Visual Studio 2022 com C++ Desktop, SDK Windows
e CMake. Execute na raiz do checkout:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target MixDownStudy_VST3 MixDownStudy_Standalone MixDownSmoke MixDownUiSmoke --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

Os presets reproduzem os valores públicos dos XMLs. Input é preservado nas
trocas de preset; sessão da DAW restaura todos os parâmetros. O link inverso
atua durante arraste dos faders. O processamento não foi comparado ao original.
