#include "PluginEditor.h"

using namespace vintage;

//==============================================================================
StimmeAudioProcessorEditor::StimmeAudioProcessorEditor (StimmeAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    addKnob ("comp",     "KOMPRESSION", true);
    addKnob ("out",      "AUSGANG (DB)", true, juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x92" "12|\xe2\x88\x92" "6|0|+6|+12")));
    addKnob ("hpf",      "FREQUENZ (HZ)", false, "40|70|120|200|300");
    addKnob ("dsfreq",   "FREQUENZ (KHZ)", false, "3|4|6|8|10");
    addKnob ("dsamount", juce::String (juce::CharPointer_UTF8 ("ST\xc3\x84RKE")), false);
    addKnob ("warmth",   juce::String (juce::CharPointer_UTF8 ("W\xc3\x84RME")), false, juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x92" "6|\xe2\x88\x92" "3|0|+3|+6")));
    addKnob ("presence", juce::String (juce::CharPointer_UTF8 ("PR\xc3\x84SENZ")), false, juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x92" "8|\xe2\x88\x92" "4|0|+4|+8")));
    addKnob ("air",      "LUFT", false, juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x92" "8|\xe2\x88\x92" "4|0|+4|+8")));

    setupSwitch (listenSwitch, "listen", "ZISCHEN", "NORMAL", lampAmber);
    setupSwitch (tubeSwitch, "tube", "AN", "AUS", lampRed);
    dsMeter.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (dsMeter);

    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    addAndMakeVisible (vu);

    rack::styleToggle (listenSwitch);
    rack::styleToggle (tubeSwitch);
    setSize (rackW, rackH);
    startTimerHz (60);
}

StimmeAudioProcessorEditor::~StimmeAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

StimmeAudioProcessorEditor::Knob& StimmeAudioProcessorEditor::addKnob (const juce::String& paramID, const juce::String& name, bool big, const juce::String& steps)
{
    auto k = std::make_unique<Knob>();
    k->name = name;
    k->big = big;
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    k->slider.setMouseDragSensitivity (big ? 320 : (steps.isNotEmpty() ? 140 : 220));
    if (steps.isNotEmpty())
        k->slider.getProperties().set ("stepLabels", steps);
    addAndMakeVisible (k->slider);
    k->attachment = std::make_unique<SliderAttachment> (proc.apvts, paramID, k->slider);
    if (auto* rp = proc.apvts.getParameter (paramID))
        k->slider.setDoubleClickReturnValue (true, rp->convertFrom0to1 (rp->getDefaultValue()));
    knobs.push_back (std::move (k));
    return *knobs.back();
}

void StimmeAudioProcessorEditor::setupSwitch (juce::ToggleButton& b, const juce::String& paramID,
                                            const juce::String& onText, const juce::String& offText, juce::Colour lamp)
{
    b.setButtonText ("x");
    b.getProperties().set ("onText", onText);
    b.getProperties().set ("offText", offText);
    b.getProperties().set ("lamp", (int) lamp.getARGB());
    addAndMakeVisible (b);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (proc.apvts, paramID, b));
}

void StimmeAudioProcessorEditor::timerCallback()
{
    constexpr float dt = 1.0f / 60.0f;
    // Pegelreduktion des Kompressors: ohne Kompression steht der Zeiger auf 0 VU
    vu.setRestAtZeroVu (true);
    vu.update (juce::Decibels::decibelsToGain (-proc.compGrDb.load() - 18.0f) / 1.1107f, 0.0f, dt);
    proc.peakOut.store (0.0f);

    const float ds = proc.deEssGrDb.load();
    const float shown = ds > dsMeter.db ? ds : dsMeter.db * 0.9f;
    if (std::abs (shown - dsMeter.db) > 0.05f) { dsMeter.db = shown; dsMeter.repaint(); }

    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1)
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
}

//==============================================================================
void StimmeAudioProcessorEditor::renderPanel()
{
    panelImage = rack::renderPanel (rackW, rackH, 23);
    juce::Graphics g (panelImage);
    const juce::String titles[] = { "KOMPRESSOR", "PEGELREDUKTION", "AUSGANG", "TRITTSCHALL", "DE-ESSER", "KLANG" };
    for (size_t i = 0; i < sections.size(); ++i)
        rack::drawSection (g, sections[i], titles[i], lnf);
    rack::drawBezel (g, vuBezel);
    rack::drawBezel (g, ledBezel, 6.0f);
    rack::drawLabel (g, "DB", ledBezel.toFloat().withY ((float) ledBezel.getBottom() + 4.0f).withHeight (14.0f), lnf, 10.0f);
    for (auto& k : knobs)
        rack::drawKnobLabel (g, k->slider, k->name, lnf, k->big ? 14.0f : 12.0f);
    const juce::String names[] = { juce::String (juce::CharPointer_UTF8 ("DE-ESSER ABH\xc3\x96REN")), juce::String (juce::CharPointer_UTF8 ("R\xc3\x96HRENW\xc3\x84RME")) };
    for (size_t i = 0; i < 2; ++i)
        rack::drawLabel (g, names[i], switchLabels[i].toFloat(), lnf, 11.0f, juce::Justification::centredLeft);
    rack::drawNameplate (g, nameplate, "STIMME", "GESANGSKANAL", lnf);
    rack::drawLabel (g, "PROGRAMM", presetBox.getBounds().toFloat().withY ((float) presetBox.getY() - 16.0f).withHeight (14.0f), lnf, 11.0f, juce::Justification::centredLeft);
    rack::drawTypeTag (g, rack::u8 ("TYP KB-57  \xc2\xb7  NR. 0621"), tagArea, lnf);
}

void StimmeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (panelImage, 0, 0);
}

void StimmeAudioProcessorEditor::resized()
{
    const int top = 30, bottom = rackH - 34;
    sections[0] = { 72, top, 240, 262 };
    sections[1] = { 324, top, 460, 262 };
    sections[2] = { 796, top, 234, 262 };
    const int ry = top + 282, rh = bottom - ry;
    sections[3] = { 72, ry, 170, rh };
    sections[4] = { 254, ry, 420, rh };
    sections[5] = { 686, ry, 344, rh };

    knobs[0]->slider.setBounds (juce::Rectangle<int> (220, 220).withCentre ({ sections[0].getCentreX(), top + 132 }));
    knobs[1]->slider.setBounds (juce::Rectangle<int> (220, 220).withCentre ({ sections[2].getCentreX(), top + 132 }));
    vuBezel = juce::Rectangle<int> (380, 236).withCentre ({ sections[1].getCentreX(), top + 134 });
    vu.setBounds (vuBezel.reduced (10));

    constexpr int kw = 128;
    knobs[2]->slider.setBounds (juce::Rectangle<int> (kw, kw).withCentre ({ sections[3].getCentreX(), ry + 18 + kw / 2 }));
    knobs[3]->slider.setBounds (sections[4].getX() + 14, ry + 18, kw, kw);
    knobs[4]->slider.setBounds (sections[4].getX() + 14 + kw + 8, ry + 18, kw, kw);
    ledBezel = { sections[4].getRight() - 110, ry + 22, 90, rh - 58 };
    dsMeter.setBounds (ledBezel.reduced (8));
    const int tw = sections[5].getWidth() / 3;
    for (int i = 0; i < 3; ++i)
        knobs[(size_t) (5 + i)]->slider.setBounds (juce::Rectangle<int> (112, 112).withCentre ({ sections[5].getX() + tw * i + tw / 2, ry + 18 + 64 }));

    const int colX = 1046, colW = rackW - 56 - 16 - colX;
    nameplate = { colX, top - 8, colW, 88 };
    presetBox.setBounds (colX, top + 112, colW, 28);
    juce::ToggleButton* sws[] = { &listenSwitch, &tubeSwitch };
    for (int i = 0; i < 2; ++i)
    {
        const int y = top + 168 + i * 84;
        switchLabels[(size_t) i] = { colX + 4, y, colW, 14 };
        sws[i]->setBounds (colX + 4, y + 18, 150, 40);
    }
    tagArea = { colX - 60, rackH - 28, colW + 60, 16 };
    renderPanel();
}

//==============================================================================
void StimmeAudioProcessorEditor::LedMeter::paint (juce::Graphics& g)
{
    // von unten nach oben: 1, 2, 3, 4, 6, 8, 10, 12 dB Absenkung
    static constexpr float steps[] = { 1, 2, 3, 4, 6, 8, 10, 12 };
    const auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0d0f11));
    g.fillRoundedRectangle (b, 3.0f);
    const float h = (b.getHeight() - 6.0f) / 8.0f;
    for (int i = 0; i < 8; ++i)
    {
        const bool on = db >= steps[i] - 0.25f;
        const juce::Colour col = i < 5 ? juce::Colour (0xffffb23c) : juce::Colour (0xffff4a32);
        const auto r = juce::Rectangle<float> (b.getX() + 6.0f, b.getBottom() - 3.0f - (float) (i + 1) * h + 2.0f, 20.0f, h - 4.0f);
        if (on)
        {
            g.setColour (col.withAlpha (0.35f));
            g.fillRoundedRectangle (r.expanded (3.0f), 4.0f);
        }
        g.setColour (on ? col : col.withAlpha (0.12f));
        g.fillRoundedRectangle (r, 2.0f);
        g.setColour (juce::Colours::white.withAlpha (on ? 0.85f : 0.35f));
        g.setFont (juce::FontOptions (10.0f));
        g.drawText (juce::String ((int) steps[i]), juce::Rectangle<float> (r.getRight() + 6.0f, r.getY() - 2.0f, 30.0f, r.getHeight() + 4.0f), juce::Justification::centredLeft);
    }
}
