# Análise dos XMLs de CLA MixDown

Foram analisados oito XMLs anexados, sem modificar os originais ou qualquer
imagem. Os arquivos contêm oito presets de usuário/fábrica, duas estruturas
iniciais (mono/estéreo), o schema de parâmetros, o layout da interface e
metadados do plugin. Não são oito implementações diferentes de processamento.

## Fontes e identificação

Os nomes se repetem; a identificação depende da raiz XML, do tamanho e do SHA-256,
nunca apenas de `1000.xml` ou `1001.xml`.

| Arquivo | Bytes | Raiz | Conteúdo |
| --- | ---: | --- | --- |
| Primeiro `1000.xml` | 5828 | `PresetXMLTree` | 8 presets, componente estéreo `CMBS` |
| `1050.xml` | 121144 | `PresetXMLTree` | `Initial Mono` e `Initial Stereo`, módulos e leis de controle |
| `1000.xml` | 36872 | `Parameter_List` | 60 parâmetros, nomes, faixas, defaults e regras de carregamento |
| `1000.xml` | 8229 | `ComponentDefinitions` | Definições genéricas de formatos e plataformas |
| `1001.xml` | 2949 | `ComponentDefinitions` | Definições específicas mono/estéreo do CLA MixDown |
| `1001.xml` | 337 | `cALGORITHMs` | Identidade `CMBU`, nome e 32 níveis de undo |
| `1000.xml` | 29145 | `GUITree` | Layout 738 × 474 e 32 definições de controles, incluindo alternativas mono/estéreo |
| `1001.xml` | 4515 | `ToolbarXML` | Presets, Undo/Redo, comparação A/B, Save, Help e posições |

Os caminhos completos e hashes estão em `analysis/reference-analysis.json`.
O conteúdo dos documentos foi interpretado como dados técnicos, não como
instruções para executar comandos.

## Presets decodificados

Valores abaixo são as posições dos controles na escala mostrada no plugin.
Os quatro macros variam de −10 a +10, com resolução de 0,1. O schema não declara
unidade em dB para esses macros. Eles comandam múltiplos parâmetros internos;
por exemplo, Bass +6 não significa automaticamente um shelf de +6 dB.

| Preset | Bass | Treble | Glue | Drive | Bass ativo | Treble ativo | Glue ativo/modo | Drive ativo |
| --- | ---: | ---: | ---: | ---: | :---: | :---: | :---: | :---: |
| Start Me Up | 0 | 0 | 0 | 0 | Sim | Sim | 1 | Sim |
| Full Reset | 0 | 0 | 0 | 0 | Sim | Sim | 1 | Sim |
| Drive Me Home | −4,2 | +4,4 | +1,4 | +7,1 | Sim | Sim | 1 | Sim |
| Clearview | 0 | +6 | +1 | 0 | Não | Sim | 1 | Não |
| Bounce Master | 0 | +3 | +1 | −3 | Sim | Sim | 1 | Sim |
| Tear the Top Off | 0 | +6 | 0 | +6 | Não | Sim | 1 | Sim |
| The Big Man | +6 | 0 | 0 | 0 | Sim | Sim | 2 | Não |
| Mr Squeeze | 0 | 0 | +10 | +10 | Não | Não | 1 | Sim |

Em todos os oito arquivos de preset: Input armazenado = 0, Output = 0,
VU Cal = 18 e seletor principal = 1 (GR). Input tem regra específica de
preservação no carregamento: o 0 armazenado não autoriza zerar o ganho atual.
Link I/O não está presente nesses presets; não há valor de link a extrair deles.

`Start Me Up` e `Full Reset` diferem somente nos índices 36 e 40, leituras GR.
Portanto, os valores de controle sonoro salvos nesses dois presets são iguais.
Não é correto interpretar esses valores GR como ganhos de compensação.

`Full Reset` também não é bypass: todas as seções continuam ligadas. Posição
zero nos macros não garante resposta neutra. As leis de Bass e Treble já
mostram ganhos internos positivos na região central.

