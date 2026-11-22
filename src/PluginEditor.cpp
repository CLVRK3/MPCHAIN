#include "PluginEditor.h"

namespace {

MPChainTheme makeTheme(int idx)
{
    if (idx == 0) // MPC Red
    {
        return {
            juce::Colour(0xff30171e), juce::Colour(0xff4a1c28), juce::Colour(0xff6e192d),
            juce::Colour(0xffa82442), juce::Colour(0xfff4e6ea), juce::Colour(0xff5c2a37),
            juce::Colour(0xff222229), juce::Colour(0xff15151a), juce::Colour(0xff3f3f4a),
            juce::Colour(0xffe81f40), juce::Colour(0xffff8c24), juce::Colour(0xffffffff),
            juce::Colour(0xfff7ecee), juce::Colour(0xffc9a4ad), juce::Colour(0xfff2f2f6), juce::Colour(0xff9b9baa),
            juce::Colour(0xff16161b), juce::Colour(0xff383842), juce::Colour(0xff4e4e5a), juce::Colour(0xff25252d),
            juce::Colour(0xff343440), juce::Colour(0xff0d0d12), juce::Colour(0xff2a2a33), juce::Colour(0xff454552),
            juce::Colour(0xffe0e0e8), juce::Colour(0xffff3855)
        };
    }
    if (idx == 1) // Retro 3000
    {
        return {
            juce::Colour(0xffe7e2d6), juce::Colour(0xffd5ccb9), juce::Colour(0xff3d5c82),
            juce::Colour(0xff6889b0), juce::Colour(0xfff8f6f0), juce::Colour(0xffc0b7a5),
            juce::Colour(0xffedeae2), juce::Colour(0xffd8d2c4), juce::Colour(0xffa69e8e),
            juce::Colour(0xff1f5ea0), juce::Colour(0xffd44628), juce::Colour(0xffffffff),
            juce::Colour(0xff1f242c), juce::Colour(0xff545b66), juce::Colour(0xff1e222a), juce::Colour(0xff575d68),
            juce::Colour(0xff1e2229), juce::Colour(0xff464d59), juce::Colour(0xfff4f1ea), juce::Colour(0xffb8b0a0),
            juce::Colour(0xffb8b0a0), juce::Colour(0xff1e2229), juce::Colour(0xffcfc7b6), juce::Colour(0xff9a9180),
            juce::Colour(0xff262b34), juce::Colour(0xffc93c24)
        };
    }
    // Default 2: Obsidian Blue (Stealth Black primary + Electric Cobalt #1F82FF secondary)
    return {
        juce::Colour(0xff07090d), juce::Colour(0xff0d1726), juce::Colour(0xff0f3670),
        juce::Colour(0xff1c6be0), juce::Colour(0xffd6ecff), juce::Colour(0xff2e558a),
        juce::Colour(0xff12151b), juce::Colour(0xff08090c), juce::Colour(0xff203b63),
        juce::Colour(0xff1f82ff), juce::Colour(0xffff6a33), juce::Colour(0xffffffff),
        juce::Colour(0xfff0f6ff), juce::Colour(0xff738eb3), juce::Colour(0xffe6eef9), juce::Colour(0xff6f89ad),
        juce::Colour(0xff0d1015), juce::Colour(0xff2d3a4f), juce::Colour(0xff364154), juce::Colour(0xff0e1117),
        juce::Colour(0xff1d2432), juce::Colour(0xff07090d), juce::Colour(0xff161b24), juce::Colour(0xff2a384f),
        juce::Colour(0xff9ab2d4), juce::Colour(0xff66b3ff)
    };
}

} // namespace

void MPChainLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(2.5f);
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float cx = bounds.getCentreX(), cy = bounds.getCentreY();
    const bool isHero = (bool)slider.getProperties().getWithDefault("hero", false);
    const bool isBipolar = (bool)slider.getProperties().getWithDefault("bipolar", false);
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    if (isHero)
    {
        juce::ColourGradient bez(theme.bezel0, cx, cy - radius, theme.bezel1, cx, cy + radius, false);
        g.setGradientFill(bez);
        g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
        const float clipFrac = 9.0f / 24.0f;
        g.setColour(sliderPos > clipFrac ? theme.accentClip.withAlpha(0.85f) : theme.accent.withAlpha(0.55f));
        g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.5f);
    }

    const float arcW = isHero ? 6.0f : (radius > 28.0f ? 3.8f : 2.5f);
    const float arcR = radius - (isHero ? 9.5f : arcW * 0.5f);

    juce::Path track;
    track.addCentredArc(cx, cy, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour(theme.track);
    g.strokePath(track, juce::PathStrokeType(arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (isHero)
    {
        // +9.0 dB clip onset marker on 0..+24 dB scale (9/24 = 0.375)
        const float clipFrac = 9.0f / 24.0f;
        const float clipAng  = startAngle + clipFrac * (endAngle - startAngle);
        if (sliderPos > 0.002f)
        {
            juce::Path a1;
            a1.addCentredArc(cx, cy, arcR, arcR, 0.0f, startAngle, juce::jmin(angle, clipAng), true);
            g.setColour(theme.accent);
            g.strokePath(a1, juce::PathStrokeType(arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            if (sliderPos > clipFrac)
            {
                juce::Path a2;
                a2.addCentredArc(cx, cy, arcR, arcR, 0.0f, clipAng, angle, true);
                g.setColour(theme.accentClip);
                g.strokePath(a2, juce::PathStrokeType(arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }
        g.setColour(theme.accentClip);
        g.drawLine(cx + std::sin(clipAng) * (arcR - 5.0f), cy - std::cos(clipAng) * (arcR - 5.0f),
                   cx + std::sin(clipAng) * (arcR + 5.0f), cy - std::cos(clipAng) * (arcR + 5.0f), 2.3f);
    }
    else
    {
        const float midAngle = (startAngle + endAngle) * 0.5f;
        const float a0 = isBipolar ? juce::jmin(midAngle, angle) : startAngle;
        const float a1 = isBipolar ? juce::jmax(midAngle, angle) : angle;
        if (a1 - a0 > 0.01f)
        {
            juce::Path valArc;
            valArc.addCentredArc(cx, cy, arcR, arcR, 0.0f, a0, a1, true);
            g.setColour(theme.accent);
            g.strokePath(valArc, juce::PathStrokeType(arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    const float bodyR = arcR - arcW - 2.0f;
    g.setColour(theme.knobBody);
    g.fillEllipse(cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour(theme.knobLight);
    g.drawEllipse(cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.2f);

    juce::Path ptr;
    ptr.startNewSubPath(cx + std::sin(angle) * bodyR * (isHero ? 0.16f : 0.28f),
                        cy - std::cos(angle) * bodyR * (isHero ? 0.16f : 0.28f));
    ptr.lineTo(cx + std::sin(angle) * (bodyR - 3.5f), cy - std::cos(angle) * (bodyR - 3.5f));
    g.setColour(juce::Colour(0xffeaf4ff));
    g.strokePath(ptr, juce::PathStrokeType(isHero ? 3.0f : 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void MPChainLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float, float, juce::Slider::SliderStyle, juce::Slider& slider)
{
    const bool isMiniCeil = (bool)slider.getProperties().getWithDefault("miniCeil", false);
    if (isMiniCeil)
    {
        const auto tr = juce::Rectangle<float>((float)x + 3.0f, (float)y + (float)height * 0.5f - 3.5f, (float)width - 6.0f, 7.0f);
        g.setColour(theme.meterWell);
        g.fillRoundedRectangle(tr, 3.5f);
        g.setColour(theme.btnBorder);
        g.drawRoundedRectangle(tr, 3.5f, 0.9f);
        const float cx = juce::jlimit(tr.getX() + 4.0f, tr.getRight() - 4.0f, sliderPos);
        g.setColour(theme.accent);
        g.fillRoundedRectangle(tr.getX() + 1.0f, tr.getY() + 1.0f, juce::jmax(2.0f, cx - tr.getX()), tr.getHeight() - 2.0f, 2.5f);
        g.setColour(juce::Colours::white);
        g.fillEllipse(cx - 4.5f, tr.getCentreY() - 4.5f, 9.0f, 9.0f);
        g.setColour(theme.accent);
        g.drawEllipse(cx - 4.5f, tr.getCentreY() - 4.5f, 9.0f, 9.0f, 1.3f);
        return;
    }

    const auto pill = juce::Rectangle<float>((float)x + 2.0f, (float)y + 3.0f, (float)width - 4.0f, (float)height - 6.0f);
    g.setColour(theme.knobBody);
    g.fillRoundedRectangle(pill, pill.getHeight() * 0.5f);
    g.setColour(theme.knobLight);
    g.drawRoundedRectangle(pill, pill.getHeight() * 0.5f, 1.2f);

    const float left = pill.getX() + 20.0f, right = pill.getRight() - 20.0f;
    const float midX = (left + right) * 0.5f, midY = pill.getCentreY();

    g.setColour(theme.knobLight);
    g.fillRect(left - 7.0f, pill.getY() + 6.0f, 2.2f, pill.getHeight() - 12.0f);
    g.fillRect(right + 4.8f, pill.getY() + 6.0f, 2.2f, pill.getHeight() - 12.0f);
    g.setColour(theme.accent.withAlpha(0.75f));
    g.fillRect(midX - 1.0f, pill.getY() + 5.0f, 2.0f, pill.getHeight() - 10.0f);

    const float capW = 36.0f, capH = pill.getHeight() - 8.0f;
    const float cx = juce::jlimit(left + capW * 0.5f, right - capW * 0.5f, sliderPos);
    g.setColour(theme.accent);
    g.fillRoundedRectangle(juce::jmin(midX, cx), midY - 2.0f, std::fabs(cx - midX), 4.0f, 2.0f);

    const juce::Rectangle<float> cap(cx - capW * 0.5f, midY - capH * 0.5f, capW, capH);
    g.setColour(theme.knobBody.interpolatedWith(theme.accent, 0.25f));
    g.fillRoundedRectangle(cap, capH * 0.5f);
    g.setColour(theme.accent);
    g.drawRoundedRectangle(cap, capH * 0.5f, 1.8f);
    g.setColour(juce::Colours::white);
    g.fillRoundedRectangle(cx - 1.2f, cap.getY() + 4.0f, 2.4f, cap.getHeight() - 8.0f, 1.2f);
}

void MPChainLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    const bool on = b.getToggleState();
    const auto r = b.getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(on ? theme.accent : theme.btnBg);
    g.fillRoundedRectangle(r, r.getHeight() * 0.5f);
    g.setColour(on ? theme.accent : theme.btnBorder);
    g.drawRoundedRectangle(r, r.getHeight() * 0.5f, 1.1f);
    g.setColour(on ? theme.accentText : theme.btnText);
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.drawText(b.getButtonText() + (on ? " ON" : ""), r, juce::Justification::centred);
}

void MultimeterStrip::setLevels(float peakDb, float lufsDb, const MPChainTheme& t)
{
    peakDb_ = peakDb; lufsDb_ = lufsDb; holdDb_ = juce::jmax(peakDb, holdDb_ - 0.16f);
    theme_ = t; repaint();
}

void MultimeterStrip::paint(juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    // Sleeker, razor-thin LED multimeter strips (6.0 px wide)
    const float barW = 6.0f;
    const auto pkRect = juce::Rectangle<float>(b.getX() + 6.0f, b.getY(), barW, b.getHeight());
    const auto luRect = juce::Rectangle<float>(b.getRight() - barW - 6.0f, b.getY(), barW, b.getHeight());

    auto drawBar = [&](juce::Rectangle<float> r, float db, bool isLufs) {
        g.setColour(theme_.meterWell);
        g.fillRoundedRectangle(r, 2.0f);
        const float norm = juce::jlimit(0.0f, 1.0f, (db + 36.0f) / 36.0f);
        const int segs = 38; const float gap = 1.4f;
        const float sh = (r.getHeight() - gap * (segs + 1)) / (float)segs;
        for (int i = 0; i < segs; ++i)
        {
            const float f = (float)(i + 1) / (float)segs;
            const float sy = r.getBottom() - (float)(i + 1) * (sh + gap);
            const bool lit = f <= norm;
            juce::Colour col = f > 0.90f ? (lit ? juce::Colour(0xffff2a3c) : juce::Colour(0x22ff2a3c))
                             : f > 0.72f ? (lit ? juce::Colour(0xffffba24) : juce::Colour(0x22ffba24))
                             : (lit ? juce::Colour(isLufs ? 0xff7df0a0 : 0xff5ce674) : juce::Colour(0x1f5ce674));
            g.setColour(col);
            g.fillRoundedRectangle(r.getX() + 0.8f, sy, r.getWidth() - 1.6f, sh, 0.8f);
        }
        const float ty = r.getBottom() - r.getHeight() * (22.0f / 36.0f);
        g.setColour(theme_.targetTick);
        g.fillRect(r.getX() - 1.5f, ty - 0.8f, r.getWidth() + 3.0f, 1.6f);
        if (!isLufs)
        {
            const float hn = juce::jlimit(0.0f, 1.0f, (holdDb_ + 36.0f) / 36.0f);
            if (hn > 0.02f)
            {
                g.setColour(holdDb_ > -0.5f ? juce::Colour(0xffff384b) : juce::Colours::white);
                g.fillRect(r.getX() + 0.5f, r.getBottom() - r.getHeight() * hn, r.getWidth() - 1.0f, 1.8f);
            }
        }
    };
    drawBar(pkRect, peakDb_, false);
    drawBar(luRect, lufsDb_, true);

    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.setColour(theme_.topMuted);
    g.drawText("0", juce::Rectangle<float>(pkRect.getRight(), b.getY(), luRect.getX() - pkRect.getRight(), 12.0f), juce::Justification::centred);
    g.setColour(theme_.targetTick);
    const float ty = b.getBottom() - b.getHeight() * (22.0f / 36.0f);
    g.drawText("-14", juce::Rectangle<float>(pkRect.getRight() - 2.0f, ty - 6.0f, luRect.getX() - pkRect.getRight() + 4.0f, 12.0f), juce::Justification::centred);
    g.setColour(theme_.deckMuted);
    g.drawText("-36", juce::Rectangle<float>(pkRect.getRight(), b.getBottom() - 12.0f, luRect.getX() - pkRect.getRight(), 12.0f), juce::Justification::centred);
}

MPChainAudioProcessorEditor::MPChainAudioProcessorEditor(MPChainAudioProcessor& p)
    : AudioProcessorEditor(&p), proc_(p)
{
    setLookAndFeel(&lnf_);
    setSize(1080, 540);
    setResizable(true, true);
    setResizeLimits(880, 440, 1600, 800);

    configureKnob(louderKnob_,   " dB", false, true);
    configureKnob(eqLowKnob_,    " dB", true);
    configureKnob(eqLowMidKnob_, " dB", true);
    configureKnob(eqMidKnob_,    " dB", true);
    configureKnob(eqHighKnob_,   " dB", true);
    configureKnob(glueKnob_,     "%",   false);
    configureKnob(hpfKnob_,      " Hz", false, false, true);
    configureKnob(inGainKnob_,   " dB", true,  false, true);
    configureKnob(outGainKnob_,  " dB", true,  false, true);

    widthXfader_.setSliderStyle(juce::Slider::LinearHorizontal);
    widthXfader_.setTextBoxStyle(juce::Slider::TextBoxAbove, false, 68, 16);
    widthXfader_.setTextValueSuffix("%");
    addAndMakeVisible(widthXfader_);

    ceilingSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    ceilingSlider_.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    ceilingSlider_.getProperties().set("miniCeil", true);
    addAndMakeVisible(ceilingSlider_);

    addAndMakeVisible(inMeter_);
    addAndMakeVisible(outMeter_);
    addAndMakeVisible(cleanBtn_);
    addAndMakeVisible(airBtn_);

    // HQ oversampling target rates: 1x | 2x 96k | 4x 192k | 8x 384k
    auto setupHq = [&](juce::TextButton& b, int idx) {
        b.onClick = [this, idx] {
            if (auto* param = proc_.apvts.getParameter("hq_os"))
                param->setValueNotifyingHost(param->convertTo0to1((float)idx));
            updateHqButtons(idx);
        };
        addAndMakeVisible(b);
    };
    setupHq(hq1Btn_, 0); setupHq(hq2Btn_, 1); setupHq(hq4Btn_, 2); setupHq(hq8Btn_, 3);

    // 3 Skins: RED | RETRO | BLUE (Obsidian Blue default)
    auto setupSkin = [&](juce::TextButton& b, int idx) {
        b.onClick = [this, idx] {
            if (auto* param = proc_.apvts.getParameter("skin"))
                param->setValueNotifyingHost(param->convertTo0to1((float)idx));
            applySkin(idx);
        };
        addAndMakeVisible(b);
    };
    setupSkin(skinRedBtn_, 0); setupSkin(skinRetroBtn_, 1); setupSkin(skinBlueBtn_, 2);

    // Auto-LUFS clickable toggles: -9 LUFS, -11 LUFS (smaller above) + AUTO -14 (clicking active toggles OFF)
    auto setupAuto = [&](juce::TextButton& b, int tgtVal) {
        b.onClick = [this, tgtVal] {
            const int cur = (int)std::lround(proc_.apvts.getRawParameterValue("auto_target")->load());
            const int next = (cur == tgtVal) ? mixchain::kAutoOff : tgtVal;
            if (auto* param = proc_.apvts.getParameter("auto_target"))
                param->setValueNotifyingHost(param->convertTo0to1((float)next));
            updateAutoButtons(next);
        };
        addAndMakeVisible(b);
    };
    setupAuto(auto14Btn_, mixchain::kAuto14);
    setupAuto(auto11Btn_, mixchain::kAuto11);
    setupAuto(auto9Btn_,  mixchain::kAuto9);

    // 4 Bypassable Stereo-Bus Saturation Mode switches underneath LOUDER (clicking active bypasses)
    auto setupSat = [&](juce::TextButton& b, int modeVal) {
        b.onClick = [this, modeVal] {
            const int cur = (int)std::lround(proc_.apvts.getRawParameterValue("sat_mode")->load());
            const int next = (cur == modeVal) ? mixchain::kSatOff : modeVal;
            if (auto* param = proc_.apvts.getParameter("sat_mode"))
                param->setValueNotifyingHost(param->convertTo0to1((float)next));
            updateSatButtons(next);
        };
        addAndMakeVisible(b);
    };
    setupSat(satTapeBtn_,   mixchain::kSatTape);
    setupSat(satTubeBtn_,   mixchain::kSatTube);
    setupSat(satXfmrBtn_,   mixchain::kSatXfmr);
    setupSat(satClassABtn_, mixchain::kSatClassA);

    // Preset selector + Save / Delete user presets
    refreshPresetCombo();
    presetBox_.onChange = [this] {
        const int id = presetBox_.getSelectedId();
        const int numFact = proc_.getNumPrograms();
        if (id >= 1 && id <= numFact)
        {
            proc_.setCurrentProgram(id - 1);
            delPresetBtn_.setEnabled(false);
        }
        else if (id >= 1001)
        {
            const int uIdx = id - 1001;
            if (uIdx >= 0 && uIdx < userPresetNames_.size())
            {
                proc_.loadUserPreset(userPresetNames_[uIdx]);
                delPresetBtn_.setEnabled(true);
            }
        }
    };
    addAndMakeVisible(presetBox_);

    savePresetBtn_.onClick = [this] { promptSaveUserPreset(); };
    delPresetBtn_.onClick = [this] {
        const int id = presetBox_.getSelectedId();
        if (id >= 1001)
        {
            const int uIdx = id - 1001;
            if (uIdx >= 0 && uIdx < userPresetNames_.size())
            {
                proc_.deleteUserPreset(userPresetNames_[uIdx]);
                refreshPresetCombo();
            }
        }
    };
    delPresetBtn_.setEnabled(false);
    addAndMakeVisible(savePresetBtn_);
    addAndMakeVisible(delPresetBtn_);

    louderAtt_   = std::make_unique<SliderAtt>(proc_.apvts, "loud",       louderKnob_);
    widthAtt_    = std::make_unique<SliderAtt>(proc_.apvts, "width",      widthXfader_);
    ceilAtt_     = std::make_unique<SliderAtt>(proc_.apvts, "ceiling",    ceilingSlider_);
    eqLowAtt_    = std::make_unique<SliderAtt>(proc_.apvts, "eq_low",     eqLowKnob_);
    eqLowMidAtt_ = std::make_unique<SliderAtt>(proc_.apvts, "eq_lowmid",  eqLowMidKnob_);
    eqMidAtt_    = std::make_unique<SliderAtt>(proc_.apvts, "eq_mid",     eqMidKnob_);
    eqHighAtt_   = std::make_unique<SliderAtt>(proc_.apvts, "eq_high",    eqHighKnob_);
    glueAtt_     = std::make_unique<SliderAtt>(proc_.apvts, "vibe",       glueKnob_);
    hpfAtt_      = std::make_unique<SliderAtt>(proc_.apvts, "comp_hpf",   hpfKnob_);
    inGainAtt_   = std::make_unique<SliderAtt>(proc_.apvts, "in_gain",    inGainKnob_);
    outGainAtt_  = std::make_unique<SliderAtt>(proc_.apvts, "out_gain",   outGainKnob_);
    cleanAtt_    = std::make_unique<ButtonAtt>(proc_.apvts, "treble_clean", cleanBtn_);
    airAtt_      = std::make_unique<ButtonAtt>(proc_.apvts, "air_boost",    airBtn_);

    for (int i = 0; i < kFftSize; ++i)
    {
        fftWin_[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * (float)i / (float)(kFftSize - 1)));
        int r = 0, x = i;
        for (int b = 0; b < kFftOrder; ++b) { r = (r << 1) | (x & 1); x >>= 1; }
        fftBitRev_[i] = r;
    }
    for (int i = 0; i < kSpecPts; ++i) { specSmooth_[i] = 0.0f; specPeak_[i] = 0.0f; }

    applySkin((int)std::lround(proc_.apvts.getRawParameterValue("skin")->load()));
    updateHqButtons((int)std::lround(proc_.apvts.getRawParameterValue("hq_os")->load()));
    updateAutoButtons((int)std::lround(proc_.apvts.getRawParameterValue("auto_target")->load()));
    updateSatButtons((int)std::lround(proc_.apvts.getRawParameterValue("sat_mode")->load()));
    startTimerHz(45);
}

MPChainAudioProcessorEditor::~MPChainAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void MPChainAudioProcessorEditor::refreshPresetCombo(const juce::String& selectUserName)
{
    presetBox_.clear(juce::dontSendNotification);
    const int numFact = proc_.getNumPrograms();
    for (int i = 0; i < numFact; ++i)
        presetBox_.addItem(proc_.getProgramName(i), i + 1);

    userPresetNames_ = proc_.getUserPresetNames();
    if (!userPresetNames_.isEmpty())
    {
        presetBox_.addSeparator();
        presetBox_.addSectionHeading("User Presets");
        for (int i = 0; i < userPresetNames_.size(); ++i)
            presetBox_.addItem(userPresetNames_[i], 1001 + i);
    }

    if (selectUserName.isNotEmpty())
    {
        const int idx = userPresetNames_.indexOf(selectUserName);
        if (idx >= 0)
        {
            presetBox_.setSelectedId(1001 + idx, juce::dontSendNotification);
            delPresetBtn_.setEnabled(true);
            return;
        }
    }
    presetBox_.setSelectedId(proc_.getCurrentProgram() + 1, juce::dontSendNotification);
    delPresetBtn_.setEnabled(false);
}

void MPChainAudioProcessorEditor::promptSaveUserPreset()
{
    auto* aw = new juce::AlertWindow("Save User Preset", "Enter a name for this MPCHAIN preset:", juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor("name", "My Master", "Preset Name:");
    aw->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    aw->enterModalState(true, juce::ModalCallbackFunction::create([this, aw](int res) {
        if (res == 1)
        {
            const auto name = aw->getTextEditorContents("name").trim();
            if (name.isNotEmpty() && proc_.saveUserPreset(name))
                refreshPresetCombo(juce::File::createLegalFileName(name));
        }
        delete aw;
    }));
}

void MPChainAudioProcessorEditor::configureKnob(juce::Slider& s, const juce::String& suffix, bool bipolar, bool isHero, bool isMini)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, isHero ? 88 : (isMini ? 56 : 64), isHero ? 20 : 15);
    s.setTextValueSuffix(suffix);
    s.getProperties().set("bipolar", bipolar);
    s.getProperties().set("hero", isHero);
    addAndMakeVisible(s);
}

void MPChainAudioProcessorEditor::applySkin(int skinIdx)
{
    skinIdx = juce::jlimit(0, 2, skinIdx);
    currentSkin_ = skinIdx;
    lnf_.theme = makeTheme(skinIdx);

    auto styleSlider = [&](juce::Slider& s) {
        s.setColour(juce::Slider::textBoxTextColourId, lnf_.theme.deckText);
        s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    };
    for (auto* s : { &louderKnob_, &widthXfader_, &eqLowKnob_, &eqLowMidKnob_, &eqMidKnob_, &eqHighKnob_,
                     &glueKnob_, &hpfKnob_, &inGainKnob_, &outGainKnob_ })
        styleSlider(*s);

    auto styleBtn = [&](juce::TextButton& b, bool active) {
        b.setColour(juce::TextButton::buttonColourId, active ? lnf_.theme.accent : lnf_.theme.btnBg);
        b.setColour(juce::TextButton::textColourOffId, active ? lnf_.theme.accentText : lnf_.theme.btnText);
    };
    styleBtn(skinRedBtn_,   skinIdx == 0);
    styleBtn(skinRetroBtn_, skinIdx == 1);
    styleBtn(skinBlueBtn_,  skinIdx == 2);
    styleBtn(savePresetBtn_, false);
    styleBtn(delPresetBtn_,  false);

    presetBox_.setColour(juce::ComboBox::backgroundColourId, lnf_.theme.btnBg);
    presetBox_.setColour(juce::ComboBox::outlineColourId,    lnf_.theme.btnBorder);
    presetBox_.setColour(juce::ComboBox::textColourId,       lnf_.theme.btnText);
    presetBox_.setColour(juce::ComboBox::arrowColourId,      lnf_.theme.btnText);
    lnf_.setColour(juce::PopupMenu::backgroundColourId,      lnf_.theme.bot0);
    lnf_.setColour(juce::PopupMenu::textColourId,            lnf_.theme.deckText);
    lnf_.setColour(juce::PopupMenu::highlightedBackgroundColourId, lnf_.theme.accent);
    lnf_.setColour(juce::PopupMenu::highlightedTextColourId, lnf_.theme.accentText);

    updateHqButtons(currentHq_ < 0 ? 1 : currentHq_);
    updateAutoButtons(currentAuto_ < 0 ? 0 : currentAuto_);
    updateSatButtons(currentSat_ < 0 ? 0 : currentSat_);
    repaint();
}

void MPChainAudioProcessorEditor::updateHqButtons(int hqIdx)
{
    hqIdx = juce::jlimit(0, 3, hqIdx);
    currentHq_ = hqIdx;
    auto styleBtn = [&](juce::TextButton& b, bool active) {
        b.setColour(juce::TextButton::buttonColourId, active ? lnf_.theme.accent : lnf_.theme.btnBg);
        b.setColour(juce::TextButton::textColourOffId, active ? lnf_.theme.accentText : lnf_.theme.btnText);
    };
    styleBtn(hq1Btn_, hqIdx == 0); styleBtn(hq2Btn_, hqIdx == 1);
    styleBtn(hq4Btn_, hqIdx == 2); styleBtn(hq8Btn_, hqIdx == 3);
}

void MPChainAudioProcessorEditor::updateAutoButtons(int autoIdx)
{
    autoIdx = juce::jlimit(0, 3, autoIdx);
    currentAuto_ = autoIdx;
    auto styleBtn = [&](juce::TextButton& b, bool active) {
        b.setColour(juce::TextButton::buttonColourId, active ? lnf_.theme.accent : lnf_.theme.btnBg);
        b.setColour(juce::TextButton::textColourOffId, active ? lnf_.theme.accentText : lnf_.theme.btnText);
    };
    styleBtn(auto14Btn_, autoIdx == mixchain::kAuto14);
    styleBtn(auto11Btn_, autoIdx == mixchain::kAuto11);
    styleBtn(auto9Btn_,  autoIdx == mixchain::kAuto9);
}

void MPChainAudioProcessorEditor::updateSatButtons(int satIdx)
{
    satIdx = juce::jlimit(0, 4, satIdx);
    currentSat_ = satIdx;
    auto styleBtn = [&](juce::TextButton& b, bool active) {
        b.setColour(juce::TextButton::buttonColourId, active ? lnf_.theme.accent : lnf_.theme.btnBg);
        b.setColour(juce::TextButton::textColourOffId, active ? lnf_.theme.accentText : lnf_.theme.btnText);
    };
    styleBtn(satTapeBtn_,   satIdx == mixchain::kSatTape);
    styleBtn(satTubeBtn_,   satIdx == mixchain::kSatTube);
    styleBtn(satXfmrBtn_,   satIdx == mixchain::kSatXfmr);
    styleBtn(satClassABtn_, satIdx == mixchain::kSatClassA);
}

void MPChainAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    if (lufsIntHitRect_.contains(e.position))
        proc_.requestResetIntegratedLoudness();
}

void MPChainAudioProcessorEditor::updateSpectrum()
{
    proc_.copyRecentAudioForSpectrum(fftRe_, kFftSize);

    float tmp[kFftSize];
    for (int i = 0; i < kFftSize; ++i) tmp[i] = fftRe_[i] * fftWin_[i];
    for (int i = 0; i < kFftSize; ++i) { fftRe_[fftBitRev_[i]] = tmp[i]; fftIm_[i] = 0.0f; }

    for (int len = 2; len <= kFftSize; len <<= 1)
    {
        const int half = len >> 1;
        const float ang = -2.0f * juce::MathConstants<float>::pi / (float)len;
        const float wStepRe = std::cos(ang), wStepIm = std::sin(ang);
        for (int i = 0; i < kFftSize; i += len)
        {
            float wRe = 1.0f, wIm = 0.0f;
            for (int j = 0; j < half; ++j)
            {
                const int u = i + j, v = u + half;
                const float tRe = fftRe_[v] * wRe - fftIm_[v] * wIm;
                const float tIm = fftRe_[v] * wIm + fftIm_[v] * wRe;
                fftRe_[v] = fftRe_[u] - tRe; fftIm_[v] = fftIm_[u] - tIm;
                fftRe_[u] += tRe;            fftIm_[u] += tIm;
                const float nextRe = wRe * wStepRe - wIm * wStepIm;
                wIm = wRe * wStepIm + wIm * wStepRe;
                wRe = nextRe;
            }
        }
    }

    const float sr = (float)juce::jmax(8000.0, proc_.getDspSampleRate());
    const float minHz = 30.0f, maxHz = 20000.0f, logRatio = std::log(maxHz / minHz);
    const float invNorm = 2.0f / (float)kFftSize;

    float raw[kSpecPts];
    for (int p = 0; p < kSpecPts; ++p)
    {
        const float fC  = minHz * std::exp(logRatio * ((float)p / (float)(kSpecPts - 1)));
        const float fLo = minHz * std::exp(logRatio * juce::jmax(0.0f, ((float)p - 0.5f) / (float)(kSpecPts - 1)));
        const float fHi = minHz * std::exp(logRatio * juce::jmin(1.0f, ((float)p + 0.5f) / (float)(kSpecPts - 1)));
        const int bLo = juce::jlimit(1, kFftSize / 2 - 1, (int)std::floor(fLo * (float)kFftSize / sr));
        const int bHi = juce::jlimit(bLo, kFftSize / 2 - 1, (int)std::ceil(fHi * (float)kFftSize / sr));

        float magSq = 0.0f;
        if (bHi == bLo)
        {
            const float binF = fC * (float)kFftSize / sr;
            const int b0 = juce::jlimit(1, kFftSize / 2 - 2, (int)binF);
            const float fr = binF - (float)b0;
            const float m0 = fftRe_[b0] * fftRe_[b0] + fftIm_[b0] * fftIm_[b0];
            const float m1 = fftRe_[b0 + 1] * fftRe_[b0 + 1] + fftIm_[b0 + 1] * fftIm_[b0 + 1];
            magSq = m0 + (m1 - m0) * fr;
        }
        else
        {
            float sum = 0.0f;
            for (int b = bLo; b <= bHi; ++b) sum += fftRe_[b] * fftRe_[b] + fftIm_[b] * fftIm_[b];
            magSq = sum / (float)(bHi - bLo + 1);
        }

        const float mag = std::sqrt(magSq) * invNorm * 2.0f;
        const float tiltDb = 3.0f * std::log2(fC / 500.0f);
        const float db = (mag > 1.0e-6f ? 20.0f * std::log10(mag) : -96.0f) + tiltDb;
        raw[p] = juce::jlimit(0.0f, 1.0f, (db + 72.0f) / 68.0f);
    }

    for (int p = 0; p < kSpecPts; ++p)
    {
        const float l = raw[juce::jmax(0, p - 1)], c = raw[p], r = raw[juce::jmin(kSpecPts - 1, p + 1)];
        const float v = 0.25f * l + 0.50f * c + 0.25f * r;
        specSmooth_[p] = v > specSmooth_[p] ? (specSmooth_[p] + 0.45f * (v - specSmooth_[p]))
                                            : (specSmooth_[p] + 0.14f * (v - specSmooth_[p]));
        specPeak_[p]   = v > specPeak_[p]   ? v : juce::jmax(specSmooth_[p], specPeak_[p] - 0.0055f);
    }
}

void MPChainAudioProcessorEditor::timerCallback()
{
    const int skin = (int)std::lround(proc_.apvts.getRawParameterValue("skin")->load());
    if (skin != currentSkin_) applySkin(skin);
    const int hq = (int)std::lround(proc_.apvts.getRawParameterValue("hq_os")->load());
    if (hq != currentHq_) updateHqButtons(hq);
    const int au = (int)std::lround(proc_.apvts.getRawParameterValue("auto_target")->load());
    if (au != currentAuto_) updateAutoButtons(au);
    const int st = (int)std::lround(proc_.apvts.getRawParameterValue("sat_mode")->load());
    if (st != currentSat_) updateSatButtons(st);

    updateSpectrum();
    inMeter_.setLevels(proc_.inPeakDb.load(std::memory_order_relaxed), proc_.inLufsDb.load(std::memory_order_relaxed), lnf_.theme);
    outMeter_.setLevels(proc_.outPeakDb.load(std::memory_order_relaxed), proc_.outLufsDb.load(std::memory_order_relaxed), lnf_.theme);
    repaint(0, 0, getWidth(), (int)(getHeight() * 0.48f));
}

void MPChainAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto& t = lnf_.theme;
    const auto b = getLocalBounds().toFloat();
    const float w = b.getWidth(), h = b.getHeight();
    const float horizon = h * 0.455f;

    // Upper luminous zone + lower satin brushed deck
    g.setGradientFill(juce::ColourGradient(t.top0, 0, 0, t.top1, 0, horizon, false));
    g.fillRect(0.0f, 0.0f, w, horizon);
    g.setGradientFill(juce::ColourGradient(t.bot0, 0, horizon, t.bot1, 0, h, false));
    g.fillRect(0.0f, horizon, w, h - horizon);

    // Calibrated frequency grid (50 Hz .. 10 kHz) + dB scale (-6 .. -66 dB)
    const float x0 = w * 0.072f, x1 = w * (1.0f - 0.072f);
    const float topPad = 40.0f;
    const float specH = horizon - topPad;
    const float minHz = 30.0f, maxHz = 20000.0f, logSpan = std::log(maxHz / minHz);

    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    for (float db : { -6.0f, -18.0f, -30.0f, -42.0f, -54.0f, -66.0f })
    {
        const float py = horizon - juce::jlimit(0.0f, 1.0f, (db + 72.0f) / 68.0f) * specH;
        g.setColour(t.gridLine.withAlpha(0.50f));
        g.drawHorizontalLine((int)py, x0, x1);
        g.setColour(t.topMuted.withAlpha(0.75f));
        g.drawText(juce::String((int)db), (int)x0 + 3, (int)py - 6, 26, 12, juce::Justification::centredLeft);
    }

    struct FreqTick { float hz; const char* lbl; };
    const FreqTick freqs[] = {
        { 50.0f, "50" }, { 100.0f, "100" }, { 200.0f, "200" }, { 500.0f, "500" },
        { 1000.0f, "1k" }, { 2000.0f, "2k" }, { 5000.0f, "5k" }, { 10000.0f, "10k" }
    };
    for (const auto& ft : freqs)
    {
        const float px = x0 + (x1 - x0) * (std::log(ft.hz / minHz) / logSpan);
        g.setColour(t.gridLine.withAlpha(0.50f));
        g.drawVerticalLine((int)px, topPad + 12.0f, horizon);
        g.setColour(t.topMuted.withAlpha(0.85f));
        g.drawText(ft.lbl, (int)px - 18, (int)topPad - 2, 36, 12, juce::Justification::centred);
    }

    // Live Dual-Layer Real-Time Spectral Analyzer
    auto buildSmoothPath = [&](const float* arr, bool closeBottom) {
        juce::Path p;
        const float dx = (x1 - x0) / (float)(kSpecPts - 1);
        auto ptY = [&](int idx) { return horizon - arr[idx] * (specH - 8.0f); };
        if (closeBottom) { p.startNewSubPath(x0, horizon); p.lineTo(x0, ptY(0)); }
        else             { p.startNewSubPath(x0, ptY(0)); }
        for (int i = 1; i < kSpecPts - 1; ++i)
        {
            const float cx = x0 + dx * (float)i, cy = ptY(i);
            const float nx = x0 + dx * (float)(i + 1), ny = ptY(i + 1);
            p.quadraticTo(cx, cy, 0.5f * (cx + nx), 0.5f * (cy + ny));
        }
        p.lineTo(x1, ptY(kSpecPts - 1));
        if (closeBottom) { p.lineTo(x1, horizon); p.closeSubPath(); }
        return p;
    };

    g.setColour(t.crestDark.withAlpha(0.75f));
    g.fillPath(buildSmoothPath(specPeak_, true));
    g.setGradientFill(juce::ColourGradient(t.crestFill.withAlpha(0.86f), 0.0f, topPad, t.crestFill.withAlpha(0.50f), 0.0f, horizon, false));
    g.fillPath(buildSmoothPath(specSmooth_, true));
    g.setColour(t.crestLine.withAlpha(0.95f));
    g.strokePath(buildSmoothPath(specSmooth_, false), juce::PathStrokeType(1.9f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Horizon rule
    g.setColour(t.horizon);
    g.fillRect(0.0f, horizon - 1.0f, w, 2.0f);

    // Header text
    g.setColour(t.topText);
    g.setFont(juce::FontOptions(19.0f, juce::Font::bold));
    g.drawText("MPCHAIN", 14, 7, 108, 26, juce::Justification::centredLeft);
    g.setColour(t.topMuted);
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.drawText("HQ", hq1Btn_.getX() - 26, 9, 22, 22, juce::Justification::centredRight);

    // Live Loudness Telemetry
    const float outLufsS = proc_.outLufsShortDb.load(std::memory_order_relaxed);
    const float outLufsI = proc_.outLufsIntDb.load(std::memory_order_relaxed);
    const float outPeak  = proc_.outPeakDb.load(std::memory_order_relaxed);
    const float outRms   = proc_.outRmsDb.load(std::memory_order_relaxed);
    const float outPlr   = proc_.outPlrDb.load(std::memory_order_relaxed);
    const float loudVal  = proc_.apvts.getRawParameterValue("loud")->load();
    const float ceilVal  = proc_.apvts.getRawParameterValue("ceiling")->load();

    // Frosted strips behind left & right telemetry
    g.setColour(t.top1.withAlpha(0.62f));
    g.fillRoundedRectangle(x0 + 8.0f, horizon - 54.0f, (float)auto14Btn_.getRight() - (x0 + 8.0f) + 6.0f, 49.0f, 6.0f);
    g.fillRoundedRectangle(w * 0.592f, horizon - 54.0f, x1 - w * 0.592f - 8.0f, 49.0f, 6.0f);

    // Left: SUBTLE | MODERATE | OVERDRIVE loudness bar + LUFS-I
    const auto zoneRect = juce::Rectangle<float>(x0 + 14.0f, horizon - 30.0f, w * 0.155f, 18.0f);
    g.setColour(t.meterWell.withAlpha(0.88f)); g.fillRoundedRectangle(zoneRect, 4.0f);
    g.setColour(t.topText.withAlpha(0.28f));   g.drawRoundedRectangle(zoneRect, 4.0f, 1.0f);
    const float zNorm = juce::jlimit(0.0f, 1.0f, (outLufsS + 32.0f) / 28.0f);
    const juce::Colour zCol = zNorm > 0.78f ? juce::Colour(0xffff3e36)
                            : zNorm > 0.46f ? juce::Colour(0xff5fe68c)
                                            : t.accent;
    g.setColour(zCol.withAlpha(0.90f));
    g.fillRoundedRectangle(zoneRect.reduced(1.5f).withWidth(juce::jmax(0.0f, (zoneRect.getWidth() - 3.0f) * zNorm)), 2.5f);
    const float nx = zoneRect.getX() + 2.0f + (zoneRect.getWidth() - 4.0f) * zNorm;
    g.setColour(juce::Colours::white);
    g.fillRect(nx - 1.0f, zoneRect.getY() + 1.0f, 2.0f, zoneRect.getHeight() - 2.0f);

    g.setColour(t.topText); g.setFont(juce::FontOptions(8.8f, juce::Font::bold));
    g.drawText("SUBTLE   MODERATE   OVERDRIVE", (int)zoneRect.getX() - 2, (int)zoneRect.getY() - 15, (int)zoneRect.getWidth() + 4, 14, juce::Justification::centred);

    const float lufsIX = zoneRect.getRight() + 6.0f;
    lufsIntHitRect_ = juce::Rectangle<float>(lufsIX, horizon - 48.0f, 56.0f, 40.0f);
    g.setColour(t.topMuted); g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.drawText("LUFS-I", (int)lufsIX, (int)horizon - 47, 56, 13, juce::Justification::centred);
    g.setColour(t.topText); g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText(outLufsI > -60.0f ? juce::String(outLufsI, 1) : "--.-", (int)lufsIX, (int)horizon - 31, 56, 20, juce::Justification::centred);

    // Right: LUFS-S | PEAK | RMS | PLR + controllable mini CEIL slider readout
    const float rx0 = w * 0.598f;
    const float ceilLeft = (float)ceilingSlider_.getX() - 8.0f;
    const float rW = (ceilLeft - rx0) / 4.0f;
    auto drawReadout = [&](int slot, const char* lbl, const juce::String& val, juce::Colour valCol) {
        const int sx = (int)(rx0 + rW * (float)slot);
        const int sw = (int)rW;
        g.setColour(t.topMuted); g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.drawText(lbl, sx, (int)horizon - 47, sw, 13, juce::Justification::centred);
        g.setColour(valCol); g.setFont(juce::FontOptions(13.5f, juce::Font::bold));
        g.drawText(val, sx, (int)horizon - 31, sw, 20, juce::Justification::centred);
    };
    drawReadout(0, "LUFS-S", outLufsS > -60.0f ? juce::String(outLufsS, 1) : "--.-", t.topText);
    drawReadout(1, "PEAK",   outPeak  > -60.0f ? juce::String(outPeak,  1) : "--.-", outPeak > (ceilVal - 0.1f) ? t.accentClip : t.topText);
    drawReadout(2, "RMS",    outRms   > -60.0f ? juce::String(outRms,   1) : "--.-", t.topText);
    drawReadout(3, "PLR",    outLufsS > -60.0f ? juce::String(outPlr,   1) : "--.-", t.topText);

    // Mini CEIL slider label & live readout
    g.setColour(loudVal > 9.0f ? t.accentClip : t.topText);
    g.setFont(juce::FontOptions(9.2f, juce::Font::bold));
    const juce::String ceilStr = "CEIL " + juce::String(ceilVal, 1) + (loudVal > 9.0f ? " CLIP" : " dB");
    g.drawText(ceilStr, ceilingSlider_.getX() - 4, ceilingSlider_.getY() - 16, ceilingSlider_.getWidth() + 8, 14, juce::Justification::centred);

    // Lower deck section labels (NO "1.5:1 bus glue" text per request!)
    g.setColour(t.deckMuted); g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText("LINEAR PHASE EQ", eqLowKnob_.getX(), (int)horizon + 12, 160, 16, juce::Justification::centredLeft);

    g.setColour(t.deckText); g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("Low",       eqLowKnob_.getX(),    eqLowKnob_.getY() - 16,    eqLowKnob_.getWidth(),    15, juce::Justification::centred);
    g.drawText("Low Mid",   eqLowMidKnob_.getX(), eqLowMidKnob_.getY() - 16, eqLowMidKnob_.getWidth(), 15, juce::Justification::centred);
    g.drawText("Mid",       eqMidKnob_.getX(),    eqMidKnob_.getY() - 16,    eqMidKnob_.getWidth(),    15, juce::Justification::centred);
    g.drawText("High",      eqHighKnob_.getX(),   eqHighKnob_.getY() - 16,   eqHighKnob_.getWidth(),   15, juce::Justification::centred);
    g.drawText("Glue Comp", glueKnob_.getX() - 8, glueKnob_.getY() - 18,     glueKnob_.getWidth() + 16, 16, juce::Justification::centred);
    g.drawText("LOUDER",    louderKnob_.getX(),   louderKnob_.getBottom() - 2, louderKnob_.getWidth(), 15, juce::Justification::centred);

    // Crossover frequency guide captions under the 4 EQ bands
    g.setColour(t.deckMuted); g.setFont(juce::FontOptions(8.8f, juce::Font::bold));
    g.drawText("30-120 Hz",  eqLowKnob_.getX(),    eqLowKnob_.getBottom() + 1,    eqLowKnob_.getWidth(),    12, juce::Justification::centred);
    g.drawText("120-1.5k",   eqLowMidKnob_.getX(), eqLowMidKnob_.getBottom() + 1, eqLowMidKnob_.getWidth(), 12, juce::Justification::centred);
    g.drawText("1.5k-7k",    eqMidKnob_.getX(),    eqMidKnob_.getBottom() + 1,    eqMidKnob_.getWidth(),    12, juce::Justification::centred);
    g.drawText("> 7 kHz",    eqHighKnob_.getX(),   eqHighKnob_.getBottom() + 1,   eqHighKnob_.getWidth(),   12, juce::Justification::centred);

    g.setColour(t.deckText); g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawText("HPF", hpfKnob_.getX(), hpfKnob_.getY() - 13, hpfKnob_.getWidth(), 13, juce::Justification::centred);

    g.setColour(t.deckMuted); g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawText("WIDTH",     widthXfader_.getX() + 12, widthXfader_.getY() + 1, 56, 15, juce::Justification::centredLeft);
    g.drawText("0% MONO",   widthXfader_.getX() + 12, widthXfader_.getBottom() - 1, 68, 13, juce::Justification::centredLeft);
    g.drawText("100%",      widthXfader_.getBounds().getCentreX() - 24, widthXfader_.getBottom() - 1, 48, 13, juce::Justification::centred);
    g.drawText("200% WIDE", widthXfader_.getRight() - 80, widthXfader_.getBottom() - 1, 68, 13, juce::Justification::centredRight);
    g.drawText("IN",  inGainKnob_.getX(),  inGainKnob_.getY() - 13,  inGainKnob_.getWidth(),  13, juce::Justification::centred);
    g.drawText("OUT", outGainKnob_.getX(), outGainKnob_.getY() - 13, outGainKnob_.getWidth(), 13, juce::Justification::centred);
}

void MPChainAudioProcessorEditor::resized()
{
    const int w = getWidth(), h = getHeight();
    const int horizon = (int)(h * 0.455f);

    // Top bar: Preset dropdown + SAVE/DEL left, HQ 1x/2x 96k/4x 192k/8x 384k centered, Skin RED/RETRO/BLUE right
    presetBox_.setBounds(118, 9, 154, 24);
    savePresetBtn_.setBounds(276, 9, 44, 24);
    delPresetBtn_.setBounds(323, 9, 36, 24);

    const int hqX = w / 2 - 118;
    hq1Btn_.setBounds(hqX, 9, 34, 24);
    hq2Btn_.setBounds(hqX + 38, 9, 58, 24);
    hq4Btn_.setBounds(hqX + 100, 9, 64, 24);
    hq8Btn_.setBounds(hqX + 168, 9, 64, 24);

    skinBlueBtn_.setBounds(w - 68, 9, 56, 24);
    skinRetroBtn_.setBounds(w - 128, 9, 56, 24);
    skinRedBtn_.setBounds(w - 188, 9, 56, 24);

    // Sleeker edge multimeters + small In/Out Gain dials
    const int mW = juce::jlimit(58, 76, (int)(w * 0.068f));
    inMeter_.setBounds(6, 48, mW - 4, h - 136);
    outMeter_.setBounds(w - mW - 2, 48, mW - 4, h - 136);
    inGainKnob_.setBounds(6, h - 76, mW - 4, 66);
    outGainKnob_.setBounds(w - mW - 2, h - 76, mW - 4, 66);

    // Giant Central LOUDER dial straddling the horizon
    const int heroD = juce::jmin(162, (int)(h * 0.305f));
    louderKnob_.setBounds(w / 2 - heroD / 2, horizon - heroD / 2 - 12, heroD, heroD + 22);

    // 4 small clickable/bypassable Saturation buttons directly underneath LOUDER
    const int satW = 48, satGap = 4;
    const int satTotal = satW * 4 + satGap * 3;
    const int satX = w / 2 - satTotal / 2;
    const int satY = louderKnob_.getBottom() + 15;
    satTapeBtn_.setBounds(satX, satY, satW, 20);
    satTubeBtn_.setBounds(satX + (satW + satGap), satY, satW, 20);
    satXfmrBtn_.setBounds(satX + (satW + satGap) * 2, satY, satW, 20);
    satClassABtn_.setBounds(satX + (satW + satGap) * 3, satY, satW, 20);

    // Stacked Auto-LUFS controls left of LOUDER: smaller -9 & -11 pills ABOVE AUTO -14
    const int autoRight = louderKnob_.getX() - 10;
    const int autoW = 104;
    auto14Btn_.setBounds(autoRight - autoW, horizon - 29, autoW, 22);
    auto9Btn_.setBounds (autoRight - autoW, horizon - 51, 50, 19);
    auto11Btn_.setBounds(autoRight - 50,    horizon - 51, 50, 19);

    // Tiny controllable CEIL slider (-14.0 .. 0.0 dB) on upper right telemetry
    const int ceilW = juce::jlimit(78, 112, (int)(w * 0.095f));
    ceilingSlider_.setBounds(w - mW - 18 - ceilW, horizon - 29, ceilW, 18);

    // Bottom Center Width Crossfader + small HPF dial right ABOVE Width
    const int xfW = juce::jmin(300, (int)(w * 0.28f));
    widthXfader_.setBounds(w / 2 - xfW / 2, h - 68, xfW, 50);

    const int hpfW = 58, hpfH = 66;
    const int hpfY = juce::jmax(satY + 34, widthXfader_.getY() - hpfH - 6);
    hpfKnob_.setBounds(w / 2 - hpfW / 2, hpfY, hpfW, hpfH);

    // Lower Left: 4-Band Linear-Phase EQ (Low, Low Mid, Mid, High) + Clean above High
    const int leftAvail = (louderKnob_.getX() - 20) - (mW + 10);
    const int eqGap = 6;
    const int eqW = juce::jmin(82, (leftAvail - eqGap * 3) / 4);
    const int eqH = eqW + 18;
    const int eqY = horizon + 62;
    const int eqStart = mW + 12;
    eqLowKnob_.setBounds(eqStart, eqY, eqW, eqH);
    eqLowMidKnob_.setBounds(eqStart + (eqW + eqGap), eqY, eqW, eqH);
    eqMidKnob_.setBounds(eqStart + (eqW + eqGap) * 2, eqY, eqW, eqH);
    eqHighKnob_.setBounds(eqStart + (eqW + eqGap) * 3, eqY, eqW, eqH);
    cleanBtn_.setBounds(eqHighKnob_.getX(), horizon + 10, eqW, 22);

    // Lower Right: Prominent Glue Comp knob + Air (+1 dB) toggle button on bottom right (Mix scrapped)
    const int rightStart = louderKnob_.getRight() + 26;
    const int rightEnd   = w - mW - 14;
    const int rightMid   = (rightStart + rightEnd) / 2;
    const int glueD = juce::jmin(118, (int)(h * 0.22f));
    glueKnob_.setBounds(rightMid - glueD / 2 - 34, horizon + 54, glueD, glueD + 20);
    airBtn_.setBounds(rightEnd - 108, h - 78, 102, 30);
}
