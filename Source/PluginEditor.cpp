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
    dirtyMode("Dirty Mode"),
    inputGainAttachment(audioProcessor.params, "INPUTGAIN", inputGain),
    outputGainAttachment(audioProcessor.params, "OUTPUTGAIN", outputGain),
    ultiknobAttachment(audioProcessor.params, "PERCENTAGE", ultiknob),
    dirtyModeAttachment(audioProcessor.params, "DIRTYMODE", dirtyMode)
{
    ultiknob.setTextValueSuffix(" %");
    ultiknob.addListener(this);

    inputGain.setTextValueSuffix(" dB");
    outputGain.setTextValueSuffix(" dB");

    inputGainLabel.setText("Input Volume", juce::dontSendNotification);
    inputGainLabel.attachToComponent(&inputGain, false);
    outputGainLabel.setText("Output Volume", juce::dontSendNotification);
    outputGainLabel.attachToComponent(&outputGain, false);

    addAndMakeVisible(ultiknob);
    addAndMakeVisible(inputGain);
    addAndMakeVisible(outputGain);
    addAndMakeVisible(dirtyMode);

    addAndMakeVisible(inputGainLabel);
    addAndMakeVisible(outputGainLabel);

    setSize (480, 600);
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

    // Colour palette: https://coolors.co/ffffff-848c8e-435058-fcc200-d34e24
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

void UltiknobAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    double percentage{ slider->getValue() };

    audioProcessor.params.getRawParameterValue("DELAYTIME")->store(percentage * 0.4f);
    audioProcessor.params.getRawParameterValue("RATIO")->store(1.f + (percentage * 0.09f));

    if (dirtyMode.getToggleState() == false)
    {
        audioProcessor.params.getRawParameterValue("LOWCUT")->store(20.f + (percentage * 0.40f));
        audioProcessor.params.getRawParameterValue("HIGHCUT")->store(20'000.f - (percentage * 80.f));
    }
    else {
        audioProcessor.params.getRawParameterValue("LOWCUT")->store(20.f + (percentage * 0.60f));
        audioProcessor.params.getRawParameterValue("HIGHCUT")->store(20'000.f - (percentage * 120.f));
    }
}

