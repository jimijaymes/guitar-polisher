#include "PluginEditor.h"
namespace {
const juce::Colour dark(0xff192326),panel(0xff28373a),white(0xffeee8d8),teal(0xff61d0bd),muted(0xffa9b7b5);
}
GuitarPolisherEditor::GuitarPolisherEditor(GuitarPolisherProcessor& p):AudioProcessorEditor(p),processor(p) {
    const char* ids[]={"hpf","lpf","resonance","midfreq","midgain","midq","shelf","shelffreq","satfreq","drive","comp","output"};
    const char* names[]={"HIGH PASS","LOW PASS","RESONANCE","MID FREQ","MID GAIN","MID Q","HIGH SHELF","SHELF FREQ","SAT FOCUS","SAT DRIVE","COMPRESS","OUTPUT"};
    for(size_t i=0;i<dials.size();++i){
        auto& d=dials[i];
        d.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        d.slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,92,20);
        d.slider.setColour(juce::Slider::rotarySliderFillColourId,teal);
        d.slider.setColour(juce::Slider::rotarySliderOutlineColourId,muted.withAlpha(0.45f));
        d.slider.setColour(juce::Slider::thumbColourId,white);
        d.slider.setColour(juce::Slider::textBoxTextColourId,white);
        d.slider.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
        d.slider.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
        addAndMakeVisible(d.slider);
        d.label.setText(names[i],juce::dontSendNotification);
        d.label.setJustificationType(juce::Justification::centred);
        d.label.setColour(juce::Label::textColourId,white);
        d.label.setFont(juce::Font(juce::FontOptions(12.0f,juce::Font::bold)));
        addAndMakeVisible(d.label);
        d.attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,ids[i],d.slider);
    }
    hpSlope.addItem("6 dB/oct",1);hpSlope.addItem("12 dB/oct",2);
    lpSlope.addItem("6 dB/oct",1);lpSlope.addItem("12 dB/oct",2);lpSlope.addItem("18 dB/oct",3);
    for(auto* combo:{&hpSlope,&lpSlope}){
        combo->setColour(juce::ComboBox::backgroundColourId,dark);
        combo->setColour(juce::ComboBox::textColourId,white);
        combo->setColour(juce::ComboBox::outlineColourId,teal);
        addAndMakeVisible(*combo);
    }
    hpAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,"hpslope",hpSlope);
    lpAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,"lpslope",lpSlope);
    hpSlopeLabel.setText("HP SLOPE",juce::dontSendNotification);
    lpSlopeLabel.setText("LP SLOPE",juce::dontSendNotification);
    for(auto* label:{&hpSlopeLabel,&lpSlopeLabel}){
        label->setColour(juce::Label::textColourId,white);
        label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(*label);
    }
    setSize(930,510);
    startTimerHz(25);
}
void GuitarPolisherEditor::paint(juce::Graphics& g) {
    g.fillAll(dark);
    g.setColour(panel);
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(12),12);
    g.setColour(teal);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(12),12,2);
    g.setFont(juce::Font(juce::FontOptions(32.0f,juce::Font::bold)));
    g.setColour(white);
    g.drawText("GUITAR POLISHER",34,20,560,45,juce::Justification::centredLeft);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.setColour(muted);
    g.drawText("POST-MIC TONE SHAPING  /  SATURATION  /  LEVELING",36,65,620,22,juce::Justification::centredLeft);
    g.setColour(teal);
    g.drawLine(32,99,898,99,2);
    g.setColour(white);
    g.setFont(juce::Font(juce::FontOptions(13.0f,juce::Font::bold)));
    g.drawText("GAIN REDUCTION",680,27,210,23,juce::Justification::centred);
    g.setColour(juce::Colour(0xff0d1618));
    g.fillRoundedRectangle(698,57,174,16,5);
    g.setColour(teal);
    g.fillRoundedRectangle(698,57,174*juce::jlimit(0.0f,1.0f,meter/24.0f),16,5);
    g.setColour(muted);
    g.drawText(juce::String(meter,1)+" dB",738,77,95,18,juce::Justification::centred);
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("INDEPENDENT AUDIO   •   VST3",34,478,450,20,juce::Justification::centredLeft);
}
void GuitarPolisherEditor::resized() {
    for(size_t i=0;i<dials.size();++i){
        const int row=(int)i/6,col=(int)i%6;
        const int x=32+col*146,y=115+row*160;
        dials[i].slider.setBounds(x,y,132,112);
        dials[i].label.setBounds(x,y+109,132,23);
    }
    hpSlopeLabel.setBounds(76,438,105,22);
    hpSlope.setBounds(180,438,125,26);
    lpSlopeLabel.setBounds(360,438,105,22);
    lpSlope.setBounds(465,438,125,26);
}
void GuitarPolisherEditor::timerCallback(){
    const float next=processor.gainReductionDb.load(std::memory_order_relaxed);
    meter+=0.25f*(next-meter);
    repaint(680,24,210,75);
}
