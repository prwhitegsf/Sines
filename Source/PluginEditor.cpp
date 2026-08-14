/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SinesAudioProcessorEditor::SinesAudioProcessorEditor (SinesAudioProcessor& p)
        : AudioProcessorEditor (&p), audioProcessor (p), scaleComponent(p.apvts, p.scale.getNames()),
          toneComponent0(p.apvts, 0),
          toneComponent1(p.apvts, 1),
          toneComponent2(p.apvts, 2),
          toneComponent3(p.apvts, 3),
          keyboardComponent(keyboardState, juce::MidiKeyboardComponent::Orientation::horizontalKeyboard)
{
    startTimerHz(10);

    addAndMakeVisible(toneComponent0);
    addAndMakeVisible(toneComponent1);
    addAndMakeVisible(toneComponent2);
    addAndMakeVisible(toneComponent3);


    keyboardComponent.setMidiChannel(2);
    keyboardState.addListener(&audioProcessor.getMidiMessageCollector());

    initLabel(fmLabel, "Freq Mod");
    initLabel(crLabel, "Carrier");
    initLabel(rmLabel, "Ring Mod");
    initLabel(mixerLabel, "Mixer");

    addAndMakeVisible(scaleComponent);
    if (juce::JUCEApplication::isStandaloneApp())
    {
        addAndMakeVisible(keyboardComponent);
    }

    configOSC();
    addAndMakeVisible(globalHold);

    setSize(1000, 830);
}

SinesAudioProcessorEditor::~SinesAudioProcessorEditor()
{
    keyboardState.removeListener(&audioProcessor.getMidiMessageCollector());
}
//==============================================================================
void SinesAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);

    if (!juce::JUCEApplication::isStandaloneApp())
    {
        g.setColour(juce::Colour(15,15,15));
        juce::Rectangle<float> filler(500, 720, 250, 100);
        g.fillRect(filler);

        g.setColour(juce::Colour(105,105,105));
        g.drawText("... reserved for future use ...", filler, juce::Justification::centred, true);
    }
}

void SinesAudioProcessorEditor::resized()
{
    const int topMargin = 10;
    const int toneWidth = 250;
    const int toneHeight = 710;
    toneComponent0.setBounds(0, topMargin, toneWidth, toneHeight);
    toneComponent1.setBounds(toneComponent0.getRight(), topMargin, toneWidth, toneHeight);
    toneComponent2.setBounds(toneComponent1.getRight(), topMargin, toneWidth, toneHeight);
    toneComponent3.setBounds(toneComponent2.getRight(), topMargin, toneWidth, toneHeight);

    fmLabel.setBounds(0, 0, 1000, 20);
    crLabel.setBounds(0, 200, 1000, 20);
    rmLabel.setBounds(0, 390, 1000, 20);
    mixerLabel.setBounds(0, 580, 1000, 20);

    if (juce::JUCEApplication::isStandaloneApp())
    {
        scaleComponent.setBounds(0, toneHeight+topMargin, toneWidth, 100);
        keyboardComponent.setBounds(scaleComponent.getRight(), toneHeight+topMargin, toneWidth, 100);
        globalHold.setBounds(scaleComponent.getRight() + 500, toneHeight+topMargin, toneWidth, 100);
    }
    else
    {
        scaleComponent.setBounds(0, toneHeight+topMargin, toneWidth, 100);
        oscReceiverLabel.setBounds(scaleComponent.getRight() + 10, 730, 125, 20);
        oscReceiverAddress.setBounds(scaleComponent.getRight() + 10, oscReceiverLabel.getBottom() + 5, 125, 20);
        // oscSenderLabel.setBounds(oscReceiverLabel.getRight() + 10, 730, 125, 20);
        // oscSenderAddress.setBounds(oscReceiverLabel.getRight() + 10, oscSenderLabel.getBottom() + 5, 125, 20);
        globalHold.setBounds(scaleComponent.getRight() + 500, toneHeight+topMargin, toneWidth, 100);

    }
}

