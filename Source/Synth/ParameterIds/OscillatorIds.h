/*
  ==============================================================================

    OscillatorIds.h
    Created: 28 Jan 2026 11:42:08am
    Author:  User

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class OscillatorIds
{
public:
    OscillatorIds() = default;
    OscillatorIds(juce::String prefix, int idx);

    void attach(const juce::AudioProcessorValueTreeState& apvts);

    juce::String freqMult;
    juce::String tune;
    juce::String tuneLfoRate;
    juce::String tuneLfoAmp;
    juce::String tuneLfoHold;

    juce::String depth;
    juce::String depthLfoRate;
    juce::String depthLfoAmp;
    juce::String depthLfoHold;

    juce::String attack;
    juce::String decay;
    juce::String sustain;
    juce::String release;

    std::atomic<float>* freqMultValue = nullptr;
    std::atomic<float>* tuneValue = nullptr;
    std::atomic<float>* tuneLfoRateValue = nullptr;
    std::atomic<float>* tuneLfoAmpValue = nullptr;
    std::atomic<float>* tuneLfoHoldValue = nullptr;
    std::atomic<float>* depthValue = nullptr;
    std::atomic<float>* depthLfoRateValue = nullptr;
    std::atomic<float>* depthLfoAmpValue = nullptr;
    std::atomic<float>* depthLfoHoldValue = nullptr;
    std::atomic<float>* attackValue = nullptr;
    std::atomic<float>* decayValue = nullptr;
    std::atomic<float>* sustainValue = nullptr;
    std::atomic<float>* releaseValue = nullptr;
};
