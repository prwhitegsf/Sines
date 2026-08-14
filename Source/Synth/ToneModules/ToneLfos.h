/*
  ==============================================================================

    ToneLfos.h
    Created: 22 Feb 2026 9:11:40am
    Author:  User

  ==============================================================================
*/

#pragma once
#include "ToneSubModules/Lfo.h"



class ToneLfos
{

public:


    void prepare(const juce::dsp::ProcessSpec& spec);

    void updateLfos(
        const float fmTuneRate,  const float fmTuneAmp,  const bool fmTuneHold,
        const float fmDepthRate, const float fmDepthAmp, const bool fmDepthHold,
        const float crTuneRate,  const float crTuneAmp,  const bool crTuneHold,
        const float crDepthRate, const float crDepthAmp, const bool crDepthHold,
        const float rmTuneRate,  const float rmTuneAmp,  const bool rmTuneHold,
        const float rmDepthRate, const float rmDepthAmp, const bool rmDepthHold);
    
    void setSamples(juce::AudioSampleBuffer& wavetable);

    [[nodiscard]] float getFmTuneSample() const {return fmTuneSample; }
    [[nodiscard]] float getFmDepthSample() const {return fmDepthSample; }

    [[nodiscard]] float getCrTuneSample() const {return crTuneSample; }
    [[nodiscard]] float getCrDepthSample() const {return crDepthSample; }

    [[nodiscard]] float getRmTuneSample() const {return rmTuneSample; }
    [[nodiscard]] float getRmDepthSample() const {return rmDepthSample; }

    static float depthFrom(Lfo &lfo);


private:
    Lfo fmTuneLfo;
    Lfo fmDepthLfo;
    Lfo crTuneLfo;
    Lfo crDepthLfo;
    Lfo rmTuneLfo;
    Lfo rmDepthLfo;

    float fmTuneSample{}, fmDepthSample{};
    float crTuneSample{}, crDepthSample{};
    float rmTuneSample{}, rmDepthSample{};
  

};
