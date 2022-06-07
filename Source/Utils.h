#pragma once
#include <JuceHeader.h>

namespace utils
{
	inline float linearInterpolation(float* bufferChannel, float readPos, int bufferSize) noexcept
	{
		const auto readPosFloor = std::floor(readPos);
		const auto floor = static_cast<int>(readPosFloor);
		const auto ceiling = (floor + 1) % bufferSize;
		const auto fraction = readPos - readPosFloor;
		return bufferChannel[floor] + fraction * (bufferChannel[ceiling] - bufferChannel[floor]);
	}

	// Many thanks to 'Beats basteln :3' on youtube!
	struct Smooth
	{
		static constexpr float tau = 6.28318530718f;
		static constexpr float e = 2.71828182846f;

		static void makeFromDecayInSamples(Smooth& s, float d) noexcept
		{
			const auto x = std::pow(e, -1.f / d);
			s.setX(x);
		}
		static void makeFromDecayInSecs(Smooth& s, float d, float Fs) noexcept
		{
			makeFromDecayInSamples(s, d * Fs);
		}
		static void makeFromDecayInFc(Smooth& s, float fc) noexcept
		{
			const auto x = std::pow(e, -tau * fc);
			s.setX(x);
		}
		static void makeFromDecayInHz(Smooth& s, float d, float Fs) noexcept
		{
			makeFromDecayInFc(s, d / Fs);
		}
		static void makeFromDecayInMs(Smooth& s, float d, float Fs) noexcept
		{
			makeFromDecayInSamples(s, d * Fs * .001f);
		}

		Smooth(const bool _snap = true, const float startVal = 0.f) :
			a0(1.f),
			b1(0.f),
			y1(startVal),
			eps(0.f),
			snap(_snap)
		{}
		void reset()
		{
			a0 = 1.f;
			b1 = 0.f;
			y1 = 0.f;
			eps = 0.f;
		}
		void setX(float x) noexcept
		{
			a0 = 1.f - x;
			b1 = x;
			eps = a0 * 1.5f;
		}
		void operator()(float* buffer, float val, int numSamples) noexcept
		{
			if (buffer[0] == val)
				return juce::FloatVectorOperations::fill(buffer, val, numSamples);
			for (auto s = 0; s < numSamples; ++s)
				buffer[s] = processSample(val);
		}
		void operator()(float* buffer, int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
				buffer[s] = processSample(buffer[s]);
		}
		float operator()(float sample) noexcept
		{
			return processSample(sample);
		}
	protected:
		float a0, b1, y1, eps;
		const bool snap;

		float processSample(float x0) noexcept
		{
			if (snap && std::abs(y1 - x0) < eps)
				y1 = x0;
			else
				y1 = x0 * a0 + y1 * b1;
			return y1;
		}
	};
}