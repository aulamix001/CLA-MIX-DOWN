#include "Panel.h"

juce::AudioProcessorEditor* MixDown::createEditor() { return new Panel(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MixDown(); }
