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
        return p;
    }
    MixDown() : AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true)
                    .withOutput("Output",juce::AudioChannelSet::stereo(),true)), state(*this,nullptr,"MixDown",layout()) {}
    juce::AudioProcessorValueTreeState state;
    std::array<std::atomic<float>,2> inLevel{}, outLevel{};
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
            std::array<float,2> ip{},op{};
            for(int i=0;i<n;++i)
            {
                float gain=inputGain.getNextValue(),bm=bassMix.getNextValue(),tm=trebleMix.getNextValue();
                std::array<float,2> samples{};
                for(int c=0;c<channels;++c)
                {
                    const auto raw=buffer.getSample(c,start+i);
                    delay[delayPos][c]=raw;
                    dry.setSample(c,i,delay[(delayPos+1)%delay.size()][c]);
                    auto x=raw*gain; ip[c]=juce::jmax(ip[c],std::abs(x));
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
                    buffer.setSample(c,start+i,x); op[c]=juce::jmax(op[c],std::abs(x));
                }
            }
            for(int c=0;c<2;++c) { inLevel[c].store(ip[c]); outLevel[c].store(op[c]); }
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
        static constexpr float bank[8][9]={{0,0,0,0,1,1,1,1,0},{0,0,0,0,1,1,1,1,0},{-4.2f,4.4f,1.4f,7.1f,1,1,1,1,0},{0,6,1,0,0,1,1,0,0},{0,3,1,-3,1,1,1,1,0},{0,6,0,6,0,1,1,1,0},{6,0,0,0,1,1,1,0,1},{0,0,10,10,0,0,1,1,0}};
        i=juce::jlimit(0,7,i); const char* ids[]={"bass","treble","glue","drive","bassOn","trebleOn","glueOn","driveOn","mode"};
        for(int j=0;j<9;++j) set(ids[j],bank[i][j]); set("output",0); set("meter",1); program=i;
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

