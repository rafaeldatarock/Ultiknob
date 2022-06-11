/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class UltiknobAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                      public juce::Slider::Listener
{
public:
    UltiknobAudioProcessorEditor (UltiknobAudioProcessor&);
    ~UltiknobAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    UltiknobAudioProcessor& audioProcessor;

    juce::Slider ultiknob;
    juce::Slider inputGain;
    juce::Slider outputGain;
    juce::ToggleButton dirtyMode;

    juce::Label inputGainLabel;
    juce::Label outputGainLabel;

    juce::AudioProcessorValueTreeState::SliderAttachment ultiknobAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment inputGainAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment outputGainAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment dirtyModeAttachment;

    juce::Rectangle<int> logoArea;
    juce::Rectangle<int> footerArea;

    void sliderValueChanged(juce::Slider* slider) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UltiknobAudioProcessorEditor)
};
