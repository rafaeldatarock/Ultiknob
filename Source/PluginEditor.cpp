/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
UltiknobAudioProcessorEditor::UltiknobAudioProcessorEditor (UltiknobAudioProcessor& p)
    : AudioProcessorEditor (&p), 
    audioProcessor (p), 
    ultiknob(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox),
    inputGain(juce::Slider::SliderStyle::LinearVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow),
    outputGain(juce::Slider::SliderStyle::LinearVertical, juce::Slider::TextEntryBoxPosition::TextBoxBelow),
    dirtyMode("Dirty Mode", "Make it dirty!")
{
    addAndMakeVisible(ultiknob);
    addAndMakeVisible(inputGain);
    addAndMakeVisible(outputGain);
    addAndMakeVisible(dirtyMode);
    
    setSize (400, 600);
}

UltiknobAudioProcessorEditor::~UltiknobAudioProcessorEditor()
{
}

//==============================================================================
void UltiknobAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (48.f);
    g.drawFittedText("Ultiknob", logoArea, juce::Justification::centred, 1);

    g.setFont(14.f);
    g.drawFittedText ("DataRock Studio, 2022", footerArea, juce::Justification::centredLeft, 1);
}

void UltiknobAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();
    logoArea = bounds.removeFromTop(bounds.getHeight() * 0.30);
    footerArea = bounds.removeFromBottom(bounds.getHeight() * 0.20);

    dirtyMode.setBounds(bounds.removeFromBottom(bounds.getHeight() * 0.20));
    inputGain.setBounds(bounds.removeFromLeft(bounds.getWidth() * 0.25));
    outputGain.setBounds(bounds.removeFromRight(bounds.getWidth() * 0.333333333));
    ultiknob.setBounds(bounds);
}
