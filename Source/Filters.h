#pragma once

namespace dsp
{
	struct CutFilters 
	{
		CutFilters() :
			lowCutFreq(20.f),
			highCutFreq(20'000.f)
		{}

		void prepare(double sampleRate, int blockSize)
		{
			auto lowCutCoefs = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
				lowCutFreq,
				sampleRate,
				1);

			auto highCutCoefs = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
				highCutFreq,
				sampleRate,
				1);

			juce::dsp::ProcessSpec spec;
			spec.maximumBlockSize = blockSize;
			spec.numChannels = 1;
			spec.sampleRate = sampleRate;

			leftChain.prepare(spec);
			rightChain.prepare(spec);
		}

		void updateParameters(float _lowCutFreq, float _highCutFreq)
		{
			lowCutFreq = _lowCutFreq;
			highCutFreq = _highCutFreq;
		}

		void processBlock(juce::dsp::AudioBlock<float> block, int numChannels, int numSamples)
		{
			auto leftBlock = block.getSingleChannelBlock(0);
			auto rightBlock = block.getSingleChannelBlock(1);

			juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
			juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

			leftChain.process(leftContext);
			rightChain.process(rightContext);
		}

	protected:
		using Filter = juce::dsp::IIR::Filter<float>;

		using CutFilterChain = juce::dsp::ProcessorChain<Filter>;

		CutFilterChain leftChain, rightChain;

		float lowCutFreq, highCutFreq;
	};
}