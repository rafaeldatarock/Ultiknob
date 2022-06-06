/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
UltiknobAudioProcessor::UltiknobAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), 
    params (*this, nullptr, "Parameters", createParameters()),
    oversampler(2, 2, juce::dsp::Oversampling<float>::FilterType::filterHalfBandFIREquiripple)
#endif
{
}

UltiknobAudioProcessor::~UltiknobAudioProcessor()
{
}

//==============================================================================
const juce::String UltiknobAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool UltiknobAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool UltiknobAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool UltiknobAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double UltiknobAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int UltiknobAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int UltiknobAudioProcessor::getCurrentProgram()
{
    return 0;
}

void UltiknobAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String UltiknobAudioProcessor::getProgramName (int index)
{
    return {};
}

void UltiknobAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void UltiknobAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    double oversamplingRate = sampleRate * oversampler.getOversamplingFactor();
    int oversamplesPerBlock = samplesPerBlock * (int)oversampler.getOversamplingFactor();

    // reset the values for smoothing of delayTime
    delayTime.reset(oversamplingRate, 2.0);
    delayTime.setCurrentAndTargetValue(params.getRawParameterValue("DELAYTIME")->load());

    // prepare the delayLine
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = oversamplingRate;
    spec.maximumBlockSize = oversamplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    delayLine.prepare(spec);
    delayLine.setMaximumDelayInSamples((2000 * (int)oversampler.getOversamplingFactor()) + oversamplesPerBlock);

    oversampler.reset();
    oversampler.initProcessing(static_cast<size_t>(samplesPerBlock));
}

void UltiknobAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool UltiknobAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
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

void UltiknobAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    const int bufferSize{ buffer.getNumSamples() };

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, bufferSize);

    delayTime.setTargetValue(params.getRawParameterValue("DELAYTIME")->load());

    juce::dsp::AudioBlock<float> audioBlock{ buffer };
    juce::dsp::AudioBlock<float> upsampledBlock;

    upsampledBlock = oversampler.processSamplesUp(audioBlock);
 
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        float* upsampledBlockPointer = upsampledBlock.getChannelPointer(channel);

        for (auto s = 0; s < upsampledBlock.getNumSamples(); ++s)
        {
            delayLine.pushSample(channel, upsampledBlockPointer[s]);
            upsampledBlockPointer[s] = delayLine.popSample(channel, delayTime.getNextValue(), true);
        }
    }

    oversampler.processSamplesDown(audioBlock);
}

//==============================================================================
bool UltiknobAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* UltiknobAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
    //return new UltiknobAudioProcessorEditor (*this);
}

//==============================================================================
void UltiknobAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void UltiknobAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new UltiknobAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout UltiknobAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("DELAYTIME", "Delay Time", 0.0f, 8'000.0f, 0.0f));

    return { params.begin(), params.end() };
}