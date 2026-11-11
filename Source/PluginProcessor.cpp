#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ScaledEditor.h"

namespace
{
using APF = juce::AudioParameterFloat;
using Attr = juce::AudioParameterFloatAttributes;
juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }
Attr withText (std::function<juce::String (float)> f)
{
    return Attr().withStringFromValueFunction ([f] (float v, int) { return f (v); })
                 .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); });
}
Attr db() { return withText ([] (float v) { return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; }); }
Attr hz() { return withText ([] (float v) { return v >= 1000.0f ? juce::String (v / 1000.0f, 1) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz"; }); }
Attr s10() { return withText ([] (float v) { return juce::String (v, 1); }); }
}

juce::AudioProcessorValueTreeState::ParameterLayout StimmeAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<APF> (juce::ParameterID { "hpf", 1 }, "Trittschallfilter", juce::NormalisableRange<float> (40.0f, 300.0f, 1.0f, 0.5f), 80.0f, hz()));
    l.add (std::make_unique<APF> (juce::ParameterID { "dsfreq", 1 }, "De-Esser Frequenz", juce::NormalisableRange<float> (3000.0f, 10000.0f, 10.0f, 0.6f), 6000.0f, hz()));
    l.add (std::make_unique<APF> (juce::ParameterID { "dsamount", 1 }, u8 ("De-Esser St\xc3\xa4rke"), juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 3.0f, s10()));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "listen", 1 }, u8 ("De-Esser abh\xc3\xb6ren"), false));
    l.add (std::make_unique<APF> (juce::ParameterID { "comp", 1 }, "Kompression", juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 3.0f, s10()));
    l.add (std::make_unique<APF> (juce::ParameterID { "warmth", 1 }, u8 ("W\xc3\xa4rme"), juce::NormalisableRange<float> (-6.0f, 6.0f, 0.1f), 0.0f, db()));
    l.add (std::make_unique<APF> (juce::ParameterID { "presence", 1 }, u8 ("Pr\xc3\xa4senz"), juce::NormalisableRange<float> (-8.0f, 8.0f, 0.1f), 0.0f, db()));
    l.add (std::make_unique<APF> (juce::ParameterID { "air", 1 }, "Luft", juce::NormalisableRange<float> (-8.0f, 8.0f, 0.1f), 0.0f, db()));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "tube", 1 }, u8 ("R\xc3\xb6hre"), false));
    l.add (std::make_unique<APF> (juce::ParameterID { "out", 1 }, "Ausgang", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, db()));
    return l;
}

const std::vector<StimmeAudioProcessor::Preset>& StimmeAudioProcessor::presets()
{
    static const std::vector<Preset> p = {
        { "Standard",                            { { "hpf", 80 },  { "dsfreq", 6000 }, { "dsamount", 3 },   { "comp", 3 },   { "warmth", 0 },    { "presence", 0 },  { "air", 0 },  { "tube", 0 }, { "out", 0 } } },
        { "Soul-Gesang \xe2\x80\x93 warm",       { { "hpf", 90 },  { "dsfreq", 6500 }, { "dsamount", 4 },   { "comp", 5 },   { "warmth", 2 },    { "presence", 1.5f },{ "air", 2 }, { "tube", 1 }, { "out", -1.0f } } },
        { "Blues \xe2\x80\x93 rau mit Biss",     { { "hpf", 110 }, { "dsfreq", 6000 }, { "dsamount", 3 },   { "comp", 6 },   { "warmth", 1 },    { "presence", 3.5f },{ "air", 0 }, { "tube", 1 }, { "out", -1.0f } } },
        { "Ballade \xe2\x80\x93 nah & offen",    { { "hpf", 80 },  { "dsfreq", 7000 }, { "dsamount", 5 },   { "comp", 5 },   { "warmth", 1.5f }, { "presence", 0.5f },{ "air", 4 }, { "tube", 0 }, { "out", -1.0f } } },
        { "Rock \xe2\x80\x93 nach vorne",        { { "hpf", 120 }, { "dsfreq", 6000 }, { "dsamount", 5 },   { "comp", 7 },   { "warmth", -1 },   { "presence", 4 },  { "air", 3 },  { "tube", 1 }, { "out", 0 } } },
        { "Background-Chor",                     { { "hpf", 200 }, { "dsfreq", 6500 }, { "dsamount", 6 },   { "comp", 6 },   { "warmth", -2 },   { "presence", -2 }, { "air", 2 },  { "tube", 0 }, { "out", 1.0f } } },
        { "Scharfe Stimme z\xc3\xa4hmen",        { { "hpf", 90 },  { "dsfreq", 5500 }, { "dsamount", 8 },   { "comp", 4 },   { "warmth", 1.5f }, { "presence", -2.5f },{ "air", -1 },{ "tube", 1 }, { "out", 0 } } },
        { "Sprache / Ansage",                    { { "hpf", 100 }, { "dsfreq", 6000 }, { "dsamount", 6 },   { "comp", 6.5f },{ "warmth", 1 },    { "presence", 2 },  { "air", 1 },  { "tube", 0 }, { "out", 0 } } },
        { "Mundharmonika",                       { { "hpf", 150 }, { "dsfreq", 8000 }, { "dsamount", 0 },   { "comp", 5 },   { "warmth", 3 },    { "presence", 2 },  { "air", -3 }, { "tube", 1 }, { "out", -1.5f } } },
    };
    return p;
}

void StimmeAudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets().size()))
        return;
    currentProgram = index;
    for (auto& [id, value] : presets()[(size_t) index].values)
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
}

