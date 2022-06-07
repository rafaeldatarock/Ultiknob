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
			// prepare the chains

			juce::dsp::ProcessSpec spec;
			spec.maximumBlockSize = blockSize;
			spec.numChannels = 1;
			spec.sampleRate = sampleRate;

			leftChain.prepare(spec);
			rightChain.prepare(spec);

			// prepare the filters

			auto lowCutCoefs = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
				lowCutFreq,
				sampleRate,
				1);

			auto highCutCoefs = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
				highCutFreq,
				sampleRate,
				1);

			auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
			auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();

			auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
			auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();

			makeCutFilter(leftLowCut, lowCutCoefs);
			makeCutFilter(rightLowCut, lowCutCoefs);

			makeCutFilter(leftHighCut, highCutCoefs);
			makeCutFilter(rightHighCut, highCutCoefs);
		}

		void updateParameters(float _lowCutFreq, float _highCutFreq)
		{
			lowCutFreq = _lowCutFreq;
			highCutFreq = _highCutFreq;
		}

		void processBlock(juce::dsp::AudioBlock<float> block, int numChannels, int numSamples, double sampleRate)
		{
			// configure the filters

			auto lowCutCoefs = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
				lowCutFreq,
				sampleRate,
				1);

			auto highCutCoefs = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
				highCutFreq,
				sampleRate,
				1);

			auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
			auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();

			auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
			auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
			
			makeCutFilter(leftLowCut, lowCutCoefs);
			makeCutFilter(rightLowCut, lowCutCoefs);

			makeCutFilter(leftHighCut, highCutCoefs);
			makeCutFilter(rightHighCut, highCutCoefs);

			// Process the chains

			auto leftBlock = block.getSingleChannelBlock(0);
			auto rightBlock = block.getSingleChannelBlock(1);

			juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
			juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

			leftChain.process(leftContext);
			rightChain.process(rightContext);
		}

	protected:
		float lowCutFreq, highCutFreq;

		using Filter = juce::dsp::IIR::Filter<float>;
		
		using CutFilter = juce::dsp::ProcessorChain<Filter>;

		using CutFilterChain = juce::dsp::ProcessorChain<CutFilter, CutFilter>;

		CutFilterChain leftChain, rightChain;

		enum ChainPositions
		{
			LowCut,
			HighCut
		};

		using Coefficients = Filter::CoefficientsPtr;

		template<typename ChainType, typename CoefficientType>
		void makeCutFilter(ChainType& cut, const CoefficientType& cutCoefs)
		{
			cut.setBypassed<0>(true); // may be unnecesary

			*cut.get<0>().coefficients = *cutCoefs[0];

			cut.setBypassed<0>(false);
		}
	};
}