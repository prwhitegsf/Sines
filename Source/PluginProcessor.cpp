
#include "PluginProcessor.h"
#include "PluginEditor.h"

SinesAudioProcessor::SinesAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
        : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
    .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
#endif
    .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
), apvts(*this, nullptr, "Parameters", createParams()),synth()
#endif
{
    kbState.addListener(&getMidiMessageCollector());
    scale.setKeyboard(synth.keyboard, 0);

    for(auto* ids : {&carrierIds, &fmIds, &rmIds})
        for(auto& id : *ids)
            id.attach(apvts);

    for(auto& id : mixerIds)
        id.attach(apvts);
}

SinesAudioProcessor::~SinesAudioProcessor()
{
}

//==============================================================================
const juce::String SinesAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SinesAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool SinesAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool SinesAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double SinesAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SinesAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int SinesAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SinesAudioProcessor::setCurrentProgram (int)
{
}

const juce::String SinesAudioProcessor::getProgramName (int)
{
    return {};
}

void SinesAudioProcessor::changeProgramName (int, const juce::String& )
{
}

//==============================================================================
void SinesAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{

    synth.setCurrentPlaybackSampleRate(sampleRate);
    midiMessageCollector.reset(sampleRate);

    juce::dsp::ProcessSpec spec{ sampleRate,(std::uint_fast32_t)samplesPerBlock,(std::uint_fast32_t)getTotalNumOutputChannels() };


    for (auto& toneLfo : synth.toneLfos)
    {
        toneLfo.prepare(spec);
    }

    for (auto& mixer : synth.mixers)
    {
        mixer.prepare(spec);
    }

    for (int i = 0; i < synth.getNumVoices(); i++)
    {
        if (auto voice = dynamic_cast<Voice*>(synth.getVoice(i)))
        {
            voice->prepareToPlay(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
        }
    }
}

void SinesAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SinesAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void SinesAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    midiMessageCollector.removeNextBlockOfMessages(midiMessages, buffer.getNumSamples());

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    updateScale();

    for (int j =0; j < toneCount; ++j)
    {
        updateLfos(j);
        updateMixers(j);
    }


    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<Voice*>(synth.getVoice(i)))
        {
            for (int j = 0; j < toneCount; ++j)
            {
                updateFreqMods(j, voice);
                updateCarriers(j, voice);
                updateRingMods(j, voice);
            }
            voice->clearActiveOffNotes();
        }
    }

    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

//==============================================================================
bool SinesAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SinesAudioProcessor::createEditor()
{
    return new SinesAudioProcessorEditor (*this);
}

