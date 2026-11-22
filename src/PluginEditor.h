#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

struct MPChainTheme
{
    juce::Colour top0, top1, crestDark, crestFill, crestLine, gridLine;
    juce::Colour bot0, bot1, horizon;
    juce::Colour accent, accentClip, accentText;
    juce::Colour topText, topMuted, deckText, deckMuted;
    juce::Colour knobBody, knobLight, bezel0, bezel1, track, meterWell, btnBg, btnBorder, btnText, targetTick;
};

class MPChainLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MPChainTheme theme;

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override;

    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle, juce::Slider&) override;

    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

class MultimeterStrip : public juce::Component
{
public:
    void setLevels(float peakDb, float lufsDb, const MPChainTheme& t);
    void paint(juce::Graphics& g) override;

private:
    float peakDb_ = -96.0f, lufsDb_ = -96.0f, holdDb_ = -96.0f;
    MPChainTheme theme_;
};

class MPChainAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit MPChainAudioProcessorEditor(MPChainAudioProcessor&);
    ~MPChainAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void updateSpectrum();
    void applySkin(int skinIdx);
    void updateHqButtons(int hqIdx);
    void updateAutoButtons(int autoIdx);
    void updateSatButtons(int satIdx);
    void refreshPresetCombo(const juce::String& selectUserName = {});
    void promptSaveUserPreset();
    void configureKnob(juce::Slider& s, const juce::String& suffix, bool bipolar, bool isHero = false, bool isMini = false);

    static constexpr int kFftOrder = 11; // 2048-pt FFT
    static constexpr int kFftSize  = 1 << kFftOrder;
    static constexpr int kSpecPts  = 128;

    MPChainAudioProcessor& proc_;
    MPChainLookAndFeel lnf_;
    int currentSkin_ = -1;
    int currentHq_   = -1;
    int currentAuto_ = -1;
    int currentSat_  = -1;

    float fftWin_[kFftSize]     = {};
    float fftRe_[kFftSize]      = {};
    float fftIm_[kFftSize]      = {};
    int   fftBitRev_[kFftSize]  = {};
    float specSmooth_[kSpecPts] = {};
    float specPeak_[kSpecPts]   = {};
    juce::Rectangle<float> lufsIntHitRect_;

    MultimeterStrip inMeter_, outMeter_;

    juce::Slider louderKnob_, widthXfader_, ceilingSlider_;
    juce::Slider eqLowKnob_, eqLowMidKnob_, eqMidKnob_, eqHighKnob_;
    juce::Slider glueKnob_, hpfKnob_;
    juce::Slider inGainKnob_, outGainKnob_;

    juce::ToggleButton cleanBtn_{ "CLEAN" };
    juce::ToggleButton airBtn_{ "AIR +1 dB" };

    juce::TextButton auto14Btn_{ "AUTO -14" }, auto11Btn_{ "-11 LUFS" }, auto9Btn_{ "-9 LUFS" };
    juce::TextButton satTapeBtn_{ "TAPE" }, satTubeBtn_{ "TUBE" }, satXfmrBtn_{ "XFMR" }, satClassABtn_{ "CLS-A" };

    juce::TextButton hq1Btn_{ "1x" }, hq2Btn_{ "2x 96k" }, hq4Btn_{ "4x 192k" }, hq8Btn_{ "8x 384k" };
    juce::TextButton skinRedBtn_{ "RED" }, skinRetroBtn_{ "RETRO" }, skinBlueBtn_{ "BLUE" };

    juce::ComboBox presetBox_;
    juce::TextButton savePresetBtn_{ "SAVE" }, delPresetBtn_{ "DEL" };
    juce::StringArray userPresetNames_;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAtt> louderAtt_, widthAtt_, ceilAtt_;
    std::unique_ptr<SliderAtt> eqLowAtt_, eqLowMidAtt_, eqMidAtt_, eqHighAtt_;
    std::unique_ptr<SliderAtt> glueAtt_, hpfAtt_, inGainAtt_, outGainAtt_;
    std::unique_ptr<ButtonAtt> cleanAtt_, airAtt_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MPChainAudioProcessorEditor)
};