O CSV preserva os tokens decimais completos recebidos. A tabela acima arredonda
somente a apresentação; não altera os XMLs.

## Mapa confirmado dos parâmetros

Índices abaixo começam em zero e contam os tokens `*` como posições reais.
Os IDs foram cruzados com `DynamicControlData/ParamID` em `1050.xml`.

| Índice | Nome | Faixa | Inicial | Função |
| ---: | --- | --- | ---: | --- |
| 31 | `PARAM_GLUE` | −10…+10, passo 0,1 | 0 | Intensidade do macro Glue |
| 32 | `PARAM_BASS` | −10…+10, passo 0,1 | 0 | Macro de graves |
| 33 | `PARAM_TREBLE` | −10…+10, passo 0,1 | 0 | Macro de agudos |
| 34 | `PARAM_DRIVE` | −10…+10, passo 0,1 | 0 | Macro de saturação/coloração |
| 35 | `PARAM_OUTPUT` | −12…+12, passo 0,1 | 0 | Ganho de saída |
| 36 | `PARAM_GR_L` | −30…0 | 0 | Leitura GR do EMO-D5, canal L |
| 37 | `PARAM_BASS_ONOFF` | 0/1 | 1 | Bass desligado/ligado |
| 38 | `PARAM_TREBLE_ONOFF` | 0/1 | 1 | Treble desligado/ligado |
| 39 | `PARAM_DRIVE_ONOFF` | 0/1 | 1 | Drive desligado/ligado |
| 40 | `PARAM_GR_R` | −30…0 | 0 | Leitura GR do EMO-D5, canal R |
| 41 | `PARAM_GLUE_ONOFF_AND_GR` | 0/1/2 | 1 | Glue off / modo 1 / modo 2 |
| 42 | `PARAM_INPUT` | −12…+12, passo 0,1 | 0 | Ganho de entrada; preservar ao carregar preset |
| 43 | `PARAM_HEQ_BP_STEREO` | 0/1 | 0 | Controle interno dos H-EQs, estéreo |
| 44 | `PARAM_VU_CAL` | 8…24, passo 1 | 18 | Headroom/calibração dos VUs |
| 45/46 | `PARAM_GR_L_SSL` / `PARAM_GR_R_SSL` | 0…20 dB | 0 | Leituras GR do SSLComp |
| 47 | `PARAM_METER_INPUT_OUPUT` | 0…1 no schema | 0 | Controle interno do mux IN/OUT; não é o seletor público |
| 48/49 | `PARAM_INOUT_L` / `PARAM_INOUT_R` | −141…+3,1 | 0 | Leituras dos VUs |
| 50 | `PARAM_HEQ_BP_MONO` | 0/1 | 0 | Controle interno dos H-EQs, mono |
| 51 | `PARAM_METER_IO` | 0/1/2 | 1 | IN / GR / OUT |
| 52/53 | `PARAM_DISPLAY_METER_INPUT_OUPUT_LEFT/RIGHT` | −141…+3,1 | −44 | Exibição dos VUs; direito herda do esquerdo |
| 54/55 | `PARAM_DISPLAY_METER_GR_LEFT/RIGHT` | −141…+3,1 | 0 | Exibição de redução de ganho |
| 56 | `PARAM_GLUE_ONOFF_DISPLAY` | 0/1 | 1 | Botão visual ON do Glue |
| 57 | `PARAM_GR_SELECTION` | 0/1 | 0 | Seletor visual: 0 mostra “1”, 1 mostra “2” |
| 58 | `PARAM_DISPLAY_METER_GR_MAX` | −141…+3,1 | 0 | Exibição GR máxima |
| 59 | `PARAM_LINK_IO` | 0/1 | 0 | Vínculo inverso Input/Output |