const juce::String StimmeAudioProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, (int) presets().size())
               ? juce::String (juce::CharPointer_UTF8 (presets()[(size_t) index].name)) : juce::String();
}

StimmeAudioProcessor::StimmeAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KlangbildStimme", createLayout())
{
}

bool StimmeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && in == out;
}

void StimmeAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    deEsser.prepare (sr);
    comp.prepare (sr);
    for (auto& ch : st) for (auto& s : ch) s.reset();
    lastFilter.fill (-1.0e9f);
    for (auto* s : { &smMakeup, &smOut, &smTube }) s->reset (sr, 0.03);
    smMakeup.setCurrentAndTargetValue (1.0f);
    smOut.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("out")->load()));
    smTube.setCurrentAndTargetValue (apvts.getRawParameterValue ("tube")->load());
    vuCoef = (float) (1.0 - std::exp (-1.0 / (0.01 * sr)));
    vuState = 0;
}

void StimmeAudioProcessor::updateFilters()
{
    const std::array<float, 6> now { apvts.getRawParameterValue ("hpf")->load(), apvts.getRawParameterValue ("dsfreq")->load(),
                                     apvts.getRawParameterValue ("warmth")->load(), apvts.getRawParameterValue ("presence")->load(),
                                     apvts.getRawParameterValue ("air")->load(), (float) sr };
    if (now == lastFilter)
        return;
    lastFilter = now;
    c[hp1]      = eq::makeHighPass (sr, now[0], 0.70710678);
    c[hp2]      = eq::makeHighPass (sr, now[0] * 0.6, 0.55);       // weicher Übergang, zusammen ~12 dB/Okt
    c[warmth]   = eq::makePeak (sr, 250.0, 0.8, now[2]);
    c[presence] = eq::makePeak (sr, 4500.0, 0.9, now[3]);
    c[air]      = eq::makeHighShelf (sr, 12000.0, 0.70710678, now[4]);
    deEsser.setFrequency (now[1]);
}

void StimmeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = juce::jmin (getTotalNumInputChannels(), buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    if (numCh == 0 || n == 0)
        return;

    updateFilters();
    const float amount = apvts.getRawParameterValue ("comp")->load();
    const float dsAmount = apvts.getRawParameterValue ("dsamount")->load();
    const bool listen = apvts.getRawParameterValue ("listen")->load() > 0.5f;
    comp.setAmount (amount);
    smMakeup.setTargetValue (juce::Decibels::decibelsToGain (juce::jmax (0.0f, amount - 2.5f) * 0.8f));
    smOut.setTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("out")->load()));
    smTube.setTargetValue (apvts.getRawParameterValue ("tube")->load() > 0.5f ? 1.0f : 0.0f);

    float* d[2] = { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };
    float maxComp = 0, maxDs = 0, pk = 0;
    for (int i = 0; i < n; ++i)
    {
        float x[2] = { d[0][i], d[1] ? d[1][i] : 0.0f };
        // 1) Trittschallfilter
        for (int ch = 0; ch < numCh; ++ch)
            x[ch] = st[(size_t) ch][hp2].process (c[hp2], st[(size_t) ch][hp1].process (c[hp1], x[ch]));
        const float mono = numCh > 1 ? 0.5f * (x[0] + x[1]) : x[0];

        // 2) De-Esser (gekoppelt)
        const float dsGain = deEsser.computeGain (mono, dsAmount);
        maxDs = juce::jmax (maxDs, deEsser.getReductionDb());
        for (int ch = 0; ch < numCh; ++ch)
            x[ch] = deEsser.apply (ch, x[ch], dsGain, listen);

        // 3) Kompressor (nach dem De-Esser, damit Zischlaute ihn nicht auslösen)
        const float gr = listen ? 0.0f : (float) comp.process (numCh > 1 ? 0.5f * (x[0] + x[1]) : x[0]);
        maxComp = juce::jmax (maxComp, gr);
        const float g = juce::Decibels::decibelsToGain (-gr) * (listen ? 1.0f : smMakeup.getNextValue());
        const float og = smOut.getNextValue(), tube = smTube.getNextValue();

        // 4) Klang und optionale Röhrenwärme
        for (int ch = 0; ch < numCh; ++ch)
        {
            float y = x[ch] * g;
            if (! listen)
            {
                y = st[(size_t) ch][warmth].process (c[warmth], y);
                y = st[(size_t) ch][presence].process (c[presence], y);
                y = st[(size_t) ch][air].process (c[air], y);
                if (tube > 0.001f)
                {
                    const float sat = (std::tanh (2.2f * y + 0.1f) - std::tanh (0.1f)) / 2.2f;   // weich, leicht unsymmetrisch
                    y += tube * (sat * 1.08f - y);
                }
            }
            y *= og;
            d[ch][i] = y;
            pk = juce::jmax (pk, std::abs (y));
            if (ch == 0) vuState += (std::abs (y) - vuState) * vuCoef;
        }
    }
    compGrDb.store (maxComp);
    deEssGrDb.store (maxDs);
    vuOut.store (vuState);
    peakOut.store (juce::jmax (pk, peakOut.load()));
}

void StimmeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void StimmeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;
    auto tree = juce::ValueTree::fromXml (*xml);
    currentProgram = juce::jlimit (0, getNumPrograms() - 1, (int) tree.getProperty ("program", 0));
    apvts.replaceState (tree);
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->convertTo0to1 (rp->convertFrom0to1 (rp->getValue())));
}

juce::AudioProcessorEditor* StimmeAudioProcessor::createEditor() { return new kb::ScaledEditor (*this, new StimmeAudioProcessorEditor (*this)); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new StimmeAudioProcessor(); }
