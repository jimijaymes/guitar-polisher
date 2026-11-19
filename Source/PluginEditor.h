#pragma once
#include "PluginProcessor.h"
class GuitarPolisherEditor final : public juce::AudioProcessorEditor,private juce::Timer {
public:
    explicit GuitarPolisherEditor(GuitarPolisherProcessor&);
    ~GuitarPolisherEditor() override =default;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    struct Dial {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    GuitarPolisherProcessor& processor;
    std::array<Dial,13> dials;
    juce::ComboBox hpSlope,lpSlope,compMode,compMakeup;
    juce::Label hpSlopeLabel,lpSlopeLabel,compModeLabel,compMakeupLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> hpAttachment,lpAttachment,compModeAttachment,compMakeupAttachment;
    float meter=0;
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarPolisherEditor)
};
