#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

class GuitarPolisherProcessor final : public juce::AudioProcessor {
public:
    GuitarPolisherProcessor();
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Guitar Polisher"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> gainReductionDb {0.0f};
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    using Filter=juce::dsp::IIR::Filter<float>;
    std::array<Filter,2> mid, shelf;
    std::array<float,2> hp1{},hp2{},lp1{},lp2{},lp3{},satLow{};
    std::array<float,2> compEnvelope{};
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::AudioBuffer<float> colourBuffer;
    int maxBlock=512;
    double sampleRateHz=44100.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarPolisherProcessor)
};
