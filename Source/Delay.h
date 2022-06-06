#pragma once
#include <array>
#include <vector>
#include <JuceHeader.h>

namespace interpolation
{
	inline float linearInterpolation(float* bufferChannel, float readPos, int bufferSize) noexcept
	{
		const auto readPosFloor = std::floor(readPos);
		const auto floor = static_cast<int>(readPosFloor);
		const auto ceiling = (floor + 1) % bufferSize;
		const auto fraction = readPos - readPosFloor;
		return bufferChannel[floor] + fraction * (bufferChannel[ceiling] - bufferChannel[floor]);
	}
}

namespace dsp
{
	template<typename T>
	T msToSamples(T sampleRate, T lengthInMs) noexcept
	{
		return sampleRate * lengthInMs * static_cast<T>(.001);
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

	struct WriteHead
	{
		WriteHead() :
			writeHeadBuffer(),
			writeHeadIndex(0)
		{}

		void prepare(int blockSize, int _ringBufferSize)
		{
			writeHeadBuffer.resize(blockSize);
			writeHeadIndex = 0;
			ringBufferSize = _ringBufferSize;
		}

		void operator()(int numSamples) noexcept
		{
			for (auto sample = 0; sample < numSamples; ++sample)
			{
				/*
				* writeHead always represents most current sample
				* sample furthest away from the writeHead index represents oldest sample
				* for each audio sample we increment its index by one
				* and also make sure it never exceeds the delaybuffer's length
				*/
				writeHeadIndex = (writeHeadIndex + 1) % ringBufferSize;
				writeHeadBuffer[sample] = writeHeadIndex;
			}
		}

		int operator[](int i) const noexcept { return writeHeadBuffer[i]; }

	protected:
		std::vector<float> writeHeadBuffer;
		int writeHeadIndex;
		int ringBufferSize;
	};

	struct Delay
	{
		Delay() :
			sampleRate(0.),
			ringBuffer(),
			parameterBufferLength(),
			delayTimeSmooth(0.f),
			writeHead(),
			delayLength(0.f),
			ringBufferSize(51)
		{}

		void prepare(double _sampleRate, int blockSize, double bufferLengthInMs)
		{
			sampleRate = _sampleRate;

			const auto lengthInSamples = static_cast<int>(msToSamples(_sampleRate, bufferLengthInMs));
			for (auto& channel : ringBuffer)
				channel.resize(lengthInSamples, 0.f);

			ringBufferSize = lengthInSamples;

			writeHead.prepare(blockSize, ringBufferSize);
			parameterBufferLength.resize(blockSize);
			Smooth::makeFromDecayInMs(delayTimeSmooth, 2000.f, sampleRate);
		}

		void updateParameters(float _delayLength)
		{
			delayLength = msToSamples(static_cast<float>(sampleRate), _delayLength);
		}

		void processBlock(float** samples, int numChannels, int numSamples)
		{
			writeHead(numSamples);

			delayTimeSmooth(parameterBufferLength.data(), delayLength, numSamples);

			for (auto channel = 0; channel < numChannels; ++channel)
			{
				auto samplesSingleChannel = samples[channel];
				auto ringBufferSingleChannel = ringBuffer[channel].data();

				for (auto sample = 0; sample < numSamples; ++sample)
				{
					/*
					* store current sample from audiobuffer in the ringbuffer
					* this also makes sure to always overwrite the oldest sample from the ringbuffer
					* because it will take the writehead exactly the delaybuffer's length for a full loop
					*/
					const auto writePos = writeHead[sample];
					ringBufferSingleChannel[writePos] = samplesSingleChannel[sample];

					/*
					* if the writePosition is at the beginning of the buffer
					* and the delayLength is long enough to cause the readPosition fall of the beginning
					* then the readPos should be increased by the length of the buffer
					* so that the readPos is placed near the end of the buffer
					*/ 
					auto readPos = static_cast<float>(writePos) - parameterBufferLength[sample];
					if (readPos < 0.f) readPos += ringBufferSize;
					samplesSingleChannel[sample] = interpolation::linearInterpolation(ringBufferSingleChannel, readPos, ringBufferSize);
				}
			}
		}

	protected:
		double sampleRate;
		std::array<std::vector<float>, 2> ringBuffer;
		std::vector<float> parameterBufferLength;
		Smooth delayTimeSmooth;
		WriteHead writeHead;
		float delayLength;
		int ringBufferSize;
	};
}