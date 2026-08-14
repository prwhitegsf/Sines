/*
  ==============================================================================

    MixerIds.h
    Created: 28 Jan 2026 11:41:28am
    Author:  User

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>


class MixerIds
{
public:
    MixerIds(int index);

    void attach(const juce::AudioProcessorValueTreeState& apvts);

    juce::String idx;

    juce::String lpFreq;
    juce::String lpQ;
    juce::String hpFreq;
    juce::String hpQ;
    juce::String gain;
    juce::String pan;

    std::atomic<float>* lpFreqValue;
    std::atomic<float>* lpQValue;
    std::atomic<float>* hpFreqValue;
    std::atomic<float>* hpQValue;
    std::atomic<float>* gainValue;
    std::atomic<float>* panValue;
};