class Panel final : public juce::AudioProcessorEditor,private juce::Timer
{
public:
    explicit Panel(MixDown& p):AudioProcessorEditor(p),processor(p)
    {
        const char* ids[]={"input","bass","treble","glue","drive","output"};
        for(int i=0;i<6;++i)
        {
            auto& s=faders[i]; s.setSliderStyle(juce::Slider::LinearVertical);
            s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,75,22); addAndMakeVisible(s);
            sliders[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,ids[i],s);
            s.setDoubleClickReturnValue(true,0); s.setColour(juce::Slider::thumbColourId,juce::Colour(0xffeee4c8));
            s.setName(ids[i]); s.setTooltip(ids[i]);
        }
        for(int i=0;i<4;++i)
        { const char* idsOn[]={"bassOn","trebleOn","glueOn","driveOn"}; addAndMakeVisible(on[i]); on[i].setButtonText("ON"); buttons[i]=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,idsOn[i],on[i]); }
        addAndMakeVisible(mode); mode.addItemList({"Glue 1","Glue 2"},1);
        modeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"mode",mode);
        addAndMakeVisible(meter); meter.addItemList({"IN","GR","OUT"},1);
        meterAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"meter",meter);
        addAndMakeVisible(presets); for(int i=0;i<8;++i) presets.addItem(p.getProgramName(i),i+1);
        presets.setSelectedId(p.getCurrentProgram()+1,juce::dontSendNotification);
        presets.onChange=[this]{processor.setCurrentProgram(presets.getSelectedId()-1);};
        addAndMakeVisible(link); link.setButtonText("Link I/O");
        linkAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"link",link);
        for(int index:{0,5})
        {
            faders[index].onDragStart=[this,index]{ last=faders[index].getValue(); };
            faders[index].onValueChange=[this,index]{
                if(linking || !faders[index].isMouseButtonDown() || processor.value("link")<.5f) return;
                const double current=faders[index].getValue(),delta=current-last; last=current;
                juce::ScopedValueSetter<bool> guard(linking,true);
                auto* param=processor.state.getParameter(index==0?"output":"input");
                param->beginChangeGesture(); processor.set(index==0?"output":"input",juce::jlimit(-12.f,12.f,processor.value(index==0?"output":"input")-float(delta))); param->endChangeGesture();
            };
        }
        addAndMakeVisible(bypass); bypass.setButtonText("Bypass");
        bypassAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"bypass",bypass);
        setSize(1000,630); startTimerHz(30);
    }
    void resized() override
    {
        presets.setBounds(200,18,260,28); bypass.setBounds(835,18,100,28);
        const int x[]={68,300,425,550,675,900};
        for(int i=0;i<6;++i) faders[i].setBounds(x[i]-40,i==0||i==5?260:315,80,i==0||i==5?300:245);
        for(int i=0;i<4;++i) on[i].setBounds(x[i+1]-25,270,60,28);
        mode.setBounds(525,575,100,24); meter.setBounds(480,130,80,28); link.setBounds(35,575,120,28);
    }
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff16191a));
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff555757),0,0,juce::Colour(0xff252728),0,630,false));
        g.fillRoundedRectangle(8,8,984,614,18);
        g.setColour(juce::Colour(0xff90918d)); g.drawRoundedRectangle(8,8,984,614,18,3);
        g.setColour(juce::Colours::white); g.setFont(25); g.drawText("MixDown Study",30,65,400,35,juce::Justification::left);
        g.setFont(12); g.drawText("Independent prototype / AulaMix",650,70,300,25,juce::Justification::right);
        const char* labels[]={"INPUT","BASS","TREBLE","GLUE","DRIVE","OUTPUT"}; const int xs[]={68,300,425,550,675,900};
        for(int i=0;i<6;++i) g.drawText(labels[i],xs[i]-45,550,90,22,juce::Justification::centred);
        for(int c=0;c<2;++c)
        {
            float x=c==0?270.f:580.f; juce::Rectangle<float> face(x,120,200,125);
            g.setColour(juce::Colours::black);g.fillRoundedRectangle(face.expanded(8),10);
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xffe7d6a9),x,120,juce::Colour(0xffaf956b),x,245,false));g.fillRoundedRectangle(face,5);
            g.setColour(juce::Colour(0xff3b3021));g.setFont(14);g.drawText(c==0?"VU  L":"VU  R",face.toNearestInt(),juce::Justification::centred);
            const auto modeIndex=int(processor.value("meter"));
            float db=modeIndex==1?-processor.gr.load():juce::Decibels::gainToDecibels((modeIndex==0?processor.inLevel[c]:processor.outLevel[c]).load(),-80.f)+18;
            float angle=juce::jmap(juce::jlimit(-20.f,3.f,db),-20.f,3.f,-1.f,1.f);
            float px=x+100,py=238;
            for(int t=0;t<=12;++t) { auto a=-1.f+float(t)/6; g.drawLine(px+std::sin(a)*78,py-std::cos(a)*78,px+std::sin(a)*88,py-std::cos(a)*88,1); }
            g.setColour(juce::Colours::black);g.drawLine(px,py,px+std::sin(angle)*90,py-std::cos(angle)*90,2);
            for(int s=0;s<26;++s)
            { float level=processor.outLevel[c].load();bool lit=juce::Decibels::gainToDecibels(level,-80.f)>-26+s;g.setColour(lit?juce::Colours::lawngreen:juce::Colour(0xff20361d));g.fillRect(c==0?180:810,530-s*8,12,6); }
        }
        g.setColour(juce::Colour(0xffaaaaaa));g.setFont(11);g.drawText("DSP approximation - not the original Waves algorithm",640,592,330,18,juce::Justification::right);
    }
private:
    void timerCallback() override { repaint(); }
    MixDown& processor; double last=0; bool linking=false;
    std::array<juce::Slider,6> faders;
    std::array<juce::ToggleButton,4> on;
    juce::ToggleButton link,bypass;
    juce::ComboBox presets,mode,meter;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,6> sliders;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>,4> buttons;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment,meterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> linkAttachment,bypassAttachment;
};
juce::AudioProcessorEditor* MixDown::createEditor() { return new Panel(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MixDown(); }
