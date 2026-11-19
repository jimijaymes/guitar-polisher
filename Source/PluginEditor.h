#pragma once
#include "PluginProcessor.h"
class PanelLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider&) override {
        const auto centre=juce::Point<float>((float)x+w*0.5f,(float)y+h*0.47f);
        const float r=juce::jmin((float)w,(float)h)*0.33f;
        g.setColour(juce::Colour(0xff0a101b));g.fillEllipse(centre.x-r-5,centre.y-r-5,2*r+10,2*r+10);
        g.setColour(juce::Colour(0xff394c60));g.drawEllipse(centre.x-r-5,centre.y-r-5,2*r+10,2*r+10,1.5f);
        juce::Path arc;arc.addCentredArc(centre.x,centre.y,r+9,r+9,0,start,end,true);
        g.setColour(juce::Colour(0xff394c60));g.strokePath(arc,juce::PathStrokeType(3.0f));
        juce::Path active;active.addCentredArc(centre.x,centre.y,r+9,r+9,0,start,start+pos*(end-start),true);
        g.setColour(juce::Colour(0xff59e0d2));g.strokePath(active,juce::PathStrokeType(3.3f));
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff45556a),centre.x-r,centre.y-r,juce::Colour(0xff1a2431),centre.x+r,centre.y+r,false));
        g.fillEllipse(centre.x-r,centre.y-r,2*r,2*r);
        const float angle=start+pos*(end-start);
        juce::Path pointer;pointer.addRoundedRectangle(-2.0f,-r+9.0f,4.0f,r*0.48f,1.8f);
        g.setColour(juce::Colour(0xffe9edf1));g.fillPath(pointer,juce::AffineTransform::rotation(angle).translated(centre.x,centre.y));
        g.setColour(juce::Colour(0xff0b1019));g.fillEllipse(centre.x-5,centre.y-5,10,10);
    }
};

class GuitarPolisherEditor final : public juce::AudioProcessorEditor,private juce::Timer {
public:
    explicit GuitarPolisherEditor(GuitarPolisherProcessor&);
    ~GuitarPolisherEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    struct Dial {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    GuitarPolisherProcessor& processor;
    PanelLookAndFeel dialLook;
    std::array<Dial,13> dials;
    juce::ComboBox hpSlope,lpSlope;
    juce::ToggleButton highsSwitch,makeupSwitch;
    juce::Label hpSlopeLabel,lpSlopeLabel,highsLabel,makeupLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> hpAttachment,lpAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> highsAttachment,makeupAttachment;
    float meter=0;
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarPolisherEditor)
};
