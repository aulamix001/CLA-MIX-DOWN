#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
#include <stdexcept>
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

class UiSmoke final : public juce::JUCEApplication,private juce::Timer
{
public:
    const juce::String getApplicationName() override { return "MixDown UI smoke"; }
    const juce::String getApplicationVersion() override { return "0.2.0"; }
    void initialise(const juce::String& args) override
    {
        output=juce::File::getCurrentWorkingDirectory().getChildFile("ui-snapshots");
        if(args.trim().isNotEmpty())output=juce::File(args.trim().unquoted());
        // Each run writes new outputs; neither original assets nor previous
        // screenshots are overwritten, and PNG streams never append old data.
        output=output.getChildFile("run-"+juce::String(juce::Time::currentTimeMillis()));
        output.createDirectory();
        processor.reset(createPluginFilter());processor->prepareToPlay(48000,256);
        editor.reset(processor->createEditor());editor->setVisible(true);
        startTimer(120);
    }
    void shutdown() override { stopTimer();editor.reset();processor.reset(); }
private:
    void require(bool yes,const char* text) { if(!yes)throw std::runtime_error(text); }
    juce::Component* find(juce::Component& c,const juce::String& id)
    {
        if(c.getComponentID()==id)return &c;
        for(auto* child:c.getChildren())if(auto* found=find(*child,id))return found;
        return nullptr;
    }
    juce::RangedAudioParameter& param(const char* id)
    {
        for(auto* p:processor->getParameters())if(auto* r=dynamic_cast<juce::RangedAudioParameter*>(p))if(r->paramID==id)return *r;
        throw std::runtime_error("Missing parameter");
    }
    float get(const char* id) { auto& p=param(id);return p.convertFrom0to1(p.getValue()); }
    template<class T> T& control(const char* id)
    {auto* c=dynamic_cast<T*>(find(*editor,id));require(c!=nullptr,id);return *c;}
    void click(const char* id) { control<juce::TextButton>(id).onClick(); }
    void feed()
    {
        juce::MidiBuffer midi;juce::AudioBuffer<float> buffer(2,256);
        for(int b=0;b<40;++b)
        {
            for(int i=0;i<256;++i)
            {float phase=float(i+b*256)*juce::MathConstants<float>::twoPi*800/48000;buffer.setSample(0,i,.27f*std::sin(phase));buffer.setSample(1,i,.18f*std::sin(phase));}
            processor->processBlock(buffer,midi);
        }
    }
    void capture(const juce::String& name)
    {
        auto image=editor->createComponentSnapshot(editor->getLocalBounds());
        require(image.isValid(),"Invalid UI snapshot");
        auto stream=output.getChildFile(name).createOutputStream();
        require(stream!=nullptr&&juce::PNGImageFormat{}.writeImageToStream(image,*stream),"Cannot save UI snapshot");
    }
    void timerCallback() override
    {
        try
        {
            if(step==0)
            {
                for(auto id:{"input","output","bass","treble","glue","drive"})control<juce::Slider>(id);
                for(auto id:{"bassOn","trebleOn","glueOn","driveOn","link"})control<juce::ToggleButton>(id);
                require(editor->getWidth()==1060,"Default editor width");
                control<juce::Slider>("bass").setValue(6,juce::sendNotificationSync);
                require(std::abs(get("bass")-6)<.01f,"Fader parameter binding");
                click("mode2");click("meter2");
                require(get("mode")==1&&get("meter")==2,"Meter/mode buttons");
                control<juce::ToggleButton>("bassOn").setToggleState(false,juce::sendNotification);
                require(get("bassOn")==0,"Section bypass binding");
                click("copy");click("setup");
                require(std::abs(get("bass")-6)<.01f,"A to B copy");
                control<juce::Slider>("bass").setValue(-7,juce::sendNotificationSync);
                click("setup");require(std::abs(get("bass")-6)<.01f,"A/B recall");
                click("next");require(processor->getCurrentProgram()==1,"Next preset");
                click("previous");require(processor->getCurrentProgram()==0,"Previous preset");
                // Reference panel state, all faders at their original centre.
                processor->setCurrentProgram(0);feed();
            }
            else if(step==1)
            {
                feed();capture("panel-default.png");
                editor->setSize(1592,1028);feed();
            }
            else if(step==2)
            {
                capture("panel-native-1592.png");
                control<juce::Slider>("bass").setValue(10,juce::sendNotificationSync);
                control<juce::Slider>("treble").setValue(-10,juce::sendNotificationSync);
                control<juce::Slider>("input").setValue(-12,juce::sendNotificationSync);
                control<juce::Slider>("output").setValue(12,juce::sendNotificationSync);
                control<juce::ToggleButton>("driveOn").setToggleState(false,juce::sendNotification);
                click("mode2");feed();
            }
            else if(step==3)
            {
                feed();capture("panel-controls-moved.png");
                require(std::abs(get("bass")-10)<.01f&&std::abs(get("treble")+10)<.01f,"Extreme slider values");
                editor->setSize(796,514);feed();
            }
            else
            {
                stopTimer();
                capture("panel-small-796.png");
                std::cout<<"PASS: artwork rendering at 3 sizes; faders; bypass; VU modes; Glue modes; A/B copy/recall; preset navigation.\n";
                setApplicationReturnValue(0);quit();
            }
            ++step;
        }
        catch(const std::exception& e){std::cerr<<e.what()<<'\n';setApplicationReturnValue(1);quit();}
    }
    int step=0;
    juce::File output;
    std::unique_ptr<juce::AudioProcessor> processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
};
START_JUCE_APPLICATION(UiSmoke)
