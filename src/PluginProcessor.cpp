#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {

struct FactoryPreset
{
    const char* name;
    float outGain, inGain, loud, width, eqLow, eqMid, eqHigh;
    bool clean;
    float vibe;
    bool autoGain;
    float mix;
    int hqIdx, skin;
    float lowHpf;
};

constexpr FactoryPreset kPresets[] = {
    { "Init Master",    0.0f, 0.0f, 0.0f, 100.0f,  0.0f,  0.0f,  0.0f, false, 35.0f, false, 100.0f, 1, 2, 150.0f },
    { "Streaming -14",  0.0f, 0.0f, 2.5f, 112.0f,  0.8f,  0.0f,  1.0f, true,  35.0f, true,  100.0f, 1, 2, 150.0f },
    { "BoomBap Glue",   0.0f, 0.0f, 5.5f, 108.0f,  1.5f, -0.5f,  1.2f, true,  55.0f, false, 100.0f, 2, 2, 180.0f },
    { "Trap 808 Knock", 0.0f, 0.0f, 6.8f, 118.0f,  2.0f, -0.8f,  1.8f, true,  45.0f, false, 100.0f, 2, 2, 200.0f },
    { "Drill Heavy",    0.0f, 0.0f, 7.8f, 122.0f,  1.6f, -1.0f,  2.2f, true,  45.0f, false, 100.0f, 2, 2, 210.0f },
    { "Lo-Fi Tape Bus", 0.0f, 0.0f, 5.2f,  92.0f,  1.8f,  0.6f, -1.8f, false, 65.0f, false,  85.0f, 1, 2, 140.0f },
    { "Wide & Airy",    0.0f, 0.0f, 3.2f, 142.0f,  0.5f, -0.4f,  2.6f, true,  30.0f, false, 100.0f, 1, 2, 130.0f },
    { "Club Loud",      0.0f, 0.0f, 8.5f, 120.0f,  2.0f,  0.3f,  1.6f, true,  50.0f, false, 100.0f, 3, 2, 180.0f },
    { "House Pump",     0.0f, 0.0f, 5.8f, 126.0f,  1.4f, -0.3f,  1.5f, true,  60.0f, false, 100.0f, 2, 2, 160.0f },
    { "R&B Silk",       0.0f, 0.0f, 3.0f, 128.0f,  1.0f, -0.6f,  2.0f, true,  35.0f, true,  100.0f, 1, 2, 150.0f },
    { "Analog Warmth",  0.0f, 0.0f, 5.0f, 106.0f,  1.2f,  0.4f, -0.5f, false, 48.0f, false, 100.0f, 2, 2, 140.0f },
    { "Clean Master",   0.0f, 0.0f, 2.2f, 104.0f,  0.0f,  0.0f,  0.4f, true,  20.0f, false, 100.0f, 2, 2, 150.0f },
    { "Parallel Punch", 0.0f, 0.0f, 6.5f, 115.0f,  1.4f,  0.0f,  1.2f, false, 85.0f, false,  50.0f, 1, 2, 160.0f },
    { "Drum Bus Smash", 0.0f, 0.0f, 7.2f, 110.0f,  1.5f, -0.7f,  1.8f, true,  90.0f, false,  65.0f, 2, 2, 170.0f },
    { "Mono Check",     0.0f, 0.0f, 0.0f,   0.0f,  0.0f,  0.0f,  0.0f, false,  0.0f, false, 100.0f, 0, 2,   0.0f },
    { "Broadcast -14",  0.0f, 0.0f, 1.8f, 100.0f, -0.5f,  0.8f,  0.6f, true,  40.0f, true,  100.0f, 1, 2, 120.0f },
};

} // namespace

MPChainAudioProcessor::MPChainAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "MPCHAIN_STATE", createParameterLayout())
{
    setLatencySamples(mixchain::kLinEqLatency);
}

juce::AudioProcessorValueTreeState::ParameterLayout MPChainAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("out_gain", "Out Gain", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("in_gain",  "In Gain",  juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("loud",     "Loud",     juce::NormalisableRange<float>(0.0f, 12.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("width",    "Width",    juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f), 100.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("eq_low",   "Low",      juce::NormalisableRange<float>(-6.0f, 6.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("eq_mid",   "Mid",      juce::NormalisableRange<float>(-6.0f, 6.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("eq_high",  "High",     juce::NormalisableRange<float>(-6.0f, 6.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool> ("treble_clean", "Clean", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vibe",     "Vibe",     juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 35.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool> ("auto_gain","Auto -14", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("mix",      "Mix",      juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 100.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("hq_os",   "HQ OS",    juce::StringArray{ "1x", "2x", "4x", "8x" }, 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("skin",    "Skin",     juce::StringArray{ "MPC Red", "Retro 3000", "MPC Gold" }, 2));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("comp_hpf", "Low HPF",  juce::NormalisableRange<float>(0.0f, 240.0f, 1.0f, 0.7f), 150.0f));

    return { params.begin(), params.end() };
}

void MPChainAudioProcessor::prepareToPlay(double sampleRate, int)
{
    dsp_.prepare(sampleRate);
    setLatencySamples(mixchain::kLinEqLatency);
}

void MPChainAudioProcessor::releaseResources()
{
    dsp_.reset();
}

bool MPChainAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void MPChainAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    mixchain::MPChainParams p;
    p.outGainDb   = apvts.getRawParameterValue("out_gain")->load();
    p.inGainDb    = apvts.getRawParameterValue("in_gain")->load();
    p.loudDb      = apvts.getRawParameterValue("loud")->load();
    p.widthPct    = apvts.getRawParameterValue("width")->load();
    p.eqLowDb     = apvts.getRawParameterValue("eq_low")->load();
    p.eqMidDb     = apvts.getRawParameterValue("eq_mid")->load();
    p.eqHighDb    = apvts.getRawParameterValue("eq_high")->load();
    p.trebleClean = apvts.getRawParameterValue("treble_clean")->load() >= 0.5f;
    p.vibePct     = apvts.getRawParameterValue("vibe")->load();
    p.autoGain    = apvts.getRawParameterValue("auto_gain")->load() >= 0.5f;
    p.mixPct      = apvts.getRawParameterValue("mix")->load();
    p.hqIndex     = (int)std::lround(apvts.getRawParameterValue("hq_os")->load());
    p.lowHpfHz    = apvts.getRawParameterValue("comp_hpf")->load();

    float* chL = buffer.getWritePointer(0);
    float* chR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    if (resetIntReq_.exchange(false, std::memory_order_acq_rel))
        dsp_.resetIntegratedLoudness();

    auto m = dsp_.processBlock(chL, chR, buffer.getNumSamples(), p);
    inPeakDb.store(m.inPeakDb, std::memory_order_relaxed);
    inLufsDb.store(m.inLufsDb, std::memory_order_relaxed);
    outPeakDb.store(m.outPeakDb, std::memory_order_relaxed);
    outLufsDb.store(m.outLufsDb, std::memory_order_relaxed);
    outLufsShortDb.store(m.outLufsShortDb, std::memory_order_relaxed);
    outLufsIntDb.store(m.outLufsIntDb, std::memory_order_relaxed);
    outRmsDb.store(m.outRmsDb, std::memory_order_relaxed);
    outPlrDb.store(m.outPlrDb, std::memory_order_relaxed);
}

int MPChainAudioProcessor::getNumPrograms() { return (int)std::size(kPresets); }

void MPChainAudioProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= getNumPrograms()) return;
    currentProgram_ = index;
    const auto& pr = kPresets[index];
    auto setVal = [&](const char* id, float v) {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(v));
    };
    setVal("out_gain", pr.outGain);   setVal("in_gain", pr.inGain);
    setVal("loud", pr.loud);          setVal("width", pr.width);
    setVal("eq_low", pr.eqLow);       setVal("eq_mid", pr.eqMid);
    setVal("eq_high", pr.eqHigh);     setVal("treble_clean", pr.clean ? 1.0f : 0.0f);
    setVal("vibe", pr.vibe);          setVal("auto_gain", pr.autoGain ? 1.0f : 0.0f);
    setVal("mix", pr.mix);            setVal("hq_os", (float)pr.hqIdx);
    setVal("comp_hpf", pr.lowHpf);
}

const juce::String MPChainAudioProcessor::getProgramName(int index)
{
    if (index < 0 || index >= getNumPrograms()) return {};
    return kPresets[index].name;
}

void MPChainAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MPChainAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* MPChainAudioProcessor::createEditor()
{
    return new MPChainAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MPChainAudioProcessor();
}