O schema tem 60 posições, mas os presets e estruturas recebidos têm 59 tokens
(índices 0 a 58). Não deslocar índices para compensar: o campo 59 está ausente.
Em uma instância nova o default do link é 0; ao importar um preset antigo,
preservar o estado atual de um campo ausente evita inventar dados.

Há parâmetros de medição sem a flag `DisplayOnlyParam` explícita: os índices
36 e 40 têm `ExtensionClass Type="AttenuationMeter"`. Sua classe e o
`meter_laws_table` confirmam que são telemetria.

`*` significa que não há valor numérico fornecido naquele campo. Deve ser
preservado como não especificado; não converter para zero nem remover da lista.

## O que as leis internas revelam

`1050.xml` traz módulos embutidos e tabelas que relacionam os macros aos
controles internos. São configurações e pontos de mapeamento, não código DSP.

### Glue

O controle composto 41 determina os bypasses:

| Estado | Pré/pós-ganho do Glue | EMO-D5 | SSLComp |
| ---: | --- | --- | --- |
| 0 | Bypass | Bypass | Bypass |
| 1 | Ativos | Ativo | Bypass |
| 2 | Ativos | Bypass | Ativo |

Não é fiel substituir os dois modos apenas por dois tempos de release do mesmo
compressor. As estruturas nomeiam processadores diferentes.

O macro 31 controla ganho antes e depois da compressão. A tabela de pós-ganho
inclui aproximadamente +10,020 dB / +1,520 dB / −10 dB nos pontos de estado
0 / 500 / 1000 do macro. Logo, a mudança de intensidade envolve compensação
de nível. Um simples mapeamento do fader para threshold não reproduz essa lei.

Na estrutura estéreo há pares de instâncias mono de EMO-D5 e SSLComp, um por
canal. Os XMLs recebidos não comprovam por si só qual é a lei de acoplamento
dos detectores; não assumir automaticamente um detector estéreo por máximo.

### Bass e Treble

Ambos comandam o H-EQ da seção tonal (`CLA Pultec`).

| Macro | Destino nomeado | Estados de origem | Valores reais de destino |
| --- | --- | --- | --- |
| Bass | `LF Gain` | 0 / 496,487487… / 1000 | −0,1 / +2,3 / +5,7 |
| Treble | `HMF Gain` | 0 / 502,502502… / 1000 | −4,4 / +2,65 / +7,7 |

Esses estados são os pontos internos da tabela, não a escala visível −10…+10.
A interpolação exata do motor proprietário não foi demonstrada. Os pontos
centrais ficam próximos, mas não exatamente no estado 500.

O botão Bass liga/desliga `LF On/Off` e `MF On/Off`. O botão Treble controla
`HMF On/Off` e `HF On/Off`. Portanto, cada botão afeta mais de uma banda.
Não é justificado implementar os macros como um shelf e um bell independentes
de ganho igual à posição do fader sem reconhecer a aproximação.

### Drive

O macro 34 altera simultaneamente:

- Pré-ganho: −6…+6 dB; pós-ganho: +6…−6 dB.
- `NORM_MAX_GAIN`: pontos 12 / 30 / 60.
- `OUT_GAIN_SYNT`: −20…+20.
- Sete controles `POLYNOM_GAIN_BAND*`, com valores específicos por banda.
- Filtros do primeiro H-EQ: frequência HP, LP, Q e ganho MF.

Exemplos das tabelas: LP = 18000 / 18000 / 15154 Hz; HP = 16 / 16 / 20 Hz.
Não confundir os valores de Q internos (por exemplo 20 e 28) com Q convencional
sem o schema do módulo correspondente. Não inferir ordem de harmônico a partir
do nome `BAND1`, `BAND2`, etc.

Os blocos nomeados são `Polynom THD` e `Clip and meter`. O botão Drive também
controla o bypass do clipper, além do módulo THD e dos ganhos associados.
A existência desse bloco está comprovada; sua curva, limiar e oversampling
não podem ser determinados apenas pelo nome ou pelos tokens anônimos.

