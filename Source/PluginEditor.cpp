#include "PluginEditor.h"
#include <cmath>
namespace {
const juce::Colour bg(0xff0c1019), body(0xff1c2635), light(0xffe9edf1), dim(0xff9aa9bb);
const juce::Colour cyan(0xff59e0d2), orange(0xffffb35c), border(0xff394c60);
}
GuitarPolisherEditor::GuitarPolisherEditor(GuitarPolisherProcessor& p):AudioProcessorEditor(p),processor(p) {

    const char* ids[]={"hpf","lpf","resonance","midfreq","midgain","midq","shelf","shelffreq","satfreq","satwidth","drive","comp","output","apiInputDrive","apiOutputDrive"};
    const char* names[]={"HIGH PASS","LOW PASS","LP RESONANCE","MID FREQ","MID GAIN","MID WIDTH / Q","HIGH SHELF","SHELF FREQ","SAT FOCUS","SAT WIDTH","SAT DRIVE","COMPRESS","OUTPUT","API INPUT DRIVE","API OUTPUT DRIVE"};
    for(size_t i=0;i<dials.size();++i){
        auto& d=dials[i];
        d.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        d.slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,105,19);
        d.slider.setLookAndFeel(&dialLook);
        d.slider.setColour(juce::Slider::textBoxTextColourId,light);
        d.slider.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
        d.slider.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
        addAndMakeVisible(d.slider);
        d.label.setText(names[i],juce::dontSendNotification);
        d.label.setJustificationType(juce::Justification::centred);d.label.setMinimumHorizontalScale(0.68f);
        d.label.setColour(juce::Label::textColourId,dim);
        d.label.setFont(juce::Font(juce::FontOptions(11.0f,juce::Font::bold)));
        addAndMakeVisible(d.label);
        d.attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,ids[i],d.slider);
    }
    hpSlope.addItem("6 dB/oct",1);hpSlope.addItem("12 dB/oct",2);
    lpSlope.addItem("6 dB/oct",1);lpSlope.addItem("12 dB/oct",2);lpSlope.addItem("18 dB/oct",3);
    for(auto* c:{&hpSlope,&lpSlope}){
        c->setColour(juce::ComboBox::backgroundColourId,bg);
        c->setColour(juce::ComboBox::textColourId,light);
        c->setColour(juce::ComboBox::outlineColourId,border);
        addAndMakeVisible(*c);
    }
    hpAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,"hpslope",hpSlope);
    lpAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,"lpslope",lpSlope);
    for(auto* label:{&hpSlopeLabel,&lpSlopeLabel,&highsLabel,&makeupLabel}){
        label->setColour(juce::Label::textColourId,dim);
        label->setJustificationType(juce::Justification::centredLeft);
        label->setFont(juce::Font(juce::FontOptions(11.0f,juce::Font::bold)));
        addAndMakeVisible(*label);
    }
    hpSlopeLabel.setText("HP SLOPE",juce::dontSendNotification);
    lpSlopeLabel.setText("LP SLOPE",juce::dontSendNotification);
    highsLabel.setText("HF ONLY  /  3 kHz+",juce::dontSendNotification);
    makeupLabel.setText("AUTO MAKEUP",juce::dontSendNotification);
    for(auto* b:{&highsSwitch,&makeupSwitch,&apiInputSwitch,&apiOutputSwitch}){
        b->setButtonText("");
        b->setColour(juce::ToggleButton::tickColourId,cyan);
        b->setColour(juce::ToggleButton::tickDisabledColourId,dim);
        addAndMakeVisible(*b);
    }
    highsAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"highonly",highsSwitch);
    makeupAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"compmakeup",makeupSwitch);
    apiInputSwitch.setButtonText("ON");apiOutputSwitch.setButtonText("ON");
    apiInputAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"apiInputOn",apiInputSwitch);
    apiOutputAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"apiOutputOn",apiOutputSwitch);
    setSize(1130,890);
    startTimerHz(25);
}
GuitarPolisherEditor::~GuitarPolisherEditor(){
    for(auto& d:dials)d.slider.setLookAndFeel(nullptr);
}
void GuitarPolisherEditor::paint(juce::Graphics& g) {
    g.fillAll(bg);
    g.setGradientFill(juce::ColourGradient(body,0,0,bg,(float)getWidth(),(float)getHeight(),false));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(12),18);
    g.setColour(border);g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(12),18,1.4f);
    g.setColour(cyan);g.fillRoundedRectangle(32,29,6,59,3);
    g.setColour(light);g.setFont(juce::Font(juce::FontOptions(37.0f,juce::Font::bold)));
    g.drawText("GUITAR / POLISHER",52,26,660,52,juce::Justification::centredLeft);
    g.setColour(dim);g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText("INDEPENDENT AUDIO     •     POST-MIC FINISHING CHANNEL     •     0.3",55,78,720,20,juce::Justification::centredLeft);
    const char* section[]={"01   FILTERS","02   TONE SHAPING","03   SATURATION","04   DYNAMICS"};
    const int xs[]={26,302,578,854};
    for(int i=0;i<4;++i){
        g.setColour(juce::Colour(0xff111b29));g.fillRoundedRectangle((float)xs[i],129,262,435,13);
        g.setColour(border);g.drawRoundedRectangle((float)xs[i],129,262,435,13,1);
        g.setColour(i==3?orange:cyan);g.fillRoundedRectangle((float)xs[i]+13,145,4,20,2);
        g.setFont(juce::Font(juce::FontOptions(13.0f,juce::Font::bold)));
        g.drawText(section[i],xs[i]+25,144,220,23,juce::Justification::centredLeft);
    }
    g.setColour(juce::Colour(0xff09121c));g.fillRoundedRectangle(822,29,283,70,9);
    g.setColour(dim);g.setFont(juce::Font(juce::FontOptions(11.0f,juce::Font::bold)));
    g.drawText("COMPRESSOR  /  GAIN REDUCTION",838,34,251,17,juce::Justification::centred);
    g.setColour(border);g.fillRoundedRectangle(845,59,238,12,5);
    g.setColour(orange);g.fillRoundedRectangle(845,59,238.0f*juce::jlimit(0.0f,1.0f,meter/24.0f),12,5);
    g.setColour(light);g.drawText(juce::String(meter,1)+" dB",924,76,83,18,juce::Justification::centred);
    g.setColour(dim);g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("SIGNAL PATH  •  API INPUT → HP → EQ → SAT → COMP → LP → API OUTPUT",35,653,1050,20,juce::Justification::centredLeft);
    g.setColour(border);g.drawRoundedRectangle(26,700,1078,160,12,1.3f);
    g.setColour(cyan);g.setFont(juce::Font(juce::FontOptions(14.0f,juce::Font::bold)));
    g.drawText("API 2622 INPUT TRANSFORMER + 2520",42,712,480,24,juce::Justification::centredLeft);
    g.drawText("API 2520 + 2503 OUTPUT TRANSFORMER",572,712,490,24,juce::Justification::centredLeft);
}
void GuitarPolisherEditor::resized(){
    // Section positions follow processing order; controls never overlap.
    const int sec[]={0,0,0,1,1,1,1,1,2,2,2,3,3};
    const int slot[]={0,1,2,0,1,2,3,4,0,1,2,0,1};
    const int xs[]={26,302,578,854};
    for(size_t i=0;i<dials.size();++i){
        const int x=i>=13?(i==13?195:720):xs[sec[i]]+12+(slot[i]%2)*124;
        const int y=i>=13?740:186+(slot[i]/2)*113;
        dials[i].slider.setBounds(x,y,116,89);
        dials[i].label.setBounds(x-2,y+92,120,19);
    }
    hpSlopeLabel.setBounds(45,571,78,22);hpSlope.setBounds(128,570,139,26);
    lpSlopeLabel.setBounds(315,571,78,22);lpSlope.setBounds(399,570,139,26);
    highsSwitch.setBounds(875,472,28,26);highsLabel.setBounds(908,474,180,23);
    makeupSwitch.setBounds(875,514,28,26);makeupLabel.setBounds(908,516,180,23);
    apiInputSwitch.setBounds(55,762,35,30);apiOutputSwitch.setBounds(590,762,35,30);
}
void GuitarPolisherEditor::timerCallback(){
    const float next=processor.gainReductionDb.load(std::memory_order_relaxed);
    meter+=0.25f*(next-meter);
    repaint(822,29,283,70);
}
