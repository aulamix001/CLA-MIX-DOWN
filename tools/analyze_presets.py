#!/usr/bin/env python3
"""Inspeciona XMLs de referência sem alterar entradas ou arquivos de imagem.

Mantém tokens decimais e '*' literalmente. Não implementa os algoritmos DSP
proprietários e não executa nenhum conteúdo dos documentos.
"""

import argparse
import csv
import hashlib
import json
from decimal import Decimal, InvalidOperation
from pathlib import Path
import xml.etree.ElementTree as ET


METER_CLASSES = {"AttenuationMeter", "MeterWithClip"}
PANEL_IDS = (31, 32, 33, 34, 35, 37, 38, 39, 41, 42, 44, 51, 56, 57, 59)


def read_xml(path):
    raw = path.read_bytes()
    if len(raw) > 10 * 1024 * 1024:
        raise ValueError(f"XML acima do limite de 10 MiB: {path}")
    if b"<!DOCTYPE" in raw.upper() or b"<!ENTITY" in raw.upper():
        raise ValueError(f"DTD/entidades não são aceitas: {path}")
    return raw, ET.fromstring(raw)


def literal_tokens(text):
    tokens = (text or "").split()
    for token in tokens:
        if token != "*":
            try:
                valid = Decimal(token).is_finite()
            except InvalidOperation:
                valid = False
            if not valid:
                raise ValueError(f"Token numérico inválido: {token!r}")
    return tokens


def element_record(element):
    """Representação genérica que preserva atributos, filhos e texto útil."""
    return {
        "tag": element.tag,
        "attributes": dict(element.attrib),
        "text": (element.text or "").strip(),
        "children": [element_record(child) for child in element],
    }


def parameter_schema(root):
    parameters = root.findall("Parameter")
    by_name = {p.get("Name"): p for p in parameters}

    def inherited(p, query, default=None, visited=None):
        visited = set() if visited is None else visited
        if p.get("Name") in visited:
            raise ValueError("Herança circular de parâmetros")
        visited.add(p.get("Name"))
        value = p.findtext(query)
        if value is not None:
            return value.strip()
        parent = p.get("Parent")
        if parent:
            if parent not in by_name:
                raise ValueError(f"Parâmetro pai ausente: {parent}")
            return inherited(by_name[parent], query, default, visited)
        return default

    result = []
    for index, p in enumerate(parameters):
        # ElementTree não suporta consultas de atributos com findtext.
        source = p
        visited = set()
        while source.find("ExtensionClass") is None and source.get("Parent"):
            if source.get("Name") in visited:
                raise ValueError("Herança circular de parâmetros")
            visited.add(source.get("Name"))
            source = by_name[source.get("Parent")]
        extension = source.find("ExtensionClass")
        extension_type = extension.get("Type") if extension is not None else None
        display_only = p.find("Flags/DisplayOnlyParam") is not None
        parent = p.get("Parent")
        if parent:
            display_only |= by_name[parent].find("Flags/DisplayOnlyParam") is not None
        meter = display_only or extension_type in METER_CLASSES
        item = {
            "index": index,
            "name": p.get("Name"),
            "parent": parent,
            "label": inherited(p, "Label"),
            "minimum": inherited(p, "Scale/Minimum"),
            "maximum": inherited(p, "Scale/Maximum"),
            "resolution": inherited(p, "Scale/Resolution"),
            "scale_type": inherited(p, "Scale/ScaleType"),
            "initial": inherited(p, "InitialValue"),
            "reset": inherited(p, "ResetValue"),
            "units": inherited(p, "Units"),
            "display_function": inherited(p, "DisplayFunction"),
            "display_only_flag": display_only,
            "meter": meter,
            "extension_type": extension_type,
            "ignore_on_preset_load": inherited(p, "IgnoreOnPresetLoad"),
            "display_strings": [dict(v.attrib) for v in p.findall("DisplayStrings/RealWorld")],
            "definition": element_record(p),
        }
        result.append(item)
    return result


def parse_preset(preset, source):
    result = {
        "source": source,
        "name": preset.get("Name"),
        "generic_type": preset.get("GenericType"),
        "header": {c.tag: (c.text or "").strip() for c in preset.findall("PresetHeader/*")},
        "setups": [],
    }
    for data in preset.findall("PresetData"):
        params = data.find("Parameters")
        tokens = literal_tokens(params.text if params is not None else "")
        spec = data.find("PluginSpecificXMLData")
        setup = {
            "attributes": dict(data.attrib),
            "parameter_type": params.get("Type") if params is not None else None,
            "tokens": tokens,
            "token_count": len(tokens),
            "slots": [],
            "dynamic_controls": [],
            "control_laws": [],
            "meter_laws": [],
        }
        if spec is not None:
            for control in spec.findall("DynamicControlData"):
                setup["dynamic_controls"].append({
                    "name": control.get("ControlName"),
                    **{c.tag: (c.text or "").strip() for c in control},
                })
            for key, query in (("control_laws", "link_laws_table/link_law"),
                               ("meter_laws", "meter_laws_table/link_law")):
                for law in spec.findall(query):
                    setup[key].append({
                        **law.attrib,
                        **{c.tag: (c.text or "").strip() for c in law},
                    })
            for slot in spec.findall("slot"):
                sd = slot.find("slot_data")
                if sd is None:
                    continue
                record = {
                    "index": int(slot.get("index")),
                    "plugin": sd.findtext("plugin_name"),
                    "plugin_id": sd.findtext("plugin_id"),
                    "bypass": sd.findtext("plugin_bypass"),
                    "inputs": [e.text.strip() for e in slot.findall("SlotArtistInputs/ArtistInput")],
                    "embedded_presets": [],
                }
                embedded = sd.findtext("plugin_preset", "").strip()
                if embedded:
                    inner = ET.fromstring(embedded)
                    record["embedded_root"] = inner.tag
                    record["embedded_presets"] = [
                        parse_preset(p, source) for p in inner.findall("Preset")
                    ]
                setup["slots"].append(record)
        result["setups"].append(setup)
    return result