Uma saturação genérica por `tanh` pode servir como protótipo, mas não é a
implementação demonstrada pelos XMLs.

### Entrada, saída e medidores

O link associa Input e Output ao grupo 0 com `LinkFunction=Reverse`.
É um vínculo inverso de controles, não link de canais estéreo.

O parâmetro 42 contém `IgnoreOnPresetLoad=Input`: carregar um preset deve
preservar o ganho de entrada. A confirmação vem do schema, não de uma hipótese.

O modo dos VUs é o parâmetro 51: 0=In, 1=Gr, 2=Out. O headroom é 8…24,
default 18. A GUI usa o mesmo `PARAM_VU_CAL` nos dois parafusos de calibração.
Não criar calibrações independentes L/R.

Há divergências entre camadas da referência: as barras de entrada/saída têm
faixa −26…0 no schema; parâmetros de exibição incluem −24…0; e há VUs de
−141…+3,1. O controle interno 47 tem faixa 0…1 no schema e 0…100 no editor.
Essas escalas pertencem a controles diferentes; preservar as distinções e
validar a conversão antes de desenhar ou normalizar medidores.

## Interface e compatibilidade

A GUI declara 738 × 474. Ela informa retângulos, eventos de mouse, automação,
entrada de texto e referências por `ResID` às imagens. VUs de entrada/saída
analógicos usam uma sequência com 132 passos. O LED de sensibilidade referencia
24 passos; a calibração referencia 25 passos. Esses números são contagens de
recursos de animação, não tempos de resposta dos medidores.

O último `1001.xml` detalha a toolbar: About, Undo, Redo, Preset, PrevPreset,
NextPreset, Copy A→B/B→A, Switch, Save e Help. `cALGORITHMs` declara 32 níveis
de undo. Os arquivos descrevem controles; não fornecem a implementação desses
comandos.

As definições específicas de `CMBM`/`CMBS` declaram 1→1 e 2→2 canais,
sem MIDI e sem sidechain externo, até 96 kHz. O arquivo genérico contém
48 kHz e outras topologias; não deve sobrepor a definição específica do produto.

Os bitmaps individuais identificados pelos `ResID` não vieram nesses XMLs.
Os documentos não fornecem os pixels dos faders, sombras e agulhas. A imagem
gerada anteriormente permanece intacta, em seu caminho original. Não houve
recorte, reexportação, substituição ou alteração de metadados de imagem.

## O que pode ser reproduzido e o que ainda precisa de validação

Comprovado pelos arquivos: valores dos presets, nomes e faixas dos controles,
defaults, estados de bypass, seleção Glue, vínculo inverso I/O, preservação de
Input, fontes de telemetria, pontos das leis e referências de layout/recursos.

Ainda não comprovado: equivalência sonora, implementação e interpolação exatas
dos módulos embutidos, coeficientes de filtros, curvas de saturação, constante
de integração dos medidores, acoplamento estéreo e resposta dinâmica real.
Há presets internos serializados para H-EQ, EMO-D5, SSLComp, Polynom THD,
clipper e VU; muitos tokens não têm nomes no material recebido. Não atribuir
nomes de threshold, ratio ou attack a esses números sem o schema correspondente.

Esta análise não declara o VST3/Standalone implementado ou validado. Ela fornece
a especificação confirmada para a implementação dos controles solicitada antes,
evitando reproduzir interpretações incorretas do manual em baixa resolução.

## Reprodução da análise

Na raiz do repositório:

```sh
python3 tools/analyze_presets.py /workspace/attachments --output docs/analysis
python3 -m unittest discover -s Tests -v
```

O script usa somente a biblioteca padrão do Python. Lê os documentos originais,
rejeita DTD/entidades, mantém os tokens e a precisão decimal literal, resolve
herança de parâmetros e separa os presets internos dos controles externos.
Os relatórios são derivados e não sobrescrevem as fontes.
