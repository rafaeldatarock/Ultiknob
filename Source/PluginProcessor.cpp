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
                       ), params (*this, nullptr, "Parameters", createParameters())
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
    // buffersize is equal to samplerate, so this buffer is 1 second long
    delayBuffer.setSize(getTotalNumInputChannels(), (int) sampleRate);

    // reset the values for smoothing of delaytime
    delayTime.reset(sampleRate, 0.5);
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

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const int bufferSize = buffer.getNumSamples();
    const int delayBufferSize = delayBuffer.getNumSamples();
 
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        fillBuffer(channel, bufferSize, delayBufferSize, channelData);
        readBuffer(channel, bufferSize, delayBufferSize, buffer, delayBuffer);
    }

    // to make sure to copy next value from mainbuffer to the right location in the delaybuffer
    writePosition += bufferSize;
    // to make sure to wrap around when getting to the end of the delaybuffer
    writePosition %= delayBufferSize; 
}

void UltiknobAudioProcessor::fillBuffer(
    int channel, 
    int bufferSize, 
    int delayBufferSize, 
    float* channelData)
{
    // copy data from main buffer to delay buffer
    if (delayBufferSize > bufferSize + writePosition)
    {
        delayBuffer.copyFrom(channel, writePosition, channelData, bufferSize);
    }
    else// we are at the end of the delaybuffer
    // make sure to wrap around and completely fill up delaybuffer
    {
        // how many samples are left until the end of the buffer
        auto samplesToEnd = delayBufferSize - writePosition;
        // fill up buffer
        delayBuffer.copyFrom(channel, writePosition, channelData, samplesToEnd);

        // how many samples end up at the start of the buffer (those overwrite the start of the buffer)
        auto samplesAtStart = bufferSize - samplesToEnd;
        // go back to beginning of buffer and overwrite it at position 0, with the rest of the bufferdata that didn't fit at the end
        delayBuffer.copyFrom(channel, 0, channelData + samplesToEnd, samplesAtStart);
    }
}

void UltiknobAudioProcessor::readBuffer(
    int channel, 
    int bufferSize, 
    int delayBufferSize, 
    juce::AudioBuffer<float>& buffer, 
    juce::AudioBuffer<float>& delayBuffer)
{
    // the delaytime is in samples (0 - 10.000)
    auto delayTimeSlider = params.getRawParameterValue("DELAYTIME")->load();
    delayTime.setTargetValue(delayTimeSlider);

    auto readPosition = writePosition - delayTime.skip(bufferSize);
    //auto readPosition = writePosition - delayTime.getNextValue();

    // if writeposition is 0, our readPosition would be negative
    // to solve this we need this to wrap, similar to the delaybuffer
    if (readPosition < 0)
        readPosition += delayBufferSize;

    auto gain = 1.0f;

    // in this case the readPos and buffer are within bounds of the delayBuffer
    if (readPosition + bufferSize < delayBufferSize)
    {
        buffer.copyFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, readPosition), bufferSize, gain, gain);
    }
    else
    {
        auto samplesToEnd = delayBufferSize - readPosition;
        buffer.copyFromWithRamp(channel, 0, delayBuffer.getReadPointer(channel, readPosition), samplesToEnd, gain, gain);

        auto samplesAtStart = bufferSize - samplesToEnd;
        buffer.copyFromWithRamp(channel, samplesToEnd, delayBuffer.getReadPointer(channel, 0), samplesAtStart, gain, gain);
    }
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

    params.push_back(std::make_unique<juce::AudioParameterFloat>("DELAYTIME", "Delay Time", 0.0f, 10'000.0f, 0.0f));

    return { params.begin(), params.end() };
}