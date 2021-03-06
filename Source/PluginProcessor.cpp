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
    delay(),
    cutFilters(),
    compressor(),
    random(juce::Time::currentTimeMillis())
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
    cutFilters.prepare(sampleRate, samplesPerBlock);

    // bufferLengthInMs should be at least 1 greater than the maximum slider value the user can set
    // if slider is set to exactly the maximum buffersize, the delay has no effect
    delay.prepare(sampleRate, samplesPerBlock, 51.);

    compressor.prepare(sampleRate, samplesPerBlock, getTotalNumInputChannels());
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
    int sampleRate = getSampleRate();
    float** writePointerArray = buffer.getArrayOfWritePointers();
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    // Filtering
    cutFilters.updateParameters(
        params.getRawParameterValue("LOWCUT")->load(),
        params.getRawParameterValue("HIGHCUT")->load()
    );
    cutFilters.processBlock(
        juce::dsp::AudioBlock<float>(buffer),
        numChannels,
        numSamples,
        sampleRate
    );
    
    // Speed fluctiation
    static int counter{ 0 };
    const int maxCount{ static_cast<int>( (sampleRate / numSamples) / 0.5 ) }; // change value once every 2 seconds
    const float maxDelayTime{ params.getRawParameterValue("DELAYTIME")->load() };
    if (counter < maxCount)
    {
        counter += 1;
    }
    else
    {
        delay.updateParameters(random.nextFloat() * maxDelayTime); // nextFloat() returns float between 0. and 1. so scale to linearly to between 0. and 40.
        counter = 0;
    }
    delay.processBlock(
        writePointerArray,
        numChannels,
        numSamples
    );

    // Compression
    bool isDirty{ static_cast<bool>(params.getRawParameterValue("DIRTYMODE")->load()) };
    if (isDirty) {
        compressor.updateParameters(
            params.getRawParameterValue("RATIO")->load(),
            params.getRawParameterValue("THRESHOLD")->load(),
            5.f,    // ATTACK
            20.f,   // RELEASE
            params.getRawParameterValue("INPUTGAIN")->load(),
            params.getRawParameterValue("OUTPUTGAIN")->load()
        );
    }
    else
    {
        compressor.updateParameters(
            params.getRawParameterValue("RATIO")->load(),
            params.getRawParameterValue("THRESHOLD")->load(),
            20.f,   // ATTACK
            100.f,  // RELEASE
            params.getRawParameterValue("INPUTGAIN")->load(),
            params.getRawParameterValue("OUTPUTGAIN")->load()
        );
    }
    compressor.processBlock(
        writePointerArray,
        numChannels,
        numSamples
    );
}

//==============================================================================
bool UltiknobAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* UltiknobAudioProcessor::createEditor()
{
    //return new juce::GenericAudioProcessorEditor(*this);
    return new UltiknobAudioProcessorEditor (*this);
}

//==============================================================================
void UltiknobAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    
    juce::MemoryOutputStream mos(destData, true);
    params.state.writeToStream(mos);
}

void UltiknobAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if ( tree.isValid() )
    {
        params.replaceState(tree);

        cutFilters.updateParameters(
            params.getRawParameterValue("LOWCUT")->load(),
            params.getRawParameterValue("HIGHCUT")->load()
        );

        compressor.updateParameters(
            params.getRawParameterValue("RATIO")->load(),
            params.getRawParameterValue("THRESHOLD")->load(),
            params.getRawParameterValue("ATTACK")->load(),
            params.getRawParameterValue("RELEASE")->load(),
            params.getRawParameterValue("INPUTGAIN")->load(),
            params.getRawParameterValue("OUTPUTGAIN")->load()
        );
    }
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

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "PERCENTAGE",
        "Effect Percentage",
        0.f,
        100.0f,
        0.0f)
    );

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "DIRTYMODE",
        "Dirty Mode",
        false)
    );

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "DELAYTIME", 
        "Delay Time", 
        0.f, 
        40.0f,
        0.0f)
    );

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "LOWCUT",
        "LowCut Freq",
        juce::NormalisableRange<float>(20.f, 80.f, 1.f, 0.5f),
        20.f)
    );

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "HIGHCUT",
        "HighCut Freq",
        juce::NormalisableRange<float>(8000.f, 20000.f, 1.f, 0.5f),
        20000.f)
    );

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "RATIO",
        "Comp Ratio",
        juce::NormalisableRange<float>(1.f, 10.f, 0.1f),
        1.f)
    );

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "THRESHOLD",
        "Comp Threshold",
        juce::NormalisableRange<float>(-36.f, 0.f, 0.5f),
        -18.f)
    );

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "ATTACK",
        "Comp Attack",
        juce::NormalisableRange<float>(5.f, 200.f, 1.f),
        20.f)
    );

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "RELEASE",
        "Comp Release",
        juce::NormalisableRange<float>(20.f, 600.f, 1.f),
        100.f)
    );

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "INPUTGAIN",
        "Comp Input level",
        juce::NormalisableRange<float>(-24.f, 24.f, 0.25f, 1.f),
        0.f)
    );

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OUTPUTGAIN",
        "Comp Output level",
        juce::NormalisableRange<float>(-24.f, 24.f, 0.25f, 1.f),
        0.f)
    );

    return layout;
}