def gui_controls(root):
    return {
        "view_rects": [dict(r.attrib) for r in root.findall("ViewRect_List/Rect")],
        "base_images": [dict(i.attrib) for i in root.findall("Pict_List/Image")],
        "controls": [
            {
                "name": c.get("Name"), "subcomponent": c.get("SubComp"),
                "class": c.find("Class").get("Type") if c.find("Class") is not None else None,
                "rect": dict(c.find("ControlRect").attrib) if c.find("ControlRect") is not None else None,
                "parameters": [dict(p.attrib) for p in c.findall("ParameterList/Param")],
                "resources": [dict(i.attrib) for i in c.iter("Image")],
                "definition": element_record(c),
            }
            for c in root.findall("Control_List/cCONTROLLER")
        ],
    }


def analyze(paths):
    result = {"sources": [], "schema": [], "presets": [], "gui": [], "components": []}
    for path in paths:
        raw, root = read_xml(path)
        source = str(path.resolve())
        result["sources"].append({
            "path": source, "root": root.tag,
            "bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest(),
        })
        if root.tag == "Parameter_List":
            if result["schema"]:
                raise ValueError("Mais de um schema: selecione explicitamente a versão desejada")
            result["schema"] = parameter_schema(root)
        elif root.tag == "PresetXMLTree":
            result["presets"].extend(parse_preset(p, source) for p in root.findall("Preset"))
        elif root.tag == "GUITree":
            result["gui"].append({"source": source, **gui_controls(root)})
        else:
            result["components"].append({"source": source, "definition": element_record(root)})

    schema = result["schema"]
    for preset in result["presets"]:
        for setup in preset["setups"]:
            named = []
            for p in schema:
                index = p["index"]
                present = index < len(setup["tokens"])
                raw_value = setup["tokens"][index] if present else None
                state = "absent" if not present else "unspecified" if raw_value == "*" else "value"
                is_panel = index in PANEL_IDS
                policy = "keep_current" if state != "value" or p["ignore_on_preset_load"] else (
                    "telemetry_only" if p["meter"] else "apply_control" if is_panel else "internal_state")
                named.append({
                    "index": index, "name": p["name"], "raw": raw_value,
                    "state": state, "role": "meter" if p["meter"] else "panel" if is_panel else "internal",
                    "load_policy": policy,
                })
            setup["named_parameters"] = named
            setup["missing_trailing_parameters"] = [p["name"] for p in schema[len(setup["tokens"]):]]
            setup["extra_tokens"] = setup["tokens"][len(schema):] if schema else []
            for c in setup["dynamic_controls"]:
                index = int(c["ParamID"])
                if index >= len(schema):
                    raise ValueError(f"ParamID {index} fora do schema")
                c["schema_name"] = schema[index]["name"]
    return result


def write_reports(result, output):
    output.mkdir(parents=True, exist_ok=True)
    (output / "reference-analysis.json").write_text(
        json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    fields = ["preset", "input", "output", "bass", "treble", "glue", "drive",
              "bass_on", "treble_on", "drive_on", "glue_state", "glue_display_on",
              "glue_selection", "vu_cal", "meter_mode", "link_io"]
    positions = [42, 35, 32, 33, 31, 34, 37, 38, 39, 41, 56, 57, 44, 51, 59]
    with (output / "presets.csv").open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(fields)
        for preset in result["presets"]:
            if preset["header"].get("PluginName") != "CLA MixDown":
                continue
            for setup in preset["setups"]:
                tokens = setup["tokens"]
                writer.writerow([preset["name"]] + [tokens[i] if i < len(tokens) else "ABSENT" for i in positions])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", type=Path, nargs="+")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    paths = []
    for path in args.inputs:
        paths.extend(sorted(path.rglob("*.xml")) if path.is_dir() else [path])
    if not paths:
        parser.error("Nenhum XML encontrado")
    if any(args.output.resolve() == p.resolve() or args.output.resolve() in p.resolve().parents for p in paths):
        parser.error("O diretório de saída deve ser separado dos documentos originais")
    result = analyze(paths)
    write_reports(result, args.output)
    print(f"{len(paths)} XMLs; {len(result['schema'])} parâmetros; {len(result['presets'])} presets/estruturas.")
    print(f"Relatórios: {args.output}")


if __name__ == "__main__":
    main()