void SinesAudioProcessorEditor::timerCallback()
{

    const int baseNote = (int)*audioProcessor.apvts.getRawParameterValue("SCALE_BASE_NOTE");
    const int scaleIdx = (int)*audioProcessor.apvts.getRawParameterValue("SCALE_SELECT");

    if(baseNote != shownBaseNote || scaleIdx != shownScale)
    {
        shownBaseNote = baseNote;
        shownScale = scaleIdx;

        const int notesPerOctave = (int) audioProcessor.scale.getRatios(scaleIdx).size();
        scaleComponent.notesPerOctave.setText(juce::String(notesPerOctave),juce::dontSendNotification);
        keyboardComponent.setBaseNote(baseNote);
        keyboardComponent.setNotesPerOctave(notesPerOctave);

    }


    if (globalHold.holdAll.getToggleState())
    {
        globalHold.holdAll.setToggleState(false, juce::dontSendNotification);
        toneComponent0.holdAllLfo();
        toneComponent1.holdAllLfo();
        toneComponent2.holdAllLfo();
        toneComponent3.holdAllLfo();
    }

    if (globalHold.releaseAll.getToggleState())
    {
        globalHold.releaseAll.setToggleState(false, juce::dontSendNotification);
        toneComponent0.releaseAllLfo();
        toneComponent1.releaseAllLfo();
        toneComponent2.releaseAllLfo();
        toneComponent3.releaseAllLfo();
    }
}

void SinesAudioProcessorEditor::initLabel(juce::Label& label, juce::String text)
{
    label.setFont(juce::FontOptions(16.0f));
    label.setText(text, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, juce::Colours::antiquewhite);
    label.setColour(juce::Label::backgroundColourId, juce::Colour(50, 50, 50));
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
    label.toFront(false);
}

void SinesAudioProcessorEditor::showConnectionErrorMessage(const juce::String& messageText)
{
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                           "Connection error",
                                           messageText,
                                           "OK");
}

void SinesAudioProcessorEditor::oscMessageReceived(const juce::OSCMessage& message)
{
    const auto address = message.getAddressPattern().toString();

    if (address.startsWith("/1"))
        toneComponent0.oscReceive(message);
    else if (address.startsWith("/2"))
        toneComponent1.oscReceive(message);
    else if (address.startsWith("/3"))
        toneComponent2.oscReceive(message);
    else if (address.startsWith("/4"))
        toneComponent3.oscReceive(message);

}

void SinesAudioProcessorEditor::configOSC()
{
    // Receiver
    oscReceiverLabel.setText("local ip:port", juce::dontSendNotification);
    oscReceiverLabel.setJustificationType(juce::Justification::centred);

    juce::IPAddress localIP = juce::IPAddress::getLocalAddress();
    juce::String localIpString = localIP.toString() + ":" + juce::String(oscReceivePort);

    oscReceiverAddress.setText(localIpString, juce::dontSendNotification);
    oscReceiverAddress.setJustificationType(juce::Justification::centred);
    oscReceiverAddress.setEditable(true, false, false);

    if (!receiver.connect(oscReceivePort))
        showConnectionErrorMessage("Error: receiver could not connect to provided UDP port");

    oscReceiverAddress.onTextChange = [this]
    {
        oscReceivePort = oscReceiverAddress.getText().getLastCharacters(4).getIntValue();
        auto ip = oscReceiverAddress.getText().dropLastCharacters(5);
        if (!receiver.connect(oscReceivePort))
            showConnectionErrorMessage("Error: receiver could not connect to provided UDP port");
    };


    // Sender
    oscSenderLabel.setText("remote ip:port", juce::dontSendNotification);
    oscSenderLabel.setJustificationType(juce::Justification::centred);


    oscSenderAddress.setText("127.0.0.1:8008", juce::dontSendNotification);
    oscSenderAddress.setJustificationType(juce::Justification::centred);
    oscSenderAddress.setEditable(true, false, false);


    oscSenderAddress.onTextChange = [this]
    {
        oscSendPort = oscSenderAddress.getText().getLastCharacters(4).getIntValue();
        auto ip = oscSenderAddress.getText().dropLastCharacters(5);
        if (!sender.connect(ip, oscSendPort))
            showConnectionErrorMessage("Error: sender could not connect to provided UDP port");
    };


    receiver.addListener(this);

    addAndMakeVisible(oscReceiverLabel);
    addAndMakeVisible(oscReceiverAddress);
    // addAndMakeVisible(oscSenderLabel);
    // addAndMakeVisible(oscSenderAddress);

}