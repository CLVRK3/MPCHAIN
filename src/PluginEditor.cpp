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
    // Default 2: MPC Gold (Two-Zone Sunlit Gold + Satin Pearl-Silver Console)
    return {
        juce::Colour(0xfff3ead2), juce::Colour(0xffd5b676), juce::Colour(0xff6e541c),
        juce::Colour(0xffb98e3d), juce::Colour(0xfffdfbf4), juce::Colour(0xffd6c39a),
        juce::Colour(0xfff2f2f3), juce::Colour(0xffd8d8dc), juce::Colour(0xffc2b494),
        juce::Colour(0xffb5892a), juce::Colour(0xffd84a22), juce::Colour(0xffffffff),
        juce::Colour(0xff2c2310), juce::Colour(0xff6e5a30), juce::Colour(0xff1d1c19), juce::Colour(0xff64625c),
        juce::Colour(0xff181715), juce::Colour(0xff3c3932), juce::Colour(0xfffafaf8), juce::Colour(0xffb8b6ae),
        juce::Colour(0xffc8c4b8), juce::Colour(0xff1b1915), juce::Colour(0xffe4dcc6), juce::Colour(0xffbaa985),
        juce::Colour(0xff3b2f16), juce::Colour(0xffd63b22)
    };
}

} // namespace

void MPChainLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(3.0f);
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float cx = bounds.getCentreX(), cy = bounds.getCentreY();
    const bool isHero = (bool)slider.getProperties().getWithDefault("hero", false);
    const bool isBipolar = (bool)slider.getProperties().getWithDefault("bipolar", false);
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    if (isHero)
    {
        // Brushed outer metallic bezel
        juce::ColourGradient bez(theme.bezel0, cx, cy - radius, theme.bezel1, cx, cy + radius, false);
        g.setGradientFill(bez);
        g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
        g.setColour(juce::Colours::black.withAlpha(0.28f));
        g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.3f);
    }

    const float arcW = isHero ? 6.5f : (radius > 28.0f ? 4.0f : 2.6f);
    const float arcR = radius - (isHero ? 10.0f : arcW * 0.5f);

    juce::Path track;
    track.addCentredArc(cx, cy, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour(theme.track);
    g.strokePath(track, juce::PathStrokeType(arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (isHero)
    {
        const float clipFrac = 4.5f / 12.0f;
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
        // +4.5 dB clip onset marker
        g.setColour(theme.accentClip);
        g.drawLine(cx + std::sin(clipAng) * (arcR - 5.0f), cy - std::cos(clipAng) * (arcR - 5.0f),
                   cx + std::sin(clipAng) * (arcR + 5.0f), cy - std::cos(clipAng) * (arcR + 5.0f), 2.4f);
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

    const float bodyR = arcR - arcW - 2.2f;
    g.setColour(theme.knobBody);
    g.fillEllipse(cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour(theme.knobLight);
    g.drawEllipse(cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.3f);

    juce::Path ptr;
    ptr.startNewSubPath(cx + std::sin(angle) * bodyR * (isHero ? 0.16f : 0.28f),
                        cy - std::cos(angle) * bodyR * (isHero ? 0.16f : 0.28f));
    ptr.lineTo(cx + std::sin(angle) * (bodyR - 4.0f), cy - std::cos(angle) * (bodyR - 4.0f));
    g.setColour(juce::Colour(0xfffbfaf6));
    g.strokePath(ptr, juce::PathStrokeType(isHero ? 3.2f : 2.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void MPChainLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float, float, juce::Slider::SliderStyle, juce::Slider&)
{
    const auto pill = juce::Rectangle<float>((float)x + 2.0f, (float)y + 4.0f, (float)width - 4.0f, (float)height - 8.0f);
    g.setColour(theme.knobBody);
    g.fillRoundedRectangle(pill, pill.getHeight() * 0.5f);
    g.setColour(theme.knobLight);
    g.drawRoundedRectangle(pill, pill.getHeight() * 0.5f, 1.2f);

    const float left = pill.getX() + 20.0f, right = pill.getRight() - 20.0f;
    const float midX = (left + right) * 0.5f, midY = pill.getCentreY();

    g.setColour(theme.knobLight);
    g.fillRect(left - 7.0f, pill.getY() + 7.0f, 2.5f, pill.getHeight() - 14.0f);
    g.fillRect(right + 4.5f, pill.getY() + 7.0f, 2.5f, pill.getHeight() - 14.0f);
    g.setColour(theme.accent.withAlpha(0.75f));
    g.fillRect(midX - 1.0f, pill.getY() + 6.0f, 2.0f, pill.getHeight() - 12.0f);

    const float capW = 38.0f, capH = pill.getHeight() - 10.0f;
    const float cx = juce::jlimit(left + capW * 0.5f, right - capW * 0.5f, sliderPos);
    g.setColour(theme.accent);
    g.fillRoundedRectangle(juce::jmin(midX, cx), midY - 2.0f, std::fabs(cx - midX), 4.0f, 2.0f);

    const juce::Rectangle<float> cap(cx - capW * 0.5f, midY - capH * 0.5f, capW, capH);
    g.setColour(theme.knobBody.interpolatedWith(theme.accent, 0.25f));
    g.fillRoundedRectangle(cap, capH * 0.5f);
    g.setColour(theme.accent);
    g.drawRoundedRectangle(cap, capH * 0.5f, 2.0f);
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
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
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
    const float barW = 10.0f;
    const auto pkRect = juce::Rectangle<float>(b.getX() + 4.0f, b.getY(), barW, b.getHeight());
    const auto luRect = juce::Rectangle<float>(b.getRight() - barW - 4.0f, b.getY(), barW, b.getHeight());

    auto drawBar = [&](juce::Rectangle<float> r, float db, bool isLufs) {
        g.setColour(theme_.meterWell);
        g.fillRoundedRectangle(r, 3.0f);
        const float norm = juce::jlimit(0.0f, 1.0f, (db + 36.0f) / 36.0f);
        const int segs = 32; const float gap = 1.6f;
        const float sh = (r.getHeight() - gap * (segs + 1)) / (float)segs;
        for (int i = 0; i < segs; ++i)
        {
            const float f = (float)(i + 1) / (float)segs;
            const float sy = r.getBottom() - (float)(i + 1) * (sh + gap);
            const bool lit = f <= norm;
            juce::Colour col = f > 0.90f ? (lit ? juce::Colour(0xffff2a3c) : juce::Colour(0x28ff2a3c))
                             : f > 0.72f ? (lit ? juce::Colour(0xffffba24) : juce::Colour(0x28ffba24))
                             : (lit ? juce::Colour(isLufs ? 0xff85f298 : 0xff6ae86a) : juce::Colour(0x246ae86a));
            g.setColour(col);
            g.fillRoundedRectangle(r.getX() + 1.0f, sy, r.getWidth() - 2.0f, sh, 1.0f);
        }
        const float ty = r.getBottom() - r.getHeight() * (22.0f / 36.0f);
        g.setColour(theme_.targetTick);
        g.fillRect(r.getX() - 1.0f, ty - 0.8f, r.getWidth() + 2.0f, 1.8f);
        if (!isLufs)
        {
            const float hn = juce::jlimit(0.0f, 1.0f, (holdDb_ + 36.0f) / 36.0f);
            if (hn > 0.02f) { g.setColour(holdDb_ > -1.5f ? juce::Colour(0xffff384b) : juce::Colours::white); g.fillRect(r.getX() + 1.0f, r.getBottom() - r.getHeight() * hn, r.getWidth() - 2.0f, 2.0f); }
        }
    };
    drawBar(pkRect, peakDb_, false);
    drawBar(luRect, lufsDb_, true);

    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(theme_.topMuted);
    g.drawText("0", juce::Rectangle<float>(pkRect.getRight(), b.getY(), luRect.getX() - pkRect.getRight(), 14.0f), juce::Justification::centred);
    g.setColour(theme_.targetTick);
    const float ty = b.getBottom() - b.getHeight() * (22.0f / 36.0f);
    g.drawText("-14", juce::Rectangle<float>(pkRect.getRight() - 2.0f, ty - 7.0f, luRect.getX() - pkRect.getRight() + 4.0f, 14.0f), juce::Justification::centred);
    g.setColour(theme_.deckMuted);
    g.drawText("-36", juce::Rectangle<float>(pkRect.getRight(), b.getBottom() - 14.0f, luRect.getX() - pkRect.getRight(), 14.0f), juce::Justification::centred);
}

MPChainAudioProcessorEditor::MPChainAudioProcessorEditor(MPChainAudioProcessor& p)
    : AudioProcessorEditor(&p), proc_(p)
{
    setLookAndFeel(&lnf_);
    setSize(1024, 502);
    setResizable(true, true);
    setResizeLimits(820, 402, 1600, 785);

    configureKnob(loudKnob_,    " dB", false, true);
    configureKnob(eqLowKnob_,   " dB", true);
    configureKnob(eqMidKnob_,   " dB", true);
    configureKnob(eqHighKnob_,  " dB", true);
    configureKnob(vibeKnob_,    "%",   false);
    configureKnob(lowHpfKnob_,  " Hz", false);
    configureKnob(mixKnob_,     "%",   false);
    configureKnob(inGainKnob_,  " dB", true);
    configureKnob(outGainKnob_, " dB", true);

    widthXfader_.setSliderStyle(juce::Slider::LinearHorizontal);
    widthXfader_.setTextBoxStyle(juce::Slider::TextBoxAbove, false, 70, 18);
    widthXfader_.setTextValueSuffix("%");
    addAndMakeVisible(widthXfader_);

    addAndMakeVisible(inMeter_);
    addAndMakeVisible(outMeter_);
    addAndMakeVisible(cleanBtn_);
    addAndMakeVisible(autoBtn_);

    auto setupHq = [&](juce::TextButton& b, int idx) {
        b.onClick = [this, idx] {
            if (auto* param = proc_.apvts.getParameter("hq_os"))
                param->setValueNotifyingHost(param->convertTo0to1((float)idx));
            updateHqButtons(idx);
        };
        addAndMakeVisible(b);
    };
    setupHq(hq1Btn_, 0); setupHq(hq2Btn_, 1); setupHq(hq4Btn_, 2); setupHq(hq8Btn_, 3);

    auto setupSkin = [&](juce::TextButton& b, int idx) {
        b.onClick = [this, idx] {
            if (auto* param = proc_.apvts.getParameter("skin"))
                param->setValueNotifyingHost(param->convertTo0to1((float)idx));
            applySkin(idx);
        };
        addAndMakeVisible(b);
    };
    setupSkin(skinRedBtn_, 0); setupSkin(skinRetroBtn_, 1); setupSkin(skinGoldBtn_, 2);

    for (int i = 0; i < proc_.getNumPrograms(); ++i)
        presetBox_.addItem(proc_.getProgramName(i), i + 1);
    presetBox_.setSelectedId(proc_.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox_.onChange = [this] {
        const int idx = presetBox_.getSelectedId() - 1;
        if (idx >= 0 && idx < proc_.getNumPrograms())
            proc_.setCurrentProgram(idx);
    };
    addAndMakeVisible(presetBox_);

    loudAtt_    = std::make_unique<SliderAtt>(proc_.apvts, "loud",     loudKnob_);
    widthAtt_   = std::make_unique<SliderAtt>(proc_.apvts, "width",    widthXfader_);
    eqLowAtt_   = std::make_unique<SliderAtt>(proc_.apvts, "eq_low",   eqLowKnob_);
    eqMidAtt_   = std::make_unique<SliderAtt>(proc_.apvts, "eq_mid",   eqMidKnob_);
    eqHighAtt_  = std::make_unique<SliderAtt>(proc_.apvts, "eq_high",  eqHighKnob_);
    vibeAtt_    = std::make_unique<SliderAtt>(proc_.apvts, "vibe",     vibeKnob_);
    lowHpfAtt_  = std::make_unique<SliderAtt>(proc_.apvts, "comp_hpf", lowHpfKnob_);
    mixAtt_     = std::make_unique<SliderAtt>(proc_.apvts, "mix",      mixKnob_);
    inGainAtt_  = std::make_unique<SliderAtt>(proc_.apvts, "in_gain",  inGainKnob_);
    outGainAtt_ = std::make_unique<SliderAtt>(proc_.apvts, "out_gain", outGainKnob_);
    cleanAtt_   = std::make_unique<ButtonAtt>(proc_.apvts, "treble_clean", cleanBtn_);
    autoAtt_    = std::make_unique<ButtonAtt>(proc_.apvts, "auto_gain",    autoBtn_);

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
    startTimerHz(45);
}

MPChainAudioProcessorEditor::~MPChainAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void MPChainAudioProcessorEditor::configureKnob(juce::Slider& s, const juce::String& suffix, bool bipolar, bool isHero)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, isHero ? 92 : 66, isHero ? 22 : 17);
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
    for (auto* s : { &loudKnob_, &widthXfader_, &eqLowKnob_, &eqMidKnob_, &eqHighKnob_,
                     &vibeKnob_, &lowHpfKnob_, &mixKnob_, &inGainKnob_, &outGainKnob_ })
        styleSlider(*s);

    auto styleBtn = [&](juce::TextButton& b, bool active) {
        b.setColour(juce::TextButton::buttonColourId, active ? lnf_.theme.accent : lnf_.theme.btnBg);
        b.setColour(juce::TextButton::textColourOffId, active ? lnf_.theme.accentText : lnf_.theme.btnText);
    };
    styleBtn(skinRedBtn_, skinIdx == 0);
    styleBtn(skinRetroBtn_, skinIdx == 1);
    styleBtn(skinGoldBtn_, skinIdx == 2);

    presetBox_.setColour(juce::ComboBox::backgroundColourId, lnf_.theme.btnBg);
    presetBox_.setColour(juce::ComboBox::outlineColourId,    lnf_.theme.btnBorder);
    presetBox_.setColour(juce::ComboBox::textColourId,       lnf_.theme.btnText);
    presetBox_.setColour(juce::ComboBox::arrowColourId,      lnf_.theme.btnText);
    lnf_.setColour(juce::PopupMenu::backgroundColourId,      lnf_.theme.bot0);
    lnf_.setColour(juce::PopupMenu::textColourId,            lnf_.theme.deckText);
    lnf_.setColour(juce::PopupMenu::highlightedBackgroundColourId, lnf_.theme.accent);
    lnf_.setColour(juce::PopupMenu::highlightedTextColourId, lnf_.theme.accentText);

    updateHqButtons(currentHq_ < 0 ? 1 : currentHq_);
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

void MPChainAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    if (lufsIntHitRect_.contains(e.position))
        proc_.requestResetIntegratedLoudness();
}

void MPChainAudioProcessorEditor::updateSpectrum()
{
    proc_.copyRecentAudioForSpectrum(fftRe_, kFftSize);

    // Window + bit-reversal permutation
    float tmp[kFftSize];
    for (int i = 0; i < kFftSize; ++i) tmp[i] = fftRe_[i] * fftWin_[i];
    for (int i = 0; i < kFftSize; ++i)
    {
        fftRe_[fftBitRev_[i]] = tmp[i];
        fftIm_[i] = 0.0f;
    }

    // In-place radix-2 Cooley-Tukey FFT
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
    const float minHz = 30.0f, maxHz = 20000.0f;
    const float logRatio = std::log(maxHz / minHz);
    const float invNorm = 2.0f / (float)kFftSize; // coherent Hann compensation (~4/N, scaled smoothly)

    float raw[kSpecPts];
    for (int p = 0; p < kSpecPts; ++p)
    {
        const float t0 = ((float)p - 0.5f) / (float)(kSpecPts - 1);
        const float t1 = ((float)p + 0.5f) / (float)(kSpecPts - 1);
        const float fC = minHz * std::exp(logRatio * ((float)p / (float)(kSpecPts - 1)));
        const float fLo = minHz * std::exp(logRatio * juce::jmax(0.0f, t0));
        const float fHi = minHz * std::exp(logRatio * juce::jmin(1.0f, t1));

        const float binF = fC * (float)kFftSize / sr;
        const int bLo = juce::jlimit(1, kFftSize / 2 - 1, (int)std::floor(fLo * (float)kFftSize / sr));
        const int bHi = juce::jlimit(bLo, kFftSize / 2 - 1, (int)std::ceil(fHi * (float)kFftSize / sr));

        float magSq = 0.0f;
        if (bHi == bLo)
        {
            const int b0 = juce::jlimit(1, kFftSize / 2 - 2, (int)binF);
            const float fr = binF - (float)b0;
            const float m0 = fftRe_[b0] * fftRe_[b0] + fftIm_[b0] * fftIm_[b0];
            const float m1 = fftRe_[b0 + 1] * fftRe_[b0 + 1] + fftIm_[b0 + 1] * fftIm_[b0 + 1];
            magSq = m0 + (m1 - m0) * fr;
        }
        else
        {
            float sum = 0.0f;
            for (int b = bLo; b <= bHi; ++b)
                sum += fftRe_[b] * fftRe_[b] + fftIm_[b] * fftIm_[b];
            magSq = sum / (float)(bHi - bLo + 1);
        }

        const float mag = std::sqrt(magSq) * invNorm * 2.0f;
        // +3 dB/oct mastering tilt centered at 500 Hz so pink-balanced mixes sit level
        const float tiltDb = 3.0f * std::log2(fC / 500.0f);
        const float db = (mag > 1.0e-6f ? 20.0f * std::log10(mag) : -96.0f) + tiltDb;
        raw[p] = juce::jlimit(0.0f, 1.0f, (db + 72.0f) / 68.0f);
    }

    // Gentle spatial smoothing + dual-layer ballistics (~240 ms front release, slow peak-hold back ridge)
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
    const float horizon = h * 0.472f;

    // Upper luminous zone + lower satin brushed deck
    g.setGradientFill(juce::ColourGradient(t.top0, 0, 0, t.top1, 0, horizon, false));
    g.fillRect(0.0f, 0.0f, w, horizon);
    g.setGradientFill(juce::ColourGradient(t.bot0, 0, horizon, t.bot1, 0, h, false));
    g.fillRect(0.0f, horizon, w, h - horizon);

    // Calibrated frequency grid (50 Hz .. 10 kHz) + dB scale (-6 .. -66 dB) in upper zone
    const float x0 = w * 0.088f, x1 = w * (1.0f - 0.088f);
    const float topPad = 42.0f;
    const float specH = horizon - topPad;
    const float minHz = 30.0f, maxHz = 20000.0f, logSpan = std::log(maxHz / minHz);

    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    const float dbs[] = { -6.0f, -18.0f, -30.0f, -42.0f, -54.0f, -66.0f };
    for (float db : dbs)
    {
        const float yn = juce::jlimit(0.0f, 1.0f, (db + 72.0f) / 68.0f);
        const float py = horizon - yn * specH;
        g.setColour(t.gridLine.withAlpha(0.55f));
        g.drawHorizontalLine((int)py, x0, x1);
        g.setColour(t.topMuted.withAlpha(0.75f));
        g.drawText(juce::String((int)db), (int)x0 + 3, (int)py - 7, 26, 13, juce::Justification::centredLeft);
    }

    struct FreqTick { float hz; const char* lbl; };
    const FreqTick freqs[] = {
        { 50.0f, "50" }, { 100.0f, "100" }, { 200.0f, "200" }, { 500.0f, "500" },
        { 1000.0f, "1k" }, { 2000.0f, "2k" }, { 5000.0f, "5k" }, { 10000.0f, "10k" }
    };
    for (const auto& ft : freqs)
    {
        const float px = x0 + (x1 - x0) * (std::log(ft.hz / minHz) / logSpan);
        g.setColour(t.gridLine.withAlpha(0.55f));
        g.drawVerticalLine((int)px, topPad + 12.0f, horizon);
        g.setColour(t.topMuted.withAlpha(0.85f));
        g.drawText(ft.lbl, (int)px - 18, (int)topPad - 2, 36, 13, juce::Justification::centred);
    }

    // Live Dual-Layer Real-Time Spectral Analyzer (Peak-Hold Back Ridge + Smoothed Live Crest)
    auto buildSmoothPath = [&](const float* arr, bool closeBottom) {
        juce::Path p;
        const float dx = (x1 - x0) / (float)(kSpecPts - 1);
        auto ptY = [&](int idx) { return horizon - arr[idx] * (specH - 8.0f); };
        if (closeBottom)
        {
            p.startNewSubPath(x0, horizon);
            p.lineTo(x0, ptY(0));
        }
        else
        {
            p.startNewSubPath(x0, ptY(0));
        }
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

    const juce::Path backRidge   = buildSmoothPath(specPeak_,   true);
    const juce::Path frontCrest  = buildSmoothPath(specSmooth_, true);
    const juce::Path crestStroke = buildSmoothPath(specSmooth_, false);

    g.setColour(t.crestDark.withAlpha(0.72f));
    g.fillPath(backRidge);
    g.setGradientFill(juce::ColourGradient(t.crestFill.withAlpha(0.86f), 0.0f, topPad, t.crestFill.withAlpha(0.58f), 0.0f, horizon, false));
    g.fillPath(frontCrest);
    g.setColour(t.crestLine.withAlpha(0.95f));
    g.strokePath(crestStroke, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Horizon rule
    g.setColour(t.horizon);
    g.fillRect(0.0f, horizon - 1.0f, w, 2.0f);

    // Header text
    g.setColour(t.topText);
    g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
    g.drawText("MPCHAIN", 16, 8, 118, 26, juce::Justification::centredLeft);
    g.setColour(t.topMuted);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("HQ", (int)(w * 0.5f - 134), 10, 26, 22, juce::Justification::centredRight);

    // Live Loudness Telemetry Bar (Left: SAFE / OPTIMAL / CRUSH + LUFS-I; Right: LUFS-S / PEAK / RMS / PLR)
    const float outLufsS = proc_.outLufsShortDb.load(std::memory_order_relaxed);
    const float outLufsI = proc_.outLufsIntDb.load(std::memory_order_relaxed);
    const float outPeak  = proc_.outPeakDb.load(std::memory_order_relaxed);
    const float outRms   = proc_.outRmsDb.load(std::memory_order_relaxed);
    const float outPlr   = proc_.outPlrDb.load(std::memory_order_relaxed);
    const float loudVal  = proc_.apvts.getRawParameterValue("loud")->load();

    // Subtle frosted strip behind telemetry readouts for legibility over live spectrum
    g.setColour(t.top1.withAlpha(0.45f));
    g.fillRoundedRectangle(x0 + 10.0f, horizon - 49.0f, w * 0.33f - 10.0f, 44.0f, 6.0f);
    g.fillRoundedRectangle(w * 0.60f,  horizon - 49.0f, x1 - w * 0.60f - 10.0f, 44.0f, 6.0f);

    // Left: SAFE / OPTIMAL / CRUSH loudness bar + needle
    const auto zoneRect = juce::Rectangle<float>(x0 + 14.0f, horizon - 31.0f, w * 0.145f, 18.0f);
    g.setColour(t.meterWell.withAlpha(0.82f)); g.fillRoundedRectangle(zoneRect, 4.0f);
    g.setColour(t.topText.withAlpha(0.30f));   g.drawRoundedRectangle(zoneRect, 4.0f, 1.0f);
    const float zNorm = juce::jlimit(0.0f, 1.0f, (outLufsS + 32.0f) / 28.0f); // -32 .. -4 LUFS
    const juce::Colour zCol = zNorm > 0.78f ? juce::Colour(0xffff3e36)
                            : zNorm > 0.46f ? juce::Colour(0xff7ae080)
                                            : juce::Colour(0xffd4b25c);
    g.setColour(zCol.withAlpha(0.88f));
    g.fillRoundedRectangle(zoneRect.reduced(1.5f).withWidth(juce::jmax(0.0f, (zoneRect.getWidth() - 3.0f) * zNorm)), 2.5f);
    // Needle marker at current loudness
    const float nx = zoneRect.getX() + 2.0f + (zoneRect.getWidth() - 4.0f) * zNorm;
    g.setColour(juce::Colours::white);
    g.fillRect(nx - 1.0f, zoneRect.getY() + 1.0f, 2.0f, zoneRect.getHeight() - 2.0f);

    g.setColour(t.topText); g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawText("SAFE     OPTIMAL     CRUSH", (int)zoneRect.getX(), (int)zoneRect.getY() - 15, (int)zoneRect.getWidth(), 14, juce::Justification::centred);

    // Left-center: LUFS-I (click to reset)
    const float lufsIX = zoneRect.getRight() + 8.0f;
    lufsIntHitRect_ = juce::Rectangle<float>(lufsIX, horizon - 46.0f, 62.0f, 38.0f);
    g.setColour(t.topMuted); g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawText("LUFS-I", (int)lufsIX, (int)horizon - 45, 62, 13, juce::Justification::centred);
    g.setColour(t.topText); g.setFont(juce::FontOptions(14.5f, juce::Font::bold));
    g.drawText(outLufsI > -60.0f ? juce::String(outLufsI, 1) : "--.-", (int)lufsIX, (int)horizon - 30, 62, 20, juce::Justification::centred);

    // Right: LUFS-S | PEAK | RMS | PLR | CEIL/CLIP badge
    const float rx0 = w * 0.606f;
    const float rW  = (x1 - 14.0f - rx0) / 5.0f;
    auto drawReadout = [&](int slot, const char* lbl, const juce::String& val, juce::Colour valCol) {
        const int sx = (int)(rx0 + rW * (float)slot);
        const int sw = (int)rW;
        g.setColour(t.topMuted); g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.drawText(lbl, sx, (int)horizon - 45, sw, 13, juce::Justification::centred);
        g.setColour(valCol); g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText(val, sx, (int)horizon - 30, sw, 20, juce::Justification::centred);
    };
    drawReadout(0, "LUFS-S", outLufsS > -60.0f ? juce::String(outLufsS, 1) : "--.-", t.topText);
    drawReadout(1, "PEAK",   outPeak  > -60.0f ? juce::String(outPeak,  1) : "--.-", outPeak > -1.6f ? t.accentClip : t.topText);
    drawReadout(2, "RMS",    outRms   > -60.0f ? juce::String(outRms,   1) : "--.-", t.topText);
    drawReadout(3, "PLR",    outLufsS > -60.0f ? juce::String(outPlr,   1) : "--.-", t.topText);
    drawReadout(4, "-1.5 CEIL", loudVal > 4.5f ? "CLIP" : "LIMIT", loudVal > 4.5f ? t.accentClip : t.topText);

    // Lower deck section labels
    g.setColour(t.deckMuted); g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.drawText("LINEAR PHASE EQ", eqLowKnob_.getX(), (int)horizon + 12, 160, 16, juce::Justification::centredLeft);
    g.drawText("1.5:1 BUS GLUE  |  LOW SAFE HPF", vibeKnob_.getX(), (int)horizon + 12, 240, 16, juce::Justification::centredLeft);

    g.setColour(t.deckText); g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    g.drawText("Low",     eqLowKnob_.getX(),  eqLowKnob_.getY() - 16,  eqLowKnob_.getWidth(),  16, juce::Justification::centred);
    g.drawText("Mid",     eqMidKnob_.getX(),  eqMidKnob_.getY() - 16,  eqMidKnob_.getWidth(),  16, juce::Justification::centred);
    g.drawText("High",    eqHighKnob_.getX(), eqHighKnob_.getY() - 16, eqHighKnob_.getWidth(), 16, juce::Justification::centred);
    g.drawText("Vibe",    vibeKnob_.getX(),   vibeKnob_.getY() - 16,   vibeKnob_.getWidth(),   16, juce::Justification::centred);
    g.drawText("Low HPF", lowHpfKnob_.getX(), lowHpfKnob_.getY() - 16, lowHpfKnob_.getWidth(), 16, juce::Justification::centred);
    g.drawText("Mix",     mixKnob_.getX(),    mixKnob_.getY() - 16,    mixKnob_.getWidth(),    16, juce::Justification::centred);
    g.drawText("LOUD",    loudKnob_.getX(),   loudKnob_.getBottom() - 2, loudKnob_.getWidth(), 16, juce::Justification::centred);

    g.setColour(t.deckMuted); g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText("WIDTH", widthXfader_.getX() + 14, widthXfader_.getY() + 1, 60, 16, juce::Justification::centredLeft);
    g.drawText("0% MONO",   widthXfader_.getX() + 14, widthXfader_.getBottom() - 2, 70, 14, juce::Justification::centredLeft);
    g.drawText("100%",      widthXfader_.getBounds().getCentreX() - 25, widthXfader_.getBottom() - 2, 50, 14, juce::Justification::centred);
    g.drawText("200% WIDE", widthXfader_.getRight() - 84, widthXfader_.getBottom() - 2, 70, 14, juce::Justification::centredRight);
    g.drawText("IN",  inGainKnob_.getX(),  inGainKnob_.getY() - 13,  inGainKnob_.getWidth(),  13, juce::Justification::centred);
    g.drawText("OUT", outGainKnob_.getX(), outGainKnob_.getY() - 13, outGainKnob_.getWidth(), 13, juce::Justification::centred);
}

void MPChainAudioProcessorEditor::resized()
{
    const int w = getWidth(), h = getHeight();
    const int horizon = (int)(h * 0.472f);

    // Top bar: Preset dropdown left, HQ 1x/2x/4x/8x centered, Skin RED/RETRO/GOLD right
    presetBox_.setBounds(132, 9, 158, 24);
    const int hqX = w / 2 - 100;
    hq1Btn_.setBounds(hqX, 9, 44, 24); hq2Btn_.setBounds(hqX + 48, 9, 44, 24);
    hq4Btn_.setBounds(hqX + 96, 9, 44, 24); hq8Btn_.setBounds(hqX + 144, 9, 44, 24);

    skinGoldBtn_.setBounds(w - 72, 9, 58, 24);
    skinRetroBtn_.setBounds(w - 134, 9, 58, 24);
    skinRedBtn_.setBounds(w - 196, 9, 58, 24);

    const int mW = (int)(w * 0.086f);
    inMeter_.setBounds(8, 52, mW - 8, h - 146);
    outMeter_.setBounds(w - mW, 52, mW - 8, h - 146);
    inGainKnob_.setBounds(10, h - 78, mW - 12, 68);
    outGainKnob_.setBounds(w - mW + 2, h - 78, mW - 12, 68);

    // Giant Central LOUD dial straddling the horizon
    const int heroD = juce::jmin(174, (int)(h * 0.355f));
    loudKnob_.setBounds(w / 2 - heroD / 2, horizon - heroD / 2 - 10, heroD, heroD + 24);

    // Auto -14 pill neatly between LUFS-I and LOUD bezel
    autoBtn_.setBounds(loudKnob_.getX() - 88, horizon - 38, 80, 24);

    // Bottom Center Width Crossfader
    const int xfW = juce::jmin(320, (int)(w * 0.31f));
    widthXfader_.setBounds(w / 2 - xfW / 2, h - 78, xfW, 58);

    // Left Trio: Low, Mid, High (+ Clean pill above High)
    const int kW = juce::jmin(94, (int)(w * 0.092f));
    const int kH = kW + 20;
    const int deckY = horizon + 54;
    const int leftStart = mW + 14;
    eqLowKnob_.setBounds(leftStart, deckY, kW, kH);
    eqMidKnob_.setBounds(leftStart + kW + 6, deckY, kW, kH);
    eqHighKnob_.setBounds(leftStart + (kW + 6) * 2, deckY, kW, kH);
    cleanBtn_.setBounds(eqHighKnob_.getX() + 4, horizon + 10, kW - 8, 24);

    // Right Trio: Vibe, Low HPF, Mix
    const int rightStart = w - mW - 14 - (kW * 3 + 12);
    vibeKnob_.setBounds(rightStart, deckY, kW, kH);
    lowHpfKnob_.setBounds(rightStart + kW + 6, deckY, kW, kH);
    mixKnob_.setBounds(rightStart + (kW + 6) * 2, deckY, kW, kH);
}
