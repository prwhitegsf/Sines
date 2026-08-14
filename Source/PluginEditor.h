/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/ToneComponent.h"
#include "UI/ScaleComponent.h"
#include "UI/KeyboardComponent.h"
#include "UI/GlobalHoldComponent.h"

//==============================================================================
/**
*/
class SinesAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                               public juce::Timer,
                                               public juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback>
{
public:
    SinesAudioProcessorEditor (SinesAudioProcessor&);
    ~SinesAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void timerCallback() override;

private:
    SinesAudioProcessor& audioProcessor;

    juce::Label fmLabel;
    juce::Label crLabel;
    juce::Label rmLabel;
    juce::Label mixerLabel;

    void initLabel(juce::Label& label, juce::String text);

    ScaleComponent scaleComponent;
    ToneComponent toneComponent0;
    ToneComponent toneComponent1;
    ToneComponent toneComponent2;
    ToneComponent toneComponent3;
    GlobalHoldComponent globalHold;

    juce::OSCSender sender;
    juce::OSCReceiver receiver;
    void showConnectionErrorMessage(const juce::String& messageText);
    void oscMessageReceived(const juce::OSCMessage& message) override;

    juce::Label oscConfig;
    juce::Label oscReceiverLabel;
    juce::Label oscSenderLabel;
    juce::Label oscReceiverAddress;
    juce::Label oscSenderAddress;
    int oscSendPort{ 8008 };
    int oscReceivePort{ 9000 };

    void configOSC();

    juce::MidiKeyboardState keyboardState;
    KeyboardComponent keyboardComponent;
    int shownBaseNote { -1 };
    int shownScale { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SinesAudioProcessorEditor)
};
