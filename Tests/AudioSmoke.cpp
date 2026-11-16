#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
int main()
{
    try
    {
        auto p=std::unique_ptr<juce::AudioProcessor>(createPluginFilter());
        auto parameter=[&](const char* name)->juce::RangedAudioParameter* {
            for(auto* x:p->getParameters()) if(auto* r=dynamic_cast<juce::RangedAudioParameter*>(x))
                if(r->paramID==name) return r;
            throw std::runtime_error("Missing parameter");
        };
        auto set=[&](const char* id,float v){auto* r=parameter(id);r->setValueNotifyingHost(r->convertTo0to1(v));};
        auto get=[&](const char* id){auto* r=parameter(id);return r->convertFrom0to1(r->getValue());};
        set("input",3); p->setCurrentProgram(2);
        if(std::abs(get("input")-3)>.01 || std::abs(get("drive")-7.1f)>.01) throw std::runtime_error("Preset import");
        juce::MemoryBlock state; p->getStateInformation(state); set("input",-6);
        p->setStateInformation(state.getData(),int(state.getSize()));
        if(std::abs(get("input")-3)>.01) throw std::runtime_error("State restore");
        for(double rate:{44100.,48000.,96000.})
        {
            p->prepareToPlay(rate,128); juce::MidiBuffer midi;
            for(int program=0;program<8;++program)
            {
                p->setCurrentProgram(program);
                juce::AudioBuffer<float> audio(2,257);
                for(int block=0;block<20;++block)
                {
                    for(int c=0;c<2;++c) for(int i=0;i<audio.getNumSamples();++i) audio.setSample(c,i,.2f*std::sin(float(i+block*257)*.07f));
                    p->processBlock(audio,midi);
                    for(int c=0;c<2;++c) for(int i=0;i<audio.getNumSamples();++i)
                        if(!std::isfinite(audio.getSample(c,i))) throw std::runtime_error("Nonfinite audio");
                }
            }
            set("bypass",1);
            juce::AudioBuffer<float> constant(2,128);
            for(int b=0;b<50;++b){for(int c=0;c<2;++c) for(int i=0;i<128;++i)constant.setSample(c,i,.125f);p->processBlock(constant,midi);}
            if(std::abs(constant.getSample(0,127)-.125f)>1e-5f) throw std::runtime_error("Bypass gain");
            set("bypass",0); p->releaseResources();
        }
        std::cout<<"PASS: 8 presets, 3 sample rates, oversized blocks, state restore, Input preservation, bypass.\n";
        return 0;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
