#pragma once

namespace dsp {
	struct Compressor
	{
		Compressor() :
			compressor(),
			ratio(1.f),
			threshold(0.f),
			attack(20.f),
			release(100.f),
			inputGain(),
			outputGain()
		{}

		void prepare(double sampleRate, int blockSize, int numChannels)
		{
			juce::dsp::ProcessSpec spec;
			spec.maximumBlockSize = blockSize;
			spec.numChannels = numChannels;
			spec.sampleRate = sampleRate;

			compressor.prepare(spec);
			compressor.setRatio(ratio);
			compressor.setThreshold(threshold);
			compressor.setAttack(attack);
			compressor.setRelease(release);

			inputGain.setRampDurationSeconds(0.5);
			outputGain.setRampDurationSeconds(0.5);
		}

		void updateParameters(
			float _ratio, 
			float _threshold, 
			float _attack, 
			float _release,
			float _inputGain,
			float _outputGain)
		{
			ratio = _ratio;
			threshold = _threshold;
			attack = _attack;
			release = _release;
			inputGain.setGainDecibels(_inputGain);
			outputGain.setGainDecibels(_outputGain);
		}

		void processBlock(float** samples, int numChannels, int numSamples)
		{
			compressor.setRatio(ratio);
			compressor.setThreshold(threshold);
			compressor.setAttack(attack);
			compressor.setRelease(release);

			for (auto channel = 0; channel < numChannels; ++channel)
			{
				auto samplesSingleChannel = samples[channel];

				for (auto sample = 0; sample < numSamples; ++sample)
				{
					float input = inputGain.processSample(samplesSingleChannel[sample]);

					float output = compressor.processSample(channel, input);

					samplesSingleChannel[sample] = outputGain.processSample(output);
				}
			}
		}

	protected:
		juce::dsp::Compressor<float> compressor;
		juce::dsp::Gain<float> inputGain;
		juce::dsp::Gain<float> outputGain;
		float ratio;
		float threshold;
		float attack;
		float release;
	};
}