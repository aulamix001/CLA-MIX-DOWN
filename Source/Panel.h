#pragma once
#include "Processor.h"
#include <Artwork.h>

namespace hardware
{
inline constexpr int width=1592, bodyHeight=988, toolbarHeight=40, height=bodyHeight+toolbarHeight;
inline juce::Font font(float size,int style=juce::Font::plain)
{ return juce::Font(juce::FontOptions("Arial",size,style)); }

// All metal, moving parts and light are drawn at run time. The PNG is decoded
// directly from the original bytes, never cropped, re-exported or overwritten.
inline void metal(juce::Graphics& g,juce::Rectangle<float> r)
{
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff303334),r.getX(),0,
                                         juce::Colour(0xff3c3e3e),r.getRight(),0,false));
    g.fillRect(r);
    juce::Random random(18731);
    for(int y=int(r.getY());y<r.getBottom();y+=2)
    {
        g.setColour(juce::Colours::white.withAlpha(random.nextFloat()*.024f));
        g.drawHorizontalLine(y,r.getX(),r.getRight());
        g.setColour(juce::Colours::black.withAlpha(random.nextFloat()*.035f));
        g.drawHorizontalLine(y+1,r.getX()+random.nextFloat()*12,r.getRight());
    }
    juce::ColourGradient edge(juce::Colour(0xff1d2021).withAlpha(.22f),r.getX(),0,
                              juce::Colours::transparentBlack,r.getX()+12,0,false);
    g.setGradientFill(edge);g.fillRect(r.withWidth(12));
}

inline void screw(juce::Graphics& g,float x,float y,float radius=9)
{
    g.setColour(juce::Colour(0xff161819));g.fillEllipse(x-radius-3,y-radius-3,2*radius+6,2*radius+6);
    g.setColour(juce::Colour(0xff77796f));g.drawEllipse(x-radius-1,y-radius-1,2*radius+2,2*radius+2,1.5f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff62645d),x-radius,y-radius,juce::Colour(0xff080a09),x+radius,y+radius,false));
    g.fillEllipse(x-radius,y-radius,2*radius,2*radius);
    g.setColour(juce::Colour(0xff0a0b09));g.drawLine(x-radius*.5f,y+radius*.55f,x+radius*.5f,y-radius*.55f,3);
    g.setColour(juce::Colour(0xff94958c).withAlpha(.55f));g.drawLine(x-radius*.5f+2,y+radius*.55f,x+radius*.5f+2,y-radius*.55f,1);
}

class Look final : public juce::LookAndFeel_V4
{
public:
    Look()
    {
        int size=0;
        const auto* bytes=MixDownArtwork::getNamedResource(MixDownArtwork::namedResourceList[0],size);
        artwork=juce::ImageCache::getFromMemory(bytes,size);
        setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff202223));
        setColour(juce::PopupMenu::textColourId,juce::Colour(0xffddd9cf));
        setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour(0xff625d43));
        setColour(juce::TooltipWindow::backgroundColourId,juce::Colour(0xff292b2b));
        setColour(juce::TooltipWindow::textColourId,juce::Colour(0xffeee6cf));
    }
    int getSliderThumbRadius(juce::Slider&) override { return 0; }
    juce::Slider::SliderLayout getSliderLayout(juce::Slider& s) override
    {
        juce::Slider::SliderLayout l;
        const bool io=bool(s.getProperties()["io"]);
        l.sliderBounds={0,io?98:60,s.getWidth(),io?302:216};
        return l;
    }
    void drawLinearSlider(juce::Graphics& g,int,int,int,int,float position,float,float,
                          juce::Slider::SliderStyle,juce::Slider& s) override
    {
        const bool io=bool(s.getProperties()["io"]);
        const auto area=s.getLocalBounds().toFloat();
        const float cx=area.getCentreX();
        // At zero, the original photographed fader is already at the correct
        // position. Leave that artwork visible, including every surface detail.
        if(std::abs(s.getValue())<.0001)return;
        // Repaint the parked cap using the original neighbouring metal. Each
        // draw references the entire unchanged image; clipping belongs to the
        // live component, never to an edited/re-exported image file.
        const auto patch=juce::Rectangle<float>{cx-(io?40.f:36.f),io?181.f:108.f,io?92.f:84.f,io?145.f:131.f}.toNearestInt();
        const int originY=s.getY()-toolbarHeight;
        const int sourceX=s.getComponentID()=="output"?s.getX()-22:s.getX()+s.getWidth()/2+(io?48:42);
        for(int x=patch.getX();x<patch.getRight();x+=18)
        {
            juce::Graphics::ScopedSaveState clip(g);
            g.reduceClipRegion(x,patch.getY(),juce::jmin(18,patch.getRight()-x),patch.getHeight());
            g.drawImageAt(artwork,x-sourceX,-originY);
        }
        const float top=io?48.f:39.f,bottom=io?439.f:312.f;
        g.setColour(juce::Colour(0xff85857b).withAlpha(.5f));g.drawRoundedRectangle(cx-7,top-1,14,bottom-top+2,6,1);
        g.setColour(juce::Colour(0xff040606));g.fillRoundedRectangle(cx-5.5f,top,11,bottom-top,5);
        g.setColour(juce::Colours::black.withAlpha(.5f));g.drawLine(cx+6,top+6,cx+6,bottom-6,2);
        const float w=io?72.f:56.f,h=io?118.f:96.f;
        const juce::Rectangle<float> cap(cx-w/2,position-h/2,w,h);
        juce::DropShadow(juce::Colours::black.withAlpha(.8f),10,{6,7}).drawForRectangle(g,cap.toNearestInt());
        juce::Path shape;
        shape.startNewSubPath(cap.getX()+6,cap.getY());shape.lineTo(cap.getRight()-6,cap.getY());
        shape.lineTo(cap.getRight(),cap.getY()+18);shape.lineTo(cap.getRight()-3,cap.getBottom()-9);
        shape.lineTo(cap.getRight()-7,cap.getBottom());shape.lineTo(cap.getX()+7,cap.getBottom());
        shape.lineTo(cap.getX()+3,cap.getBottom()-9);shape.lineTo(cap.getX(),cap.getY()+18);shape.closeSubPath();
        juce::ColourGradient plastic(juce::Colour(0xfffbf7e1),0,cap.getY(),juce::Colour(0xffb4a17c),0,cap.getBottom(),false);
        plastic.addColour(.14,juce::Colour(0xffd1c29e));plastic.addColour(.45,juce::Colour(0xfff9f5df));
        plastic.addColour(.6,juce::Colour(0xffe0d5b4));plastic.addColour(.89,juce::Colour(0xfffbf6df));
        g.setGradientFill(plastic);g.fillPath(shape);
        g.setColour(juce::Colour(0xff77684c));g.strokePath(shape,juce::PathStrokeType(1.2f));
        for(float y=cap.getY()+12;y<cap.getBottom()-8;y+=9)
        {
            g.setColour(juce::Colour(0xff8f7e5a).withAlpha(.66f));g.drawLine(cap.getX()+5,y,cap.getRight()-5,y,1.3f);
            g.setColour(juce::Colour(0xfffffff0).withAlpha(.9f));g.drawLine(cap.getX()+5,y+2,cap.getRight()-5,y+2,1.8f);
        }
        g.setColour(juce::Colour(0xfff9f7e8));g.fillRect(cap.getX()+2,position-4,cap.getWidth()-4,8.f);
        g.setColour(io?juce::Colour(0xff151715):juce::Colour(0xffc8170c));
        g.fillRect(cap.getX()+2,position-1.5f,cap.getWidth()-4,3.f);
        g.setColour(juce::Colours::white.withAlpha(.65f));g.drawLine(cap.getX()+7,cap.getY()+3,cap.getRight()-7,cap.getY()+3,2);
        if(s.hasKeyboardFocus(false))
        { g.setColour(juce::Colour(0xffe5d6ac).withAlpha(.65f));g.drawRoundedRectangle(cap.expanded(3),5,1); }
    }
    void drawToggleButton(juce::Graphics& g,juce::ToggleButton& b,bool hover,bool down) override
    {
        auto r=b.getLocalBounds().toFloat().reduced(3);
        if(b.getComponentID()=="link")
        {
            g.setColour(juce::Colour(0xff171a1a));g.fillRect(r);
            g.setColour(juce::Colour(0xff8e8f85));g.drawRect(r,1);
            g.setColour(b.getToggleState()?juce::Colour(0xffffdd71):juce::Colour(0xffc1c3b9));
            g.drawRoundedRectangle(r.getCentreX()-12,r.getCentreY()-5,16,10,3,2);
            g.drawRoundedRectangle(r.getCentreX()-3,r.getCentreY()-5,16,10,3,2);
            return;
        }
        const bool on=b.getToggleState();
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff8a8174),0,0,juce::Colour(0xff141615),r.getRight(),r.getBottom(),false));
        g.fillRect(r);g.setColour(juce::Colour(0xff080a0a));g.drawRect(r,2);
        r=r.reduced(9);
        g.setColour(juce::Colours::black);g.fillRect(r.expanded(3));
        if(on)
        {
            juce::DropShadow(juce::Colour(0xffffc792).withAlpha(.36f),9,{}).drawForRectangle(g,r.toNearestInt());
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xfffffff8),0,r.getY(),juce::Colour(0xffe9c5a7),0,r.getBottom(),false));
        }
        else g.setGradientFill(juce::ColourGradient(juce::Colour(0xff89877c),0,r.getY(),juce::Colour(0xff474b47),0,r.getBottom(),false));
        g.fillRect(r);
        g.setColour(juce::Colours::white.withAlpha(on?.9f:.18f));g.drawRect(r.reduced(2),1.5f);
        if(hover) { g.setColour(juce::Colours::white.withAlpha(.06f));g.fillRect(r); }
        g.setFont(font(22,juce::Font::bold));g.setColour(on?juce::Colour(0xffbb1b0c):juce::Colour(0xffc2c4b9));
        g.drawText(on?"ON":"OFF",r.translated(0,down?1:0),juce::Justification::centred);
    }
    void drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool hover,bool down) override
    {
        auto r=b.getLocalBounds().toFloat().reduced(1);
        const auto role=b.getProperties()["role"].toString();
        const bool selected=b.getToggleState();
        if(role=="selector")
        {
            g.setColour(juce::Colour(0xff080a09));g.fillRect(r);
            r=r.reduced(3);
            auto top=selected?juce::Colour(0xfffce9a2):juce::Colour(0xff393d3b);
            auto bot=selected?juce::Colour(0xffad851d):juce::Colour(0xff101312);
            g.setGradientFill(juce::ColourGradient(top,0,r.getY(),bot,0,r.getBottom(),false));g.fillRect(r);
            g.setColour(selected?juce::Colour(0xffe2cc82):juce::Colour(0xff676d65));g.drawRect(r,1);
        }
        else if(role=="mode")
        {
            g.setColour(juce::Colour(0xff0a0c0b));g.fillRect(r);
            r=r.reduced(2);
            g.setGradientFill(juce::ColourGradient(selected?juce::Colour(0xfffff8e9):juce::Colour(0xff424640),0,r.getY(),selected?juce::Colour(0xffe4bca4):juce::Colour(0xff171a17),0,r.getBottom(),false));
            g.fillRect(r);
        }
        else
        {
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xff323535),0,0,juce::Colour(0xff161919),0,r.getBottom(),false));g.fillRect(r);
            g.setColour(b.isEnabled()?juce::Colour(0xffaaa99f):juce::Colour(0xff50534f));g.drawRect(r,1.5f);
        }
        if((hover||down)&&b.isEnabled()) {g.setColour(juce::Colours::white.withAlpha(down?.09f:.04f));g.fillRect(r);}
    }
    void drawButtonText(juce::Graphics& g,juce::TextButton& b,bool,bool) override
    {
        if(b.getComponentID()=="previous"||b.getComponentID()=="next")
        {
            const float direction=b.getComponentID()=="next"?1.f:-1.f;
            const auto c=b.getLocalBounds().toFloat().getCentre();
            g.setColour(juce::Colour(0xffc8c9bd));
            g.drawLine(c.x-direction*12,c.y,c.x+direction*10,c.y,4);
            juce::Path p;p.addTriangle(c.x+direction*16,c.y,c.x+direction*4,c.y-8,c.x+direction*4,c.y+8);g.fillPath(p);
            return;
        }
        const auto role=b.getProperties()["role"].toString();
        const bool selected=b.getToggleState();
        g.setFont(font(role=="selector"?24.f:role=="mode"?20.f:21.f));
        g.setColour(!b.isEnabled()?juce::Colour(0xff575c58):role=="mode"&&selected?juce::Colour(0xffa51b12):role=="selector"&&selected?juce::Colour(0xff18170f):juce::Colour(0xffc8c9bd));
        g.drawText(b.getButtonText(),b.getLocalBounds().reduced(2),juce::Justification::centred);
    }
    void drawComboBox(juce::Graphics& g,int w,int h,bool,int,int,int,int,juce::ComboBox&) override
    {
        g.setColour(juce::Colour(0xff202323));g.fillRect(0,0,w,h);
        g.setColour(juce::Colour(0xffaaa99f));g.drawRect(1.f,1.f,float(w)-2,float(h)-2,1.5f);
        g.drawVerticalLine(w-27,6.f,float(h)-6);
        juce::Path p;p.addTriangle(float(w)-21,14,float(w)-10,14,float(w)-15.5f,23);
        g.setColour(juce::Colour(0xffc4c5b9));g.fillPath(p);
    }
    juce::Font getComboBoxFont(juce::ComboBox&) override { return font(21); }
    void positionComboBoxText(juce::ComboBox& b,juce::Label& label) override
    { label.setBounds(7,2,b.getWidth()-37,b.getHeight()-4);label.setFont(font(21));label.setJustificationType(juce::Justification::centred); }
private:
    juce::Image artwork;
};

class VuFace final : public juce::Component,public juce::SettableTooltipClient
{
public:
    VuFace(MixDown& p,int c):processor(p),channel(c)
    { setComponentID(c==0?"vuL":"vuR");setTooltip("VU: 300 ms. Arraste para calibrar (8–24 dB). Duplo clique: 18 dB."); }
    float db=-20;
    static float angle(float d)
    { return juce::jmap(juce::Decibels::decibelsToGain(juce::jlimit(-20.f,3.f,d)),.1f,juce::Decibels::decibelsToGain(3.f),-1.04f,.94f); }
    void paint(juce::Graphics& g) override
    {
        auto face=getLocalBounds().toFloat().withTrimmedBottom(19);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xffa28a61),0,0,juce::Colour(0xfff0dfb6),0,face.getHeight(),false));
        g.fillRoundedRectangle(face,17);
        juce::ColourGradient warmth(juce::Colour(0xffffeac0).withAlpha(.52f),face.getCentreX(),face.getHeight()*.65f,juce::Colours::transparentBlack,0,0,true);
        g.setGradientFill(warmth);g.fillRoundedRectangle(face,17);
        juce::Random grain(15154+channel);
        for(int y=3;y<int(face.getHeight())-3;++y)
        {g.setColour(juce::Colour(0xff63563b).withAlpha(grain.nextFloat()*.025f));g.drawHorizontalLine(y,5.f,face.getRight()-5);}
        g.setColour(juce::Colour(0xff5b4832).withAlpha(.5f));g.drawRoundedRectangle(face.reduced(1),17,3);
        const float px=getWidth()*.5f,py=float(getHeight())-15;
        auto point=[&](float a,float radius){return juce::Point<float>{px+std::sin(a)*radius,py-std::cos(a)*radius};};
        auto arc=[&](float from,float to,float radius,juce::Colour colour,float weight)
        {
            juce::Path p;
            for(int i=0;i<=80;++i){auto pt=point(from+(to-from)*float(i)/80,radius);if(i==0)p.startNewSubPath(pt);else p.lineTo(pt);}
            g.setColour(colour);g.strokePath(p,juce::PathStrokeType(weight));
        };
        arc(angle(-20),angle(0),143,juce::Colour(0xff383027),2);
        arc(angle(0),angle(3),143,juce::Colour(0xffa52a23),7);
        const std::array<float,10> marks{-10,-7,-5,-3,-2,-1,0,1,2,3};
        g.setFont(font(16));
        for(auto mark:marks)
        {
            auto a=angle(mark);auto a1=point(a,143),a2=point(a,154),t=point(a,169);
            g.setColour(mark>0?juce::Colour(0xff8b2921):juce::Colour(0xff302a22));g.drawLine({a1,a2},1.5f);
            g.drawText((mark>0?"+":"")+juce::String(int(mark)),t.x-18,t.y-9,36,18,juce::Justification::centred,false);
        }
        g.setColour(juce::Colour(0xff554733));g.setFont(font(24,juce::Font::italic|juce::Font::bold));
        g.drawText("Chris Lord-Alge",0,83,getWidth(),32,juce::Justification::centred);
        g.drawHorizontalLine(113,75.f,float(getWidth())-75);
        g.setFont(font(25,juce::Font::italic));g.drawText(channel==0?"L":"R",0,114,getWidth(),34,juce::Justification::centred);
        g.setFont(font(16,juce::Font::bold));g.drawText("VU",18,64,38,25,juce::Justification::centred);
        g.drawText("VU",getWidth()-56,64,38,25,juce::Justification::centred);
        const auto end=point(angle(db),151);
        g.setColour(juce::Colours::black.withAlpha(.2f));g.drawLine(px+3,py+1,end.x+3,end.y+1,3.5f);
        g.setColour(juce::Colour(0xff252720));g.drawLine(px,py,end.x,end.y,2.8f);
        g.setColour(juce::Colours::white.withAlpha(.035f));g.fillRoundedRectangle(face.withHeight(23).reduced(5),12);
        screw(g,px,py,17);
    }
    void mouseDown(const juce::MouseEvent&) override
    { start=processor.value("vuCal");processor.state.getParameter("vuCal")->beginChangeGesture(); }
    void mouseDrag(const juce::MouseEvent& e) override
    { processor.set("vuCal",juce::jlimit(8.f,24.f,start-float(e.getDistanceFromDragStartY())/7)); }
    void mouseUp(const juce::MouseEvent&) override { processor.state.getParameter("vuCal")->endChangeGesture(); }
    void mouseDoubleClick(const juce::MouseEvent&) override
    { auto* p=processor.state.getParameter("vuCal");p->beginChangeGesture();processor.set("vuCal",18);p->endChangeGesture(); }
private:
    MixDown& processor;int channel;float start=18;
};

class LedMeter final : public juce::Component,public juce::SettableTooltipClient
{
public:
    LedMeter(MixDown& p,bool isInput):processor(p),input(isInput)
    { setComponentID(isInput?"inputMeter":"outputMeter");setTooltip("Medidor de pico estéreo (dBFS). Clique para limpar o indicador de sobrecarga."); }
    std::array<float,2> levels{};
    void paint(juce::Graphics& g) override
    {
        auto r=getLocalBounds().toFloat();g.setColour(juce::Colour(0xff101410));g.fillRect(r);
        g.setColour(juce::Colour(0xffa0a092));g.drawRect(r.reduced(1),1);
        g.setColour(juce::Colour(0xff363d32));g.drawRect(r.reduced(4),1);
        const bool clipped=(input?processor.inputClip:processor.outputClip).load();
        for(int c=0;c<2;++c)
        {
            const float x=c==0?10.f:float(getWidth())-27;
            for(int row=0;row<36;++row)
            {
                float y=float(getHeight())-15-float(row)*9;
                const float threshold=-26.f+float(row)*26.f/35;
                const bool on=juce::Decibels::gainToDecibels(levels[c],-100.f)>=threshold;
                auto colour=row<29?juce::Colour(0xff8bf233):row<33?juce::Colour(0xffd4ed50):juce::Colour(0xffe96935);
                if(!on)colour=juce::Colour(0xff24391b);
                g.setGradientFill(juce::ColourGradient(on?colour.brighter(.35f):colour,x,y,colour.darker(.28f),x,y+6,false));
                g.fillRect(x,y,17.f,6.5f);
                if(on){g.setColour(colour.withAlpha(.10f));g.fillRect(x-2,y-2,21.f,10.f);}
            }
            for(int j=0;j<4;++j)
            { g.setColour(clipped?juce::Colour(0xfffc3b24):juce::Colour(0xff642821));g.fillRect(x+float(j)*4,18.f,3.f,7.f); }
        }
        g.setFont(font(17,juce::Font::italic));g.setColour(juce::Colour(0xffe9e9da));
        for(int value:{0,6,12,18,24})
        {float y=value==0?14.f:float(getHeight())-18-(26.f-float(value))*315.f/26;g.drawText(juce::String(value),26,int(y)-4,getWidth()-52,23,juce::Justification::centred);}
    }
    void mouseDown(const juce::MouseEvent&) override
    { if(input)processor.inputClip=false;else processor.outputClip=false;repaint(); }
private:
    MixDown& processor;bool input;
};

class Sensitivity final : public juce::Component
{
public:
    float level=0;
    void paint(juce::Graphics& g) override
    {
        auto r=getLocalBounds().toFloat().reduced(2);g.setColour(juce::Colour(0xff0c100a));g.fillRoundedRectangle(r,7);
        g.setColour(juce::Colour(0xff7e8666));g.drawRoundedRectangle(r,7,2);r=r.reduced(5);
        const auto db=juce::Decibels::gainToDecibels(level,-100.f);
        auto c=db>=0?juce::Colour(0xffff3325):db>=-18?juce::Colour(0xffe2e335):db>-60?juce::Colour(0xff87cc3e):juce::Colour(0xff294821);
        g.setGradientFill(juce::ColourGradient(c.brighter(.6f),0,r.getY(),c.darker(.35f),0,r.getBottom(),false));g.fillRoundedRectangle(r,3);
        g.setColour(juce::Colours::white.withAlpha(db>-60?.7f:.05f));g.fillRoundedRectangle(r.reduced(3).withHeight(4),2);
    }
};

class Canvas final : public juce::Component
{
public:
    Canvas()
    {
        int size=0;const auto* bytes=MixDownArtwork::getNamedResource(MixDownArtwork::namedResourceList[0],size);
        background=juce::ImageCache::getFromMemory(bytes,size);
        jassert(background.getWidth()==width && background.getHeight()==bodyHeight);
        setSize(width,height);
    }
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff101211));
        g.drawImageAt(background,0,toolbarHeight); // Original full image, no source subrectangle.
        g.setColour(juce::Colour(0xff080a09));g.fillRect(0,0,width,toolbarHeight);
        g.setColour(juce::Colour(0xff8a8d85));g.drawHorizontalLine(toolbarHeight-1,0.f,float(width));
        juce::Rectangle<float> brand(3,3,388,33);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xfff0f0e8),0,3,juce::Colour(0xff8a8d85),0,36,false));g.fillRect(brand);
        g.setColour(juce::Colour(0xff373b36));g.setFont(font(22,juce::Font::bold));
        g.drawText("CHRIS LORD-ALGE COLLECTION",48,5,335,29,juce::Justification::centred);
        for(int i=0;i<3;++i)
        { juce::Path logo;float x=10.f+float(i)*11;logo.addTriangle(x,11,x+7,11,x+3.5f,27);g.fillPath(logo); }
    }
private:
    juce::Image background;
};
}

class Panel final : public juce::AudioProcessorEditor,private juce::Timer
{
public:
    explicit Panel(MixDown& p):AudioProcessorEditor(p),processor(p),leftVu(p,0),rightVu(p,1),inputMeter(p,true),outputMeter(p,false)
    {
        setLookAndFeel(&look);addAndMakeVisible(canvas);
        const char* ids[]={"input","bass","treble","glue","drive","output"};
        const char* names[]={"Input","Bass","Treble","Glue","Drive","Output"};
        for(int i=0;i<6;++i)
        {
            auto& s=faders[i];s.getProperties().set("io",i==0||i==5);s.setSliderStyle(juce::Slider::LinearVertical);
            s.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);s.setComponentID(ids[i]);s.setName(names[i]);
            s.setDoubleClickReturnValue(true,0);s.setPopupDisplayEnabled(true,true,&canvas);s.setScrollWheelEnabled(true);
            s.setTooltip(juce::String(names[i])+": arraste para ajustar; Shift = precisão; duplo clique = 0.");
            if(i==0||i==5)s.setTextValueSuffix(" dB");
            canvas.addAndMakeVisible(s);
            sliders[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,ids[i],s);
        }
        const char* toggles[]={"bassOn","trebleOn","glueOn","driveOn"};
        for(int i=0;i<4;++i)
        {
            auto& b=on[i];b.setComponentID(toggles[i]);b.setButtonText("ON");b.setTooltip(juce::String(names[i+1])+": liga/desliga a seção.");
            canvas.addAndMakeVisible(b);buttons[i]=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,toggles[i],b);
        }
        canvas.addAndMakeVisible(leftVu);canvas.addAndMakeVisible(rightVu);canvas.addAndMakeVisible(inputMeter);canvas.addAndMakeVisible(outputMeter);
        canvas.addAndMakeVisible(sensitivity);canvas.addAndMakeVisible(link);link.setComponentID("link");link.setTooltip("Link I/O: entrada e saída se movem em sentidos opostos.");
        linkAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"link",link);
        for(int i=0;i<2;++i)
        {
            auto& b=modes[i];b.setButtonText(juce::String(i+1));b.setComponentID("mode"+juce::String(i+1));b.getProperties().set("role","mode");
            b.setTooltip("Glue modo "+juce::String(i+1));canvas.addAndMakeVisible(b);
            b.onClick=[this,i]{modeAttachment->setValueAsCompleteGesture(float(i));};
        }
        modeAttachment=std::make_unique<juce::ParameterAttachment>(*p.state.getParameter("mode"),[this](float v){for(int i=0;i<2;++i)modes[i].setToggleState(int(v)==i,juce::dontSendNotification);},&p.history);
        modeAttachment->sendInitialUpdate();
        for(int i=0;i<3;++i)
        {
            auto& b=meters[i];b.setButtonText(juce::StringArray{"IN","GR","OUT"}[i]);b.setComponentID("meter"+juce::String(i));b.getProperties().set("role","selector");
            b.setTooltip("Fonte dos VUs: "+b.getButtonText());canvas.addAndMakeVisible(b);
            b.onClick=[this,i]{meterAttachment->setValueAsCompleteGesture(float(i));};
        }
        meterAttachment=std::make_unique<juce::ParameterAttachment>(*p.state.getParameter("meter"),[this](float v){for(int i=0;i<3;++i)meters[i].setToggleState(int(v)==i,juce::dontSendNotification);},&p.history);
        meterAttachment->sendInitialUpdate();
        for(int i:{0,5})
        {
            faders[i].onDragStart=[this,i]{last=faders[i].getValue();linkedGesture=processor.value("link")>.5f;if(linkedGesture)processor.state.getParameter(i==0?"output":"input")->beginChangeGesture();};
            faders[i].onValueChange=[this,i]{
                if(linking||!linkedGesture||!faders[i].isMouseButtonDown())return;
                const double current=faders[i].getValue(),delta=current-last;last=current;
                juce::ScopedValueSetter<bool> guard(linking,true);
                processor.set(i==0?"output":"input",juce::jlimit(-12.f,12.f,processor.value(i==0?"output":"input")-float(delta)));
            };
            faders[i].onDragEnd=[this,i]{if(linkedGesture)processor.state.getParameter(i==0?"output":"input")->endChangeGesture();linkedGesture=false;};
        }
        setupToolbar();
        setResizable(true,false);setResizeLimits(hardware::width/2,hardware::height/2,hardware::width,hardware::height);
        getConstrainer()->setFixedAspectRatio(double(hardware::width)/hardware::height);
        setSize(1060,juce::roundToInt(1060.*hardware::height/hardware::width));
        resized();captureCleanState();captureSetup(0);captureSetup(1);startTimerHz(30);
    }
    ~Panel() override { stopTimer();setLookAndFeel(nullptr); }
    void resized() override
    {
        const float scale=juce::jmin(float(getWidth())/hardware::width,float(getHeight())/hardware::height);
        canvas.setTransform(juce::AffineTransform::scale(scale));canvas.setTopLeftPosition(0,0);
        auto body=[](juce::Component& c,int x,int y,int w,int h){c.setBounds(x,y+hardware::toolbarHeight,w,h);};
        body(faders[0],114,390,106,488);body(faders[5],1379,390,106,488);
        const int centres[]={533,712,886,1066};
        for(int i=0;i<4;++i){body(faders[i+1],centres[i]-48,537,96,336);body(on[i],centres[i]-44,438,88,86);}
        body(leftVu,422,218,305,193);body(rightVu,868,218,305,193);
        body(inputMeter,262,446,82,374);body(outputMeter,1255,446,82,374);
        body(sensitivity,148,394,37,33);body(link,210,878,44,34);
        for(int i=0;i<2;++i)body(modes[i],940,449+i*36,44,36);
        for(int i=0;i<3;++i)body(meters[i],766,243+i*44,67,43);
        undo.setBounds(397,3,65,33);redo.setBounds(467,3,65,33);presets.setBounds(537,3,556,33);
        previous.setBounds(1098,3,69,33);next.setBounds(1172,3,69,33);copy.setBounds(1246,3,94,33);
        setup.setBounds(1345,3,94,33);save.setBounds(1444,3,93,33);menu.setBounds(1542,3,47,33);
    }
    void paint(juce::Graphics& g) override {g.fillAll(juce::Colour(0xff0b0d0c));}
private:
    void setupToolbar()
    {
        auto add=[this](juce::TextButton& b,const juce::String& label,const juce::String& id){b.setButtonText(label);b.setComponentID(id);canvas.addAndMakeVisible(b);};
        add(undo,juce::CharPointer_UTF8("↶"),"undo");add(redo,juce::CharPointer_UTF8("↷"),"redo");
        add(previous,juce::CharPointer_UTF8("◀"),"previous");add(next,juce::CharPointer_UTF8("▶"),"next");
        add(copy,juce::CharPointer_UTF8("A → B"),"copy");add(setup,"Setup A","setup");add(save,"Save","save");add(menu,juce::CharPointer_UTF8("☰"),"menu");
        undo.setTooltip("Desfazer");redo.setTooltip("Refazer");copy.setTooltip("Copia a configuração atual para a outra configuração A/B.");setup.setTooltip("Alterna entre as configurações A e B.");save.setTooltip("Salvar preset .mixdown");menu.setTooltip("Carregar preset, tamanho da interface, bypass e informações.");
        presets.setComponentID("presets");canvas.addAndMakeVisible(presets);
        for(int i=0;i<processor.getNumPrograms();++i)presets.addItem(processor.getProgramName(i),i+1);
        presets.onChange=[this]{int i=presets.getSelectedId()-1;if(i>=0){processor.setCurrentProgram(i);captureCleanState();}};
        previous.onClick=[this]{processor.setCurrentProgram((processor.getCurrentProgram()+7)%8);captureCleanState();};
        next.onClick=[this]{processor.setCurrentProgram((processor.getCurrentProgram()+1)%8);captureCleanState();};
        undo.onClick=[this]{processor.state.copyState();processor.history.undo();};
        redo.onClick=[this]{processor.state.copyState();processor.history.redo();};
        copy.onClick=[this]{captureSetup(1-activeSetup);};
        setup.onClick=[this]{captureSetup(activeSetup);activeSetup=1-activeSetup;auto& b=setupStates[size_t(activeSetup)];processor.setStateInformation(b.getData(),int(b.getSize()));captureCleanState();};
        save.onClick=[this]{fileDialog(true);};
        menu.onClick=[this]{showMenu();};
    }
    void showMenu()
    {
            juce::PopupMenu m;m.addItem(1,"Load preset…");m.addSeparator();
            m.addItem(2,"Bypass",true,processor.value("bypass")>.5f);m.addSeparator();
            m.addItem(3,"Interface 50%");m.addItem(4,"Interface 67%");m.addItem(5,"Interface 100% (1592 px)");m.addSeparator();m.addItem(6,"About MixDown Study");
            const juce::Component::SafePointer<Panel> safe(this);
            m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&menu),[safe](int result){
                if(safe==nullptr)return;
                if(result==1)safe->fileDialog(false);
                if(result==2){auto* p=safe->processor.state.getParameter("bypass");p->beginChangeGesture();safe->processor.set("bypass",1-safe->processor.value("bypass"));p->endChangeGesture();}
                if(result>=3&&result<=5){const int w=result==3?796:result==4?1060:1592;safe->setSize(w,juce::roundToInt(double(w)*hardware::height/hardware::width));}
                if(result==6)juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"MixDown Study","AulaMix • visual panel 0.2.0\nOriginal 1592 × 988 artwork embedded without modification.\nIndependent DSP approximation; not the original Waves algorithm.");
            });
    }
    void fileDialog(bool saving)
    {
        chooser=std::make_unique<juce::FileChooser>(saving?"Save MixDown preset":"Load MixDown preset",juce::File{},"*.mixdown");
        const juce::Component::SafePointer<Panel> safe(this);
        chooser->launchAsync(saving?juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting:
                                     juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
            [safe,saving](const juce::FileChooser& dialog){
                if(safe==nullptr)return;auto f=dialog.getResult();if(f==juce::File{})return;
                bool success=false;
                if(saving){juce::MemoryBlock b;safe->processor.getStateInformation(b);success=f.withFileExtension("mixdown").replaceWithData(b.getData(),b.getSize());}
                else if(f.getSize()<4*1024*1024){juce::MemoryBlock b;if(f.loadFileAsData(b)){auto x=juce::AudioProcessor::getXmlFromBinary(b.getData(),int(b.getSize()));if(x!=nullptr&&x->hasTagName("MixDown")){safe->processor.setStateInformation(b.getData(),int(b.getSize()));success=true;}}}
                if(success)safe->captureCleanState();
                else juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Preset","Não foi possível ler ou salvar este preset.");
            });
    }
    void captureSetup(int i) { setupStates[size_t(i)].reset();processor.getStateInformation(setupStates[size_t(i)]); }
    void captureCleanState()
    {
        baseline.clear();for(auto* p:processor.getParameters())baseline.push_back(p->getValue());
        cleanProgram=processor.getCurrentProgram();updateTitle();
    }
    void updateTitle()
    {
        bool dirty=false;auto& params=processor.getParameters();
        for(int i=0;i<params.size()&&i<int(baseline.size());++i)dirty|=std::abs(params[i]->getValue()-baseline[size_t(i)])>.00001f;
        auto title=juce::String(activeSetup==0?"A: ":"B: ")+processor.getProgramName(processor.getCurrentProgram())+(dirty?"*":"");
        if(presets.getText()!=title)presets.setText(title,juce::dontSendNotification);
        copy.setButtonText(activeSetup==0?juce::String::fromUTF8("A → B"):juce::String::fromUTF8("B → A"));
        setup.setButtonText(activeSetup==0?"Setup A":"Setup B");
        undo.setEnabled(processor.history.canUndo());redo.setEnabled(processor.history.canRedo());
    }
    void timerCallback() override
    {
        for(int c=0;c<2;++c)
        {
            auto& in=inputMeter.levels[size_t(c)];auto& out=outputMeter.levels[size_t(c)];
            in=juce::jmax(in*.89f,processor.inLevel[c].exchange(0));out=juce::jmax(out*.89f,processor.outLevel[c].exchange(0));
            float target=0;
            if(processor.value("meter")<.5f)target=std::sqrt(juce::jmax(0.f,processor.inPower[c].load()))*juce::Decibels::decibelsToGain(processor.value("vuCal"));
            else if(processor.value("meter")>1.5f)target=std::sqrt(juce::jmax(0.f,processor.outPower[c].load()))*juce::Decibels::decibelsToGain(processor.value("vuCal"));
            else target=juce::Decibels::decibelsToGain(-processor.gr.load());
            needle[size_t(c)]+=(target-needle[size_t(c)])*.22f;
            auto& face=c==0?leftVu:rightVu;face.db=juce::jlimit(-20.f,3.f,juce::Decibels::gainToDecibels(needle[size_t(c)],-80.f));face.repaint();
        }
        sensitivity.level=juce::jmax(inputMeter.levels[0],inputMeter.levels[1]);sensitivity.repaint();inputMeter.repaint();outputMeter.repaint();
        if(processor.getCurrentProgram()!=cleanProgram)captureCleanState();else updateTitle();
    }
    hardware::Look look;
    MixDown& processor;
    hardware::Canvas canvas;
    hardware::VuFace leftVu,rightVu;
    hardware::LedMeter inputMeter,outputMeter;
    hardware::Sensitivity sensitivity;
    std::array<juce::Slider,6> faders;
    std::array<juce::ToggleButton,4> on;
    std::array<juce::TextButton,2> modes;
    std::array<juce::TextButton,3> meters;
    juce::ToggleButton link;
    juce::ComboBox presets;
    juce::TextButton undo,redo,previous,next,copy,setup,save,menu;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,6> sliders;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>,4> buttons;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> linkAttachment;
    std::unique_ptr<juce::ParameterAttachment> modeAttachment,meterAttachment;
    std::array<juce::MemoryBlock,2> setupStates;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::TooltipWindow tooltip{this,500};
    std::vector<float> baseline;
    std::array<float,2> needle{1,1};
    double last=0;bool linking=false,linkedGesture=false;int activeSetup=0,cleanProgram=0;
};
