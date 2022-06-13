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
    getLookAndFeel().setColour(0, juce::Colour(55, 55, 55));    // Gunmetal
    getLookAndFeel().setColour(1, juce::Colour(67, 80, 88));    // Charcoal
    getLookAndFeel().setColour(2, juce::Colour(132, 140, 142)); // Grey
    getLookAndFeel().setColour(3, juce::Colour(255, 255, 255)); // White
    getLookAndFeel().setColour(10, juce::Colour(252, 194, 0));  // Yellow
    getLookAndFeel().setColour(11, juce::Colour(211, 78, 36));  // Dark Orange
    getLookAndFeel().setColour(20, juce::Colour(20, 20, 20));   // Black

    getLookAndFeel().setColour(juce::Slider::thumbColourId, getLookAndFeel().findColour(10));
    getLookAndFeel().setColour(juce::Slider::trackColourId, getLookAndFeel().findColour(2));
    getLookAndFeel().setColour(juce::Slider::backgroundColourId, getLookAndFeel().findColour(0));
    getLookAndFeel().setColour(juce::Slider::rotarySliderOutlineColourId, getLookAndFeel().findColour(0));
    getLookAndFeel().setColour(juce::Slider::rotarySliderFillColourId, getLookAndFeel().findColour(2));

    g.setColour(getLookAndFeel().findColour(20));
    g.fillRect(getLocalBounds());

    g.setColour (getLookAndFeel().findColour (3));
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
    bounds.removeFromLeft(bounds.getWidth() * 0.08);
    bounds.removeFromRight(bounds.getWidth() * 0.08);
    logoArea = bounds.removeFromTop(bounds.getHeight() * 0.25);
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

