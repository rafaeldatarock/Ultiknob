#pragma once
#include <array>
#include <vector>

namespace dsp
{
	template<typename T>
	T msToSamples(T sampleRate, T lengthInMs) noexcept
	{
		return sampleRate * lengthInMs * static_cast<T>(.001);
	}

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
			ringBuffer(),
			writeHead(),
			delayLength(0.f),
			ringBufferSize(1001)
		{}

		void prepare(double _sampleRate, int blockSize, double bufferLengthInMs)
		{
			sampleRate = _sampleRate;

			const auto lengthInSamples = static_cast<int>(msToSamples(_sampleRate, bufferLengthInMs));
			for (auto& channel : ringBuffer)
				channel.resize(lengthInSamples, 0.f);

			ringBufferSize = lengthInSamples;

			writeHead.prepare(blockSize, ringBufferSize);
		}

		void updateParameters(float _delayLength)
		{
			delayLength = msToSamples(static_cast<float>(sampleRate), _delayLength);
		}

		void processBlock(float** samples, int numChannels, int numSamples)
		{
			writeHead(numSamples);

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
					auto readPos = static_cast<float>(writePos) - delayLength;
					if (readPos < 0.f) readPos += ringBufferSize;
					samplesSingleChannel[sample] += ringBufferSingleChannel[static_cast<int>(readPos)];
				}
			}
		}

	protected:
		double sampleRate;
		std::array<std::vector<float>, 2> ringBuffer;
		WriteHead writeHead;
		float delayLength;
		int ringBufferSize;
	};
}