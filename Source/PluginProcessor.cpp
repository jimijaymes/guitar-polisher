#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
namespace {
float onePole(float hz,double sr) { return std::exp(-2.0f*juce::MathConstants<float>::pi*hz/(float)sr); }
float dbGain(float db) { return juce::Decibels::decibelsToGain(db); }
}
juce::AudioProcessorValueTreeState::ParameterLayout GuitarPolisherProcessor::layout() {
    using F=juce::AudioParameterFloat; using C=juce::AudioParameterChoice; using B=juce::AudioParameterBool;
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    p.add(std::make_unique<F>(juce::ParameterID{"hpf",1},"High Pass (Hz)",juce::NormalisableRange<float>(30,350,1,0.5f),75));
    p.add(std::make_unique<C>(juce::ParameterID{"hpslope",1},"HP Slope",juce::StringArray{"6 dB/oct","12 dB/oct"},1));
    p.add(std::make_unique<F>(juce::ParameterID{"lpf",1},"Low Pass (Hz)",juce::NormalisableRange<float>(1500,20000,1,0.5f),10500));
    p.add(std::make_unique<C>(juce::ParameterID{"lpslope",1},"LP Slope",juce::StringArray{"6 dB/oct","12 dB/oct","18 dB/oct"},1));
    p.add(std::make_unique<F>(juce::ParameterID{"resonance",1},"LP Resonance",juce::NormalisableRange<float>(0,100,0.1f),0));
    p.add(std::make_unique<F>(juce::ParameterID{"midfreq",1},"Mid Frequency (Hz)",juce::NormalisableRange<float>(180,4500,1,0.35f),1100));
    p.add(std::make_unique<F>(juce::ParameterID{"midgain",1},"Mid Gain (dB)",juce::NormalisableRange<float>(-12,12,0.1f),0));
    p.add(std::make_unique<F>(juce::ParameterID{"midq",1},"Mid Q",juce::NormalisableRange<float>(0.3f,4.0f,0.01f),0.9f));
    p.add(std::make_unique<F>(juce::ParameterID{"shelf",1},"High Shelf (dB)",juce::NormalisableRange<float>(-12,12,0.1f),0));
    p.add(std::make_unique<F>(juce::ParameterID{"shelffreq",1},"Shelf Frequency (Hz)",juce::NormalisableRange<float>(2500,12000,1,0.5f),6500));
    p.add(std::make_unique<F>(juce::ParameterID{"satfreq",1},"Saturation Focus (Hz)",juce::NormalisableRange<float>(120,6000,1,0.5f),1600));
    p.add(std::make_unique<F>(juce::ParameterID{"satwidth",1},"Saturation Bandwidth (oct)",juce::NormalisableRange<float>(0.5f,5.0f,0.1f),2.0f));
    p.add(std::make_unique<F>(juce::ParameterID{"drive",1},"Saturation Drive (dB)",juce::NormalisableRange<float>(0,18,0.1f),3));
    p.add(std::make_unique<F>(juce::ParameterID{"comp",1},"Smooth Compression",juce::NormalisableRange<float>(0,100,0.1f),25));
    p.add(std::make_unique<B>(juce::ParameterID{"highonly",1},"High Frequency Compression Only",false));
    p.add(std::make_unique<B>(juce::ParameterID{"compmakeup",1},"Auto Makeup Gain",false));
    p.add(std::make_unique<F>(juce::ParameterID{"output",1},"Output (dB)",juce::NormalisableRange<float>(-18,18,0.1f),0));
    return p;
}
GuitarPolisherProcessor::GuitarPolisherProcessor()
: AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true)
.withOutput("Output",juce::AudioChannelSet::stereo(),true)),parameters(*this,nullptr,"STATE",layout()) {}
void GuitarPolisherProcessor::prepareToPlay(double sr,int block) {
    sampleRateHz=sr; maxBlock=juce::jmax(1,block);
    hp1.fill(0); hp2.fill(0); lp1.fill(0); lp2.fill(0); lp3.fill(0); satLow.fill(0); satHigh.fill(0); compEnvelope.fill(0); compLow.fill(0); compLow2.fill(0);
    juce::dsp::ProcessSpec spec{sr,(juce::uint32)maxBlock,1};
    for(auto& f:mid){ f.prepare(spec); f.reset(); }
    for(auto& f:shelf){ f.prepare(spec); f.reset(); }
    for(auto& f:lpPeak){ f.prepare(spec); f.reset(); }
    colourBuffer.setSize(2,maxBlock);
    oversampling=std::make_unique<juce::dsp::Oversampling<float>>(2,2,juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,true,false);
    oversampling->initProcessing((size_t)maxBlock);
    oversampling->reset();
    setLatencySamples((int)std::round(oversampling->getLatencyInSamples()));
}
void GuitarPolisherProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int n=buffer.getNumSamples(),channels=juce::jmin(2,buffer.getNumChannels());
    if(!channels)return;
    const auto param=[this](const char* id) -> float { return parameters.getRawParameterValue(id)->load(std::memory_order_relaxed); };
    const float hp=onePole(param("hpf"),sampleRateHz),lp=onePole(param("lpf"),sampleRateHz);
    const int hpStages=(int)param("hpslope")+1,lpStages=(int)param("lpslope")+1;
    const float res=param("resonance")*0.01f;
    const float satCenter=param("satfreq");
    const float halfBand=std::pow(2.0f,param("satwidth")*0.5f);
    const float satLo=onePole(juce::jmax(20.0f,satCenter/halfBand),sampleRateHz*4.0);
    const float satHi=onePole(juce::jmin((float)sampleRateHz*1.8f,satCenter*halfBand),sampleRateHz*4.0);
    const float compXover=onePole(3000.0f,sampleRateHz);
    const bool highsOnly=param("highonly")>0.5f;
    const bool autoMakeup=param("compmakeup")>0.5f;
    const float drive=dbGain(param("drive"));
    const float out=dbGain(param("output"));
    const float comp=param("comp")*0.01f;
    const float threshold=-8.0f-28.0f*comp;
    const float attack=onePole(35.0f,sampleRateHz);
    const float release=onePole(3.0f,sampleRateHz);
    const float midHz=juce::jlimit(20.0f,(float)sampleRateHz*0.45f,param("midfreq"));
    const float shelfHz=juce::jlimit(20.0f,(float)sampleRateHz*0.45f,param("shelffreq"));
    auto midCoeffs=juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRateHz,midHz,param("midq"),dbGain(param("midgain")));
    auto shelfCoeffs=juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRateHz,shelfHz,0.7071f,dbGain(param("shelf")));
    // Resonant emphasis immediately below the LP cutoff, ahead of the final LP stage.
    const float peakHz=juce::jlimit(20.0f,(float)sampleRateHz*0.43f,param("lpf")*0.82f);
    auto peakCoeffs=juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRateHz,peakHz,2.0f,dbGain(12.0f*res));
    for(int c=0;c<channels;++c){ *mid[(size_t)c].coefficients=*midCoeffs; *shelf[(size_t)c].coefficients=*shelfCoeffs; *lpPeak[(size_t)c].coefficients=*peakCoeffs; }
    float maxGR=0;
    for(int offset=0;offset<n;offset+=maxBlock) {
        const int count=juce::jmin(maxBlock,n-offset);
        for(int c=0;c<2;++c) {
            float* target=colourBuffer.getWritePointer(c);
            const float* source=buffer.getReadPointer(juce::jmin(c,channels-1),offset);
            for(int j=0;j<count;++j) {
                float x=source[j];
                hp1[(size_t)c]=hp*hp1[(size_t)c]+(1-hp)*x;
                x-=hp1[(size_t)c];
                if(hpStages==2){hp2[(size_t)c]=hp*hp2[(size_t)c]+(1-hp)*x;x-=hp2[(size_t)c];}
                x=mid[(size_t)c].processSample(x);
                x=shelf[(size_t)c].processSample(x);
                target[j]=x;
            }
        }
        juce::dsp::AudioBlock<float> block(colourBuffer);
        auto slice=block.getSubBlock(0,(size_t)count);
        auto up=oversampling->processSamplesUp(slice);
        for(int c=0;c<2;++c) {
            auto* samples=up.getChannelPointer((size_t)c);
            for(size_t j=0;j<up.getNumSamples();++j) {
                const float x=samples[j];
                auto& low=satLow[(size_t)c];
                auto& high=satHigh[(size_t)c];
                low=satLo*low+(1-satLo)*x;
                high=satHi*high+(1-satHi)*x;
                const float band=high-low;
                // Saturate only the selected frequency band, leaving the rest unchanged.
                samples[j]=x-band+std::tanh(band*drive)/drive;
            }
        }
        oversampling->processSamplesDown(slice);
        for(int c=0;c<channels;++c) {
            float* dst=buffer.getWritePointer(c,offset);
            const float* src=colourBuffer.getReadPointer(c);
            auto& env=compEnvelope[(size_t)c];
            for(int j=0;j<count;++j) {
                const float x=src[j];
                auto& low=compLow[(size_t)c];
                low=compXover*low+(1-compXover)*x;
                auto& low2=compLow2[(size_t)c];
                low2=compXover*low2+(1-compXover)*low;
                // Complementary bands: low2 + (x - low2) == x.
                // The low band remains untouched; only the high band is compressed.
                const float high=x-low2;
                const float band=highsOnly ? high : x;
                const float level=std::abs(band);
                env=(level>env?attack:release)*env+(1-(level>env?attack:release))*level;
                const float db=juce::Decibels::gainToDecibels(juce::jmax(env,1.0e-8f),-160.0f);
                const float above=db-threshold;
                // 12 dB soft knee, 2:1 compression.
                float gr=0;
                if(comp>0.0001f) {
                    if(above>6)gr=0.5f*above;
                    else if(above>-6){const float t=above+6;gr=t*t/48.0f;}
                }
                maxGR=juce::jmax(maxGR,gr);
                const float makeup=autoMakeup && comp>0.0001f ? dbGain(gr*0.65f) : 1.0f;
                float y=highsOnly ? low2+high*dbGain(-gr)*makeup : x*dbGain(-gr)*makeup;
                y=lpPeak[(size_t)c].processSample(y);
                // Last stage: low pass after EQ, saturation and compression.
                lp1[(size_t)c]=lp*lp1[(size_t)c]+(1-lp)*y;
                y=lp1[(size_t)c];
                if(lpStages>=2){lp2[(size_t)c]=lp*lp2[(size_t)c]+(1-lp)*y;y=lp2[(size_t)c];}
                if(lpStages>=3){lp3[(size_t)c]=lp*lp3[(size_t)c]+(1-lp)*y;y=lp3[(size_t)c];}
                dst[j]=y*out;
            }
        }
    }
    gainReductionDb.store(maxGR,std::memory_order_relaxed);
}
juce::AudioProcessorEditor* GuitarPolisherProcessor::createEditor(){return new GuitarPolisherEditor(*this);}
void GuitarPolisherProcessor::getStateInformation(juce::MemoryBlock& b){auto xml=parameters.copyState().createXml();copyXmlToBinary(*xml,b);}
void GuitarPolisherProcessor::setStateInformation(const void* data,int size){if(auto xml=getXmlFromBinary(data,size))if(xml->hasTagName(parameters.state.getType()))parameters.replaceState(juce::ValueTree::fromXml(*xml));}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new GuitarPolisherProcessor();}
