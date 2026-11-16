#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

// Independent DSP approximation; the XMLs do not contain the Waves algorithms.
class MixDown final : public juce::AudioProcessor
{
public:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout p;
        for (auto id : {"input", "output", "bass", "treble", "glue", "drive"})
        {
            bool io = juce::String(id) == "input" || juce::String(id) == "output";
            p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, id,
                juce::NormalisableRange<float>(io ? -12.f : -10.f, io ? 12.f : 10.f, .1f), 0.f));
        }
        for (auto id : {"bassOn", "trebleOn", "glueOn", "driveOn", "link", "bypass"})
            p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id, 1}, id,
                juce::String(id).endsWith("On")));
        p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"mode",1}, "Glue mode",juce::StringArray{"1","2"},0));
        p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"meter",1}, "Meter",juce::StringArray{"IN","GR","OUT"},1));
        p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"vuCal",1}, "VU calibration",
            juce::NormalisableRange<float>(8.f,24.f,1.f),18.f));
        return p;
    }
    MixDown() : AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true)
                    .withOutput("Output",juce::AudioChannelSet::stereo(),true)), state(*this,&history,"MixDown",layout()) {}
    juce::UndoManager history{10000};
    juce::AudioProcessorValueTreeState state;
    std::array<std::atomic<float>,2> inLevel{}, outLevel{};
    std::array<std::atomic<float>,2> inPower{}, outPower{};
    std::atomic<bool> inputClip{false},outputClip{false};
    std::atomic<float> gr{0};
    float value(const char* id) const { return state.getRawParameterValue(id)->load(); }
    void set(const char* id,float v) { auto* p=state.getParameter(id); p->setValueNotifyingHost(p->convertTo0to1(v)); }
    const juce::String getName() const override { return "MixDown Study"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    bool isBusesLayoutSupported(const BusesLayout& l) const override
    { return l.getMainInputChannelSet()==l.getMainOutputChannelSet() && (l.getMainOutputChannelSet()==juce::AudioChannelSet::mono() || l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo()); }
    void prepareToPlay(double rate,int block) override
    {
        sr=rate; envelope=0; reduction=0;
        for(int c=0;c<2;++c) { inLevel[c]=0;outLevel[c]=0;inPower[c]=0;outPower[c]=0; }
        inputClip=false;outputClip=false;
        for(auto& f:low) f.reset(); for(auto& f:high) f.reset();
        for(auto* s:{&inputGain,&outputGain,&bassMix,&trebleMix,&glueMix,&driveMix,&bypassMix})
            s->reset(rate,.025);
        driveMix.reset(rate*4,.025);
        inputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(value("input")));
        outputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(value("output")));
        bassMix.setCurrentAndTargetValue(value("bassOn")); trebleMix.setCurrentAndTargetValue(value("trebleOn"));
        glueMix.setCurrentAndTargetValue(value("glueOn")); driveMix.setCurrentAndTargetValue(value("driveOn"));
        bypassMix.setCurrentAndTargetValue(value("bypass"));
        oversampling=std::make_unique<juce::dsp::Oversampling<float>>(2,2,juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,true,true);
        oversampling->initProcessing(static_cast<size_t>(juce::jmax(1,block)));
        maximumBlock=juce::jmax(1,block);
        setLatencySamples(static_cast<int>(oversampling->getLatencyInSamples()));
        dry.setSize(2,maximumBlock); delay.assign(static_cast<size_t>(getLatencySamples()+1),{}); delayPos=0;
    }
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals noDenormals;
        if(!oversampling) return;
        const int channels=juce::jmin(2,buffer.getNumChannels());
        // Hosts may supply larger blocks than announced; process bounded slices.
        for(int start=0;start<buffer.getNumSamples();start+=maximumBlock)
        {
            const int n=juce::jmin(maximumBlock,buffer.getNumSamples()-start);
            auto bass=juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr,100.f,.707f,juce::Decibels::decibelsToGain(value("bass")*.29f+2.3f));
            auto treble=juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr,juce::jmin(8000.f,float(sr*.4)),.65f,juce::Decibels::decibelsToGain(value("treble")*.6f+2.65f));
            for(int c=0;c<2;++c) { low[c].coefficients=bass; high[c].coefficients=treble; }
            inputGain.setTargetValue(juce::Decibels::decibelsToGain(value("input")));
            outputGain.setTargetValue(juce::Decibels::decibelsToGain(value("output")));
            bassMix.setTargetValue(value("bassOn")); trebleMix.setTargetValue(value("trebleOn"));
            glueMix.setTargetValue(value("glueOn")); driveMix.setTargetValue(value("driveOn")); bypassMix.setTargetValue(value("bypass"));
            std::array<float,2> ip{},op{},ips{},ops{};
            for(int i=0;i<n;++i)
            {
                float gain=inputGain.getNextValue(),bm=bassMix.getNextValue(),tm=trebleMix.getNextValue();
                std::array<float,2> samples{};
                for(int c=0;c<channels;++c)
                {
                    const auto raw=buffer.getSample(c,start+i);
                    delay[delayPos][c]=raw;
                    dry.setSample(c,i,delay[(delayPos+1)%delay.size()][c]);
                    auto x=raw*gain; ip[c]=juce::jmax(ip[c],std::abs(x));ips[c]+=x*x;
                    x+=bm*(low[c].processSample(x)-x); x+=tm*(high[c].processSample(x)-x); samples[c]=x;
                }
                delayPos=(delayPos+1)%delay.size();
                const float detector=juce::jmax(std::abs(samples[0]),std::abs(samples[1]));
                const bool slow=value("mode")>.5f;
                const float coeff=std::exp(-1.f/float(sr*(detector>envelope ? (slow?.03:.01):(slow?.35:.1))));
                envelope=coeff*envelope+(1-coeff)*detector;
                const float threshold=-18.f-value("glue")*1.8f;
                reduction=juce::jmax(0.f,juce::Decibels::gainToDecibels(envelope,-120.f)-threshold)*(slow?.5f:.75f);
                const float gm=glueMix.getNextValue();
                const float comp=1+gm*(juce::Decibels::decibelsToGain(-reduction)-1);
                for(int c=0;c<channels;++c) buffer.setSample(c,start+i,samples[c]*comp);
            }
            juce::dsp::AudioBlock<float> full(buffer);
            auto slice=full.getSubBlock(static_cast<size_t>(start),static_cast<size_t>(n));
            auto up=oversampling->processSamplesUp(slice);
            const float amount=juce::Decibels::decibelsToGain((value("drive")+10)*.6f);
            for(size_t i=0;i<up.getNumSamples();++i)
            {
                const float mix=driveMix.getNextValue();
                for(int c=0;c<channels;++c)
                { auto* x=up.getChannelPointer(static_cast<size_t>(c)); x[i]+=mix*(std::tanh(x[i]*amount)/amount-x[i]); }
            }
            oversampling->processSamplesDown(slice);
            for(int i=0;i<n;++i)
            {
                const float og=outputGain.getNextValue(),bp=bypassMix.getNextValue();
                for(int c=0;c<channels;++c)
                {
                    auto x=buffer.getSample(c,start+i);
                    x=x*og*(1-bp)+dry.getSample(c,i)*bp;
                    buffer.setSample(c,start+i,x); op[c]=juce::jmax(op[c],std::abs(x));ops[c]+=x*x;
                }
            }
            const float rmsDecay=std::exp(-float(n)/float(sr*.3));
            auto peak=[](std::atomic<float>& dest,float value) {
                auto previous=dest.load(std::memory_order_relaxed);
                while(previous<value && !dest.compare_exchange_weak(previous,value,std::memory_order_relaxed)) {}
            };
            for(int c=0;c<2;++c)
            {
                peak(inLevel[c],ip[c]);peak(outLevel[c],op[c]);
                inPower[c].store(rmsDecay*inPower[c].load()+(1-rmsDecay)*ips[c]/float(n));
                outPower[c].store(rmsDecay*outPower[c].load()+(1-rmsDecay)*ops[c]/float(n));
                if(ip[c]>=1.f)inputClip=true;
                if(op[c]>=1.f)outputClip=true;
            }
            gr.store(reduction*value("glueOn"));
        }
    }
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    int getNumPrograms() override { return 8; }
    int getCurrentProgram() override { return program; }
    const juce::String getProgramName(int i) override
    { return juce::StringArray{"Start Me Up","Full Reset","Drive Me Home","Clearview","Bounce Master","Tear the Top Off","The Big Man","Mr Squeeze"}[juce::jlimit(0,7,i)]; }
    void changeProgramName(int,const juce::String&) override {}
    void setCurrentProgram(int i) override
    {
        history.beginNewTransaction("Load preset");
        static constexpr float bank[8][9]={{0,0,0,0,1,1,1,1,0},{0,0,0,0,1,1,1,1,0},{-4.2f,4.4f,1.4f,7.1f,1,1,1,1,0},{0,6,1,0,0,1,1,0,0},{0,3,1,-3,1,1,1,1,0},{0,6,0,6,0,1,1,1,0},{6,0,0,0,1,1,1,0,1},{0,0,10,10,0,0,1,1,0}};
        i=juce::jlimit(0,7,i); const char* ids[]={"bass","treble","glue","drive","bassOn","trebleOn","glueOn","driveOn","mode"};
        for(int j=0;j<9;++j) set(ids[j],bank[i][j]); set("output",0); set("meter",1);set("vuCal",18); program=i;
    }
    void getStateInformation(juce::MemoryBlock& b) override
    { auto tree=state.copyState(); tree.setProperty("program",program,nullptr); auto xml=tree.createXml(); copyXmlToBinary(*xml,b); }
    void setStateInformation(const void* d,int n) override
    { if(auto xml=getXmlFromBinary(d,n)) if(xml->hasTagName("MixDown")) { auto t=juce::ValueTree::fromXml(*xml); program=juce::jlimit(0,7,int(t.getProperty("program",0))); state.replaceState(t); } }
private:
    int program=0,maximumBlock=1; double sr=48000; float envelope=0,reduction=0;
    std::array<juce::dsp::IIR::Filter<float>,2> low,high;
    juce::SmoothedValue<float> inputGain,outputGain,bassMix,trebleMix,glueMix,driveMix,bypassMix;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::AudioBuffer<float> dry;
    std::vector<std::array<float,2>> delay; size_t delayPos=0;
};