//==============================================================================
void SinesAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SinesAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout SinesAudioProcessor::createParams()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    // Scale Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SCALE_BASE_NOTE", "scale_base_note", juce::NormalisableRange<float> { 1.0f, 127.0f, 1.0f}, 69, ""));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("SCALE_SELECT", "scale_select", scale.getNames(), 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("NOTES_PER_OCTAVE", "notes_per_octave", juce::NormalisableRange<float> { 4.0f, 25.0f, 1.0f}, 19, ""));

    for (int i = 0; i < toneCount; ++i)
    {
        // Carrier Parameters
        params.push_back(std::make_unique<juce::AudioParameterFloat>(carrierIds[i].freqMult, carrierIds[i].freqMult.toLowerCase(), juce::NormalisableRange<float> { 0.25f, 4.0f, 0.25f}, 1.0f, ""));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(carrierIds[i].tune, carrierIds[i].tune.toLowerCase(), juce::NormalisableRange<float> { -10.0f, 10.0f, 0.01f}, 0.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(carrierIds[i].tuneLfoRate, carrierIds[i].tuneLfoRate.toLowerCase(), juce::NormalisableRange<float> { 0.0f, 300.0f, 0.01f}, 30.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(carrierIds[i].tuneLfoAmp, carrierIds[i].tuneLfoAmp.toLowerCase(), juce::NormalisableRange<float> { -10.0f, 10.0f, 0.01f}, 0.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterBool>(carrierIds[i].tuneLfoHold, carrierIds[i].tuneLfoHold.toLowerCase(), true));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(carrierIds[i].depth, carrierIds[i].depth.toLowerCase(), juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f}, 1.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(carrierIds[i].depthLfoRate, carrierIds[i].depthLfoRate.toLowerCase(), juce::NormalisableRange<float> { 0.0f, 300.0f, 0.01f}, 30.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(carrierIds[i].depthLfoAmp, carrierIds[i].depthLfoAmp.toLowerCase(), juce::NormalisableRange<float> { -1.0f, 1.0f, 0.01f}, 0.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterBool>(carrierIds[i].depthLfoHold, carrierIds[i].depthLfoHold.toLowerCase(), true));



        params.push_back(std::make_unique<juce::AudioParameterFloat>(carrierIds[i].attack, carrierIds[i].attack.toLowerCase(), juce::NormalisableRange<float> { 0.01f, 120.0f, 0.01f}, 1.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(carrierIds[i].decay, carrierIds[i].decay.toLowerCase(), juce::NormalisableRange<float> { 0.01f, 120.0f, 0.01f}, 1.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(carrierIds[i].sustain, carrierIds[i].sustain.toLowerCase(), juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f}, 1.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(carrierIds[i].release, carrierIds[i].release.toLowerCase(), juce::NormalisableRange<float> { 0.01f, 120.0f, 0.01f}, 1.0f, ""));

        // Freq Mod Parameters
        params.push_back(std::make_unique<juce::AudioParameterFloat>(fmIds[i].freqMult, fmIds[i].freqMult.toLowerCase(), juce::NormalisableRange<float> { 0.25f, 4.0f, 0.25f}, 1.0f, ""));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(fmIds[i].tune, fmIds[i].tune.toLowerCase(), juce::NormalisableRange<float> { -10.0f, 10.0f, 0.01f}, 0.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(fmIds[i].tuneLfoRate, fmIds[i].tuneLfoRate.toLowerCase(), juce::NormalisableRange<float> { 0.0f, 300.0f, 0.01f}, 30.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(fmIds[i].tuneLfoAmp, fmIds[i].tuneLfoAmp.toLowerCase(), juce::NormalisableRange<float> { -10.0f, 10.0f, 0.01f}, 0.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterBool>(fmIds[i].tuneLfoHold, fmIds[i].tuneLfoHold.toLowerCase(), true));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(fmIds[i].depth, fmIds[i].depth.toLowerCase(), juce::NormalisableRange<float> { -6.0f, 6.0f, 0.01f}, 0.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(fmIds[i].depthLfoRate, fmIds[i].depthLfoRate.toLowerCase(), juce::NormalisableRange<float> { 0.0f, 300.0f, 0.01f}, 30.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(fmIds[i].depthLfoAmp, fmIds[i].depthLfoAmp.toLowerCase(), juce::NormalisableRange<float> { -1.0f, 1.0f, 0.01f}, 0.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterBool>(fmIds[i].depthLfoHold, fmIds[i].depthLfoHold.toLowerCase(), true));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(fmIds[i].attack, fmIds[i].attack.toLowerCase(), juce::NormalisableRange<float> { 0.01f, 120.0f, 0.01f}, 1.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(fmIds[i].decay, fmIds[i].decay.toLowerCase(), juce::NormalisableRange<float> { 0.01f, 120.0f, 0.01f}, 1.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(fmIds[i].sustain, fmIds[i].sustain.toLowerCase(), juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f}, 1.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(fmIds[i].release, fmIds[i].release.toLowerCase(), juce::NormalisableRange<float> { 0.01f, 120.0f, 0.01f}, 1.0f, ""));

        // Ring Mod Parameters
        params.push_back(std::make_unique<juce::AudioParameterFloat>(rmIds[i].freqMult, rmIds[i].freqMult.toLowerCase(), juce::NormalisableRange<float> { 0.25f, 4.0f, 0.25f}, 1.0f, ""));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(rmIds[i].tune, rmIds[i].tune.toLowerCase(), juce::NormalisableRange<float> { -10.0f, 10.0f, 0.01f}, 0.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(rmIds[i].tuneLfoRate, rmIds[i].tuneLfoRate.toLowerCase(), juce::NormalisableRange<float> { 0.0f, 300.0f, 0.01f}, 30.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(rmIds[i].tuneLfoAmp, rmIds[i].tuneLfoAmp.toLowerCase(), juce::NormalisableRange<float> { -10.0f, 10.0f, 0.01f}, 0.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterBool>(rmIds[i].tuneLfoHold, rmIds[i].tuneLfoHold.toLowerCase(), true));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(rmIds[i].depth, rmIds[i].depth.toLowerCase(), juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f}, 0.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(rmIds[i].depthLfoRate, rmIds[i].depthLfoRate.toLowerCase(), juce::NormalisableRange<float> { 0.0f, 300.0f, 0.01f}, 30.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(rmIds[i].depthLfoAmp, rmIds[i].depthLfoAmp.toLowerCase(), juce::NormalisableRange<float> { -1.0f, 1.0f, 0.01f}, 0.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterBool>(rmIds[i].depthLfoHold, rmIds[i].depthLfoHold.toLowerCase(), true));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(rmIds[i].attack, rmIds[i].attack.toLowerCase(), juce::NormalisableRange<float> { 0.01f, 120.0f, 0.01f}, 1.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(rmIds[i].decay, rmIds[i].decay.toLowerCase(), juce::NormalisableRange<float> { 0.01f, 120.0f, 0.01f}, 1.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(rmIds[i].sustain, rmIds[i].sustain.toLowerCase(), juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f}, 1.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(rmIds[i].release, rmIds[i].release.toLowerCase(), juce::NormalisableRange<float> { 0.01f, 120.0f, 0.01f}, 1.0f, ""));


        // MixerParameters
        params.push_back(std::make_unique<juce::AudioParameterFloat>(mixerIds[i].lpFreq, mixerIds[i].lpFreq.toLowerCase(), juce::NormalisableRange<float> { 20.00f, 12000.0f, 1.0f, 0.15f}, 12000.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(mixerIds[i].lpQ, mixerIds[i].lpQ.toLowerCase(), juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f}, 0.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(mixerIds[i].hpFreq, mixerIds[i].hpFreq.toLowerCase(), juce::NormalisableRange<float> { 20.00f, 12000.0f, 1.0f, 0.15f}, 20.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(mixerIds[i].hpQ, mixerIds[i].hpQ.toLowerCase(), juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f}, 0.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(mixerIds[i].gain, mixerIds[i].gain.toLowerCase(), juce::NormalisableRange<float> { 0.00f, 1.0f, 0.01f}, 0.0f, ""));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(mixerIds[i].pan, mixerIds[i].pan.toLowerCase(), juce::NormalisableRange<float> { -1.0f, 1.0f, 0.01f}, 0.0f, ""));
    }
    return { params.begin(), params.end() };
}

void SinesAudioProcessor::updateScale()
{
    const auto& baseNote = *apvts.getRawParameterValue("SCALE_BASE_NOTE");
    const auto& scaleSelect = *apvts.getRawParameterValue("SCALE_SELECT");

    if (baseNote != scale.baseNote || scaleSelect != scale.currScale)
    {
        scale.baseNote = (int)baseNote;
        scale.currScale = (int)scaleSelect;
        scale.setKeyboard(synth.keyboard, (int)scaleSelect);
    }
}

void SinesAudioProcessor::updateCarriers(int idx, Voice*& voice) const
{
    const auto& ids = carrierIds[idx];
    voice->updateCarriers(idx, *ids.freqMultValue, *ids.tuneValue, *ids.depthValue, *ids.attackValue, *ids.decayValue, *ids.sustainValue, *ids.releaseValue);
}

void SinesAudioProcessor::updateFreqMods(int idx, Voice*& voice) const
{
    const auto& ids = fmIds[idx];
    voice->updateFreqMods(idx, *ids.freqMultValue, *ids.tuneValue, *ids.depthValue, *ids.attackValue, *ids.decayValue, *ids.sustainValue, *ids.releaseValue);
}

void SinesAudioProcessor::updateRingMods(int idx, Voice*& voice) const
{
    const auto& ids = rmIds[idx];
    voice->updateRingMods(idx, *ids.freqMultValue, *ids.tuneValue, *ids.depthValue, *ids.attackValue, *ids.decayValue, *ids.sustainValue, *ids.releaseValue);
}


void SinesAudioProcessor::updateLfos(int idx) {
    const auto &fmId = fmIds[idx];
    const auto &crId = carrierIds[idx];
    const auto &rmId = rmIds[idx];

    synth.toneLfos[idx].updateLfos(*fmId.tuneLfoRateValue, *fmId.tuneLfoAmpValue, *fmId.tuneLfoHoldValue,
                                   *fmId.depthLfoRateValue, *fmId.depthLfoAmpValue, *fmId.depthLfoHoldValue,
                                   *crId.tuneLfoRateValue, *crId.tuneLfoAmpValue, *crId.tuneLfoHoldValue,
                                   *crId.depthLfoRateValue, *crId.depthLfoAmpValue, *crId.depthLfoHoldValue,
                                   *rmId.tuneLfoRateValue, *rmId.tuneLfoAmpValue, *rmId.tuneLfoHoldValue,
                                   *rmId.depthLfoRateValue, *rmId.depthLfoAmpValue, *rmId.depthLfoHoldValue);


}
void SinesAudioProcessor::updateMixers(int idx)
{
    const auto& ids = mixerIds[idx];
    synth.updateMixers(idx, *ids.lpFreqValue, *ids.lpQValue, *ids.hpFreqValue, *ids.hpQValue, *ids.gainValue, *ids.panValue);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SinesAudioProcessor();
}
