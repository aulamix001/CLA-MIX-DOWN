import importlib.util
import unittest
from pathlib import Path
import xml.etree.ElementTree as ET


MODULE = Path(__file__).resolve().parents[1] / "tools" / "analyze_presets.py"
SPEC = importlib.util.spec_from_file_location("analysis", MODULE)
analysis = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(analysis)


class PresetAnalysisTests(unittest.TestCase):
    def test_wildcards_and_decimal_precision_are_not_reinterpreted(self):
        value = "* 1.3999999999999999112 -7.7620931989930852524 0"
        self.assertEqual(analysis.literal_tokens(value), value.split())
        for invalid in ("NaN", "Infinity", "garbage", "1,4"):
            with self.assertRaises(ValueError):
                analysis.literal_tokens(invalid)

    def test_child_meter_inherits_read_only_classification_and_scale(self):
        root = ET.fromstring('''<Parameter_List>
          <Parameter Name="L"><Scale><Minimum>-141</Minimum><Maximum>3.1</Maximum></Scale>
          <Flags><DisplayOnlyParam/></Flags><ExtensionClass Type="MeterWithClip"/></Parameter>
          <Parameter Name="R" Parent="L"/>
        </Parameter_List>''')
        p = analysis.parameter_schema(root)[1]
        self.assertTrue(p["meter"])
        self.assertEqual(p["minimum"], "-141")
        self.assertEqual(p["maximum"], "3.1")

    def test_dynamic_control_values_are_not_nested_plugin_values(self):
        root = ET.fromstring('''<Preset Name="P"><PresetHeader><PluginName>CLA MixDown</PluginName></PresetHeader>
          <PresetData Setup="CURRENT"><Parameters Type="RealWorld">* 0 1</Parameters>
          <PluginSpecificXMLData><slot index="0"><SlotArtistInputs><ArtistInput>-10 0</ArtistInput></SlotArtistInputs>
          <slot_data><plugin_name>Internal</plugin_name><plugin_preset><![CDATA[
          <PresetChunkXMLTree><Preset Name="Internal"><PresetData Setup="SETUP_A">
          <Parameters Type="RealWorld">-13 1.6 250</Parameters>
          </PresetData></Preset></PresetChunkXMLTree>]]></plugin_preset></slot_data></slot>
          </PluginSpecificXMLData></PresetData></Preset>''')
        result = analysis.parse_preset(root, "test")
        self.assertEqual(result["setups"][0]["tokens"], ["*", "0", "1"])
        inner = result["setups"][0]["slots"][0]["embedded_presets"][0]
        self.assertEqual(inner["setups"][0]["tokens"], ["-13", "1.6", "250"])

    def test_cyclic_parameter_inheritance_fails_instead_of_hanging(self):
        root = ET.fromstring('''<Parameter_List><Parameter Name="A" Parent="B"/>
        <Parameter Name="B" Parent="A"/></Parameter_List>''')
        with self.assertRaises(ValueError):
            analysis.parameter_schema(root)


if __name__ == "__main__":
    unittest.main()
