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
    delay()
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
    // bufferLengthInMs should be at least 1 greater than the maximum slider value the user can set
    // if slider is set to exactly the maximum buffersize, the delay has no effect
    delay.prepare(sampleRate, samplesPerBlock, 51.);
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
    delay.updateParameters(params.getRawParameterValue("DELAYTIME")->load());

    delay.processBlock(
        buffer.getArrayOfWritePointers(),
        buffer.getNumChannels(),
        buffer.getNumSamples()
    );
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
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("DELAYTIME", "Delay Time", 0.f, 50.0f, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("LOWCUT",
        "LowCut Freq",
        juce::NormalisableRange<float>(20.f, 80.f, 1.f, 0.5f),
        20.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("HIGHCUT",
        "HighCut Freq",
        juce::NormalisableRange<float>(12000.f, 20000.f, 1.f, 0.5f),
        20000.f));

    return layout;
}