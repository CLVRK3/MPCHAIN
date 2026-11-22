#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {

struct FactoryPreset
{
    const char* name;
    float outGain, inGain, louder, width;
    float eqLow, eqLowMid, eqMid, eqHigh;
    bool  clean, air;
    float glue;
    int   autoTgt; // 0=Off, 1=-14, 2=-11, 3=-9
    int   hqIdx;   // 0=1x, 1=2x(96k), 2=4x(192k), 3=8x(384k)
    float hpf;
    float ceiling; // -14.0 .. 0.0 dB
    int   satMode; // 0=Off, 1=Tape, 2=Tube, 3=Xfmr, 4=Class-A
};

// 24 Factory Mastering & Stereo-Bus Presets built for v1.2 (4-band LinEQ, Air, 4 Sat Modes, -9/-11/-14 Auto, 0..+24 Louder)
constexpr FactoryPreset kPresets[] = {
    // Name                    Out   In   Loud  Width  Low   LMid  Mid   High  Cln    Air    Glue  Auto HQ  HPF    Ceil   Sat
    { "Init Transparent",      0.0f, 0.0f,  0.0f, 100.0f,  0.0f,  0.0f,  0.0f,  0.0f, false, false,  0.0f, 0, 1, 150.0f,  0.0f, 0 },
    { "Clean Master -14",      0.0f, 0.0f,  2.5f, 108.0f,  0.5f, -0.3f,  0.0f,  0.6f, true,  true,  25.0f, 1, 2, 140.0f, -1.0f, 4 },
    { "Streaming Loud -11",    0.0f, 0.0f,  5.5f, 114.0f,  0.8f, -0.4f,  0.2f,  0.8f, true,  true,  35.0f, 2, 2, 150.0f, -1.0f, 4 },
    { "Club Impact -9",        0.0f, 0.0f, 11.5f, 122.0f,  1.8f, -0.8f,  0.4f,  1.2f, true,  true,  48.0f, 3, 2, 180.0f, -0.5f, 3 },
    { "Tape Bus Glue",         0.0f, 0.0f,  6.5f, 108.0f,  1.2f, -0.4f,  0.0f,  0.5f, false, true,  52.0f, 0, 2, 160.0f, -0.5f, 1 },
    { "Tube Velvet Master",    0.0f, 0.0f,  5.0f, 118.0f,  1.0f,  0.3f, -0.3f,  1.2f, true,  true,  38.0f, 1, 2, 145.0f, -1.0f, 2 },
    { "Iron Transformer Bus",  0.0f, 0.0f,  8.5f, 112.0f,  1.6f, -0.6f,  0.5f,  0.9f, true,  false, 58.0f, 0, 2, 175.0f, -0.5f, 3 },
    { "Class-A Console Shine", 0.0f, 0.0f,  4.5f, 116.0f,  0.6f, -0.5f,  0.4f,  1.1f, true,  true,  32.0f, 1, 2, 140.0f, -0.5f, 4 },
    { "BoomBap Knock",         0.0f, 0.0f,  9.5f, 108.0f,  1.8f, -0.6f,  0.3f,  0.8f, true,  false, 55.0f, 0, 2, 180.0f, -0.3f, 1 },
    { "Trap 808 Slam",         0.0f, 0.0f, 13.5f, 120.0f,  2.2f, -1.2f,  0.6f,  1.5f, true,  true,  45.0f, 3, 2, 200.0f, -0.3f, 3 },
    { "UK Drill Pressure",     0.0f, 0.0f, 14.5f, 124.0f,  1.8f, -1.4f,  0.8f,  1.8f, true,  true,  50.0f, 3, 2, 210.0f, -0.3f, 3 },
    { "R&B Silk & Air",        0.0f, 0.0f,  4.0f, 128.0f,  1.1f, -0.5f, -0.4f,  1.4f, true,  true,  30.0f, 1, 2, 140.0f, -1.0f, 2 },
    { "Modern Pop Radio -9",   0.0f, 0.0f, 10.5f, 124.0f,  1.0f, -0.9f,  0.7f,  1.6f, true,  true,  42.0f, 3, 2, 150.0f, -0.5f, 4 },
    { "House & Techno Drive",  0.0f, 0.0f, 11.0f, 126.0f,  1.6f, -0.8f,  0.4f,  1.3f, true,  true,  60.0f, 2, 2, 165.0f, -0.5f, 1 },
    { "DnB Heavyweight",       0.0f, 0.0f, 15.5f, 118.0f,  1.8f, -1.1f,  0.9f,  1.6f, true,  true,  52.0f, 3, 3, 190.0f, -0.2f, 3 },
    { "Lo-Fi Warm Tape",       0.0f, 0.0f,  7.0f,  92.0f,  1.8f,  0.8f, -0.8f, -2.0f, false, false, 65.0f, 0, 1, 130.0f, -1.0f, 1 },
    { "Wide Airy Polish",      0.0f, 0.0f,  3.5f, 142.0f,  0.4f, -0.4f,  0.2f,  1.8f, true,  true,  25.0f, 1, 2, 130.0f, -1.0f, 4 },
    { "Vocal Presence Bus",    0.0f, 0.0f,  4.5f, 106.0f, -0.5f, -0.8f,  1.4f,  1.0f, true,  true,  36.0f, 1, 2, 120.0f, -1.0f, 2 },
    { "Drum Bus Punch",        0.0f, 0.0f,  9.5f, 110.0f,  1.6f, -0.9f,  0.8f,  1.2f, false, true,  72.0f, 0, 2, 165.0f, -0.5f, 3 },
    { "Acoustic Clarity",      0.0f, 0.0f,  2.0f, 112.0f,  0.3f, -0.6f,  0.4f,  0.9f, true,  true,  22.0f, 1, 2, 110.0f, -1.0f, 2 },
    { "De-Mud & Tighten",      0.0f, 0.0f,  3.5f, 108.0f,  0.6f, -2.0f,  0.6f,  1.0f, true,  true,  34.0f, 0, 2, 160.0f, -0.5f, 4 },
    { "Loud Clip Overdrive",   0.0f, 0.0f, 18.0f, 116.0f,  1.5f, -1.0f,  0.6f,  1.2f, true,  true,  50.0f, 0, 3, 180.0f, -0.3f, 3 },
    { "Broadcast Safe -14",    0.0f, 0.0f,  2.0f, 100.0f, -0.4f, -0.3f,  0.7f,  0.5f, true,  false, 38.0f, 1, 2, 120.0f, -1.5f, 0 },
    { "Mono Compatibility",    0.0f, 0.0f,  0.0f,   0.0f,  0.0f,  0.0f,  0.0f,  0.0f, false, false,  0.0f, 0, 0,   0.0f,  0.0f, 0 },
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

    params.push_back(std::make_unique<juce::AudioParameterFloat>("out_gain",    "Out Gain",  juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("in_gain",     "In Gain",   juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("loud",        "Louder",    juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("width",       "Width",     juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f), 100.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("eq_low",      "Low",       juce::NormalisableRange<float>(-6.0f, 6.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("eq_lowmid",   "Low Mid",   juce::NormalisableRange<float>(-6.0f, 6.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("eq_mid",      "Mid",       juce::NormalisableRange<float>(-6.0f, 6.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("eq_high",     "High",      juce::NormalisableRange<float>(-6.0f, 6.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool> ("treble_clean","Clean",     false));
    params.push_back(std::make_unique<juce::AudioParameterBool> ("air_boost",   "Air",       false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("vibe",        "Glue Comp", juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("auto_target","Auto LUFS", juce::StringArray{ "Off", "-14 LUFS", "-11 LUFS", "-9 LUFS" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("hq_os",      "HQ Rate",   juce::StringArray{ "1x", "2x 96k", "4x 192k", "8x 384k" }, 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("skin",       "Skin",      juce::StringArray{ "MPC Red", "Retro 3000", "Obsidian Blue" }, 2));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("comp_hpf",    "HPF",       juce::NormalisableRange<float>(0.0f, 240.0f, 1.0f, 0.7f), 150.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ceiling",     "Ceiling",   juce::NormalisableRange<float>(-14.0f, 0.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("sat_mode",   "Sat Mode",  juce::StringArray{ "Bypass", "Tape", "Tube", "Xfmr", "Class-A" }, 0));

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
    p.louderDb    = apvts.getRawParameterValue("loud")->load();
    p.widthPct    = apvts.getRawParameterValue("width")->load();
    p.eqLowDb     = apvts.getRawParameterValue("eq_low")->load();
    p.eqLowMidDb  = apvts.getRawParameterValue("eq_lowmid")->load();
    p.eqMidDb     = apvts.getRawParameterValue("eq_mid")->load();
    p.eqHighDb    = apvts.getRawParameterValue("eq_high")->load();
    p.trebleClean = apvts.getRawParameterValue("treble_clean")->load() >= 0.5f;
    p.airBoost    = apvts.getRawParameterValue("air_boost")->load() >= 0.5f;
    p.gluePct     = apvts.getRawParameterValue("vibe")->load();
    p.autoTarget  = (int)std::lround(apvts.getRawParameterValue("auto_target")->load());
    p.hqIndex     = (int)std::lround(apvts.getRawParameterValue("hq_os")->load());
    p.hpfHz       = apvts.getRawParameterValue("comp_hpf")->load();
    p.ceilingDb   = apvts.getRawParameterValue("ceiling")->load();
    p.satMode     = (int)std::lround(apvts.getRawParameterValue("sat_mode")->load());

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
    setVal("out_gain", pr.outGain);     setVal("in_gain", pr.inGain);
    setVal("loud", pr.louder);          setVal("width", pr.width);
    setVal("eq_low", pr.eqLow);         setVal("eq_lowmid", pr.eqLowMid);
    setVal("eq_mid", pr.eqMid);         setVal("eq_high", pr.eqHigh);
    setVal("treble_clean", pr.clean ? 1.0f : 0.0f);
    setVal("air_boost", pr.air ? 1.0f : 0.0f);
    setVal("vibe", pr.glue);            setVal("auto_target", (float)pr.autoTgt);
    setVal("hq_os", (float)pr.hqIdx);   setVal("comp_hpf", pr.hpf);
    setVal("ceiling", pr.ceiling);      setVal("sat_mode", (float)pr.satMode);
}

const juce::String MPChainAudioProcessor::getProgramName(int index)
{
    if (index < 0 || index >= getNumPrograms()) return {};
    return kPresets[index].name;
}

juce::File MPChainAudioProcessor::getUserPresetDir()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                   .getChildFile("MIXCHAIN")
                   .getChildFile("MPCHAIN")
                   .getChildFile("Presets");
    if (!dir.exists()) dir.createDirectory();
    return dir;
}

juce::StringArray MPChainAudioProcessor::getUserPresetNames() const
{
    juce::StringArray names;
    auto dir = getUserPresetDir();
    auto files = dir.findChildFiles(juce::File::findFiles, false, "*.xml");
    files.sort();
    for (const auto& f : files)
        names.add(f.getFileNameWithoutExtension());
    return names;
}

bool MPChainAudioProcessor::saveUserPreset(const juce::String& name)
{
    const auto safe = juce::File::createLegalFileName(name.trim());
    if (safe.isEmpty()) return false;
    auto file = getUserPresetDir().getChildFile(safe + ".xml");
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (!xml) return false;
    return xml->writeTo(file);
}

bool MPChainAudioProcessor::loadUserPreset(const juce::String& name)
{
    auto file = getUserPresetDir().getChildFile(name + ".xml");
    if (!file.existsAsFile()) return false;
    auto xml = juce::XmlDocument::parse(file);
    if (!xml || !xml->hasTagName(apvts.state.getType())) return false;
    const float keepSkin = apvts.getRawParameterValue("skin")->load();
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
    if (auto* sp = apvts.getParameter("skin"))
        sp->setValueNotifyingHost(sp->convertTo0to1(keepSkin));
    return true;
}

bool MPChainAudioProcessor::deleteUserPreset(const juce::String& name)
{
    auto file = getUserPresetDir().getChildFile(name + ".xml");
    return file.existsAsFile() && file.deleteFile();
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
