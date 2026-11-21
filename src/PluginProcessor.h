#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "MPChainDSP.h"
#include <atomic>

class MPChainAudioProcessor : public juce::AudioProcessor
{
public:
    MPChainAudioProcessor();
    ~MPChainAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MPCHAIN"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram_; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> inPeakDb       { -96.0f };
    std::atomic<float> inLufsDb       { -96.0f };
    std::atomic<float> outPeakDb      { -96.0f };
    std::atomic<float> outLufsDb      { -96.0f };
    std::atomic<float> outLufsShortDb { -96.0f };
    std::atomic<float> outLufsIntDb   { -96.0f };
    std::atomic<float> outRmsDb       { -96.0f };
    std::atomic<float> outPlrDb       {   0.0f };

    void copyRecentAudioForSpectrum(float* dst, int numSamples) const { dsp_.copyRecentAudio(dst, numSamples); }
    double getDspSampleRate() const { return dsp_.sampleRate(); }
    void requestResetIntegratedLoudness() { resetIntReq_.store(true, std::memory_order_release); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    mixchain::MPChainDSP dsp_;
    std::atomic<bool> resetIntReq_{ false };
    int currentProgram_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MPChainAudioProcessor)
};
