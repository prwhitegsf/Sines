/*
  ==============================================================================

    ToneLfos.cpp
    Created: 22 Feb 2026 9:11:40am
    Author:  User

  ==============================================================================
*/

#include "ToneLfos.h"

void ToneLfos::prepare(const juce::dsp::ProcessSpec& spec)
{
    fmTuneLfo.prepare(spec);
    fmDepthLfo.prepare(spec);
   
    crTuneLfo.prepare(spec);
    crDepthLfo.prepare(spec);
   
    rmTuneLfo.prepare(spec);
    rmDepthLfo.prepare(spec);
}


void ToneLfos::updateLfos(const float fmTuneRate, const float fmTuneAmp, const bool fmTuneHold,
    const float fmDepthRate, const float fmDepthAmp, const bool fmDepthHold,
    const float crTuneRate, const float crTuneAmp, const bool crTuneHold,
    const float crDepthRate, const float crDepthAmp, const bool crDepthHold,
    const float rmTuneRate, const float rmTuneAmp, const bool rmTuneHold,
    const float rmDepthRate, const float rmDepthAmp, const bool rmDepthHold)
{
    fmTuneLfo.update(fmTuneRate ? 1.0f / fmTuneRate : 0.0f, fmTuneAmp, fmTuneHold);
    fmDepthLfo.update(fmDepthRate ? 1.0f / fmDepthRate : 0.0f, fmDepthAmp, fmDepthHold);
   
    crTuneLfo.update(crTuneRate ? 1.0f / crTuneRate : 0.0f, crTuneAmp, crTuneHold);
    crDepthLfo.update(crDepthRate ? 1.0f / crDepthRate : 0.0f, crDepthAmp, crDepthHold);
    
    rmTuneLfo.update(rmTuneRate ? 1.0f / rmTuneRate : 0.0f, rmTuneAmp, rmTuneHold);
    rmDepthLfo.update(rmDepthRate ? 1.0f / rmDepthRate : 0.0f, rmDepthAmp, rmDepthHold);
}

void ToneLfos::setSamples(juce::AudioSampleBuffer& wavetable)
{
    fmTuneLfo.setSample(wavetable);
    fmTuneSample = fmTuneLfo.getNextSample();

    fmDepthLfo.setSample(wavetable);
    fmDepthSample = depthFrom(fmDepthLfo);
   
    crTuneLfo.setSample(wavetable);
    crTuneSample = crTuneLfo.getNextSample();

    crDepthLfo.setSample(wavetable);
    crDepthSample = depthFrom(crDepthLfo);

    rmTuneLfo.setSample(wavetable);
    rmTuneSample = rmTuneLfo.getNextSample();

    rmDepthLfo.setSample(wavetable);
    rmDepthSample = depthFrom(rmDepthLfo);
}

float ToneLfos::depthFrom(Lfo &lfo) {
    const auto depth = (lfo.getNextSample() + 1.0f) / 2.0f;
    const auto amp   = lfo.getAmplitude();
    return (1.0f - amp) + (depth * amp);
}
