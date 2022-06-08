#pragma once

namespace dsp {
	struct Compressor
	{
		Compressor() :
			shouldCompress(false),
			ratio(1.f),
			threshold(0.f),
			attack(20.f),
			release(100.f)
		{}

		void prepare(double sampleRate, int blockSize)
		{
		}

		void updateParameters(float _ratio, float _threshold, float _attack, float _release)
		{
			ratio = _ratio;
			threshold = _threshold;
			attack = _attack;
			release = _release;
		}

		void processBlock()
		{

		}

	protected:
		bool shouldCompress;
		float ratio;
		float threshold;
		float attack;
		float release;
	};
}