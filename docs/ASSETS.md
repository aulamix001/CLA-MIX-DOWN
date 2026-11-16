# Arte imutável

Arquivo: `assets/exec-a5bf11cb-f48d-464e-862a-d64669108d61.png`

- Dimensões originais: 1592 × 988.
- SHA-256: `915abf671b0a41b716282ad2fd4eeed8b47011dcdb38f0c00430b02cea562a86`.
- Origem: imagem em alta resolução gerada anteriormente nesta conversa.
- Original mantido em `/workspace/generated_images/`, com o mesmo nome.
- A cópia no repositório tem exatamente os mesmos bytes e nome do original.

A captura mais recente do usuário serve como referência visual. O projeto não
dispõe dos bitmaps individuais identificados pelos ResIDs dos XMLs; a arte
versionada não é uma extração desses recursos do produto de referência.

Nenhum script redimensiona, otimiza, converte, recorta, recomprime, sobrescreve
ou altera os metadados internos do PNG. CMake calcula o SHA-256 e interrompe a
configuração se a imagem tiver mudado. `juce_add_binary_data` incorpora os
bytes originais no VST3 e Standalone, sem exigir arquivos externos na máquina
do usuário.

A interface decodifica a imagem original diretamente. O layout usa coordenadas
nativas e mantém uma única proporção quando a janela muda de tamanho. A imagem
de origem continua na resolução original; não são produzidas versões reduzidas.

Agulhas, escalas, LEDs, luzes e faders móveis são desenhados em tempo real.
Quando um fader sai do centro, a área do cap estacionado é repintada usando
desenhos da imagem completa original, com clipping de componentes gráficos.
Isso ocorre apenas na superfície de renderização; não produz nem modifica
qualquer arquivo de imagem de asset.

O teste de interface grava screenshots em `build/ui-snapshots/` para conferir
a renderização e os estados dos controles. São resultados de teste, não
substitutos ou versões editadas do PNG original.
