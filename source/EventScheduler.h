//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <cstdio>
#include <pluginterfaces/base/ftypes.h>
#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivstprocesscontext.h>
#include <pluginterfaces/vst/vsttypes.h>
#include <string>

#include "timequeue.h"

namespace MinMax
{
	inline constexpr int SPECIAL_NOTES = -1;

	enum class NoteEventType : uint8_t
	{
		On,
		Off,
	};

	struct NoteEvent
	{
		NoteEventType eventType;
		int channel;
		int noteId;
		int sampleOffset;
		int pitch;
		float velocity;
	};

	struct IScheduledEventListener
	{
		virtual ~IScheduledEventListener() = default;
		virtual void sendNoteEvent(const NoteEvent& ev) = 0;
		virtual void chordChanged(const Steinberg::Vst::Chord chord) = 0;
	};

	struct EventScheduler
	{
		EventScheduler() = default;

		void setListener(IScheduledEventListener* l)
		{
			listener = l;
		}

		double getSamplesPerMs() const { return sampleRate / 1000.0; }
		double getSampleRate() const { return sampleRate; }
		double getTempo() const { return tempo; }
		Steinberg::int32 getTimeSigNumerator() const { return timeSigNumerator; }
		Steinberg::int32 getTimeSigDenominator() const { return timeSigDenominator; }

		void setSampleRate(double value) { sampleRate = value; }

		void process(Steinberg::Vst::ProcessData& data)
		{
			numSamples = data.numSamples;

			if (data.processContext != nullptr)
			{
				newState(data.processContext);
			}
		}

		void addNoteOn(Steinberg::uint64 onTime, Steinberg::uint64 offTime, int stringIndex, int pitch, float velocity, int channel = 0)
		{
			auto& q = buffer(stringIndex);

			if (ScheduledNote* same = q.findNoteWithOnTime(onTime))
			{
				if (same->isSendNoteOn)
				{
					return;
				}

				same->offTime = offTime;
				same->pitch = pitch;
				same->velocity = velocity;
				same->valid = true;

				return;
			}

			if (ScheduledNote* prev = q.findLastBefore(onTime))
			{
				if (prev->offTime > onTime)
				{
					if (onTime == currentSampleTime)
					{
						// 自身のオンがブロック頭の場合は直前のオフを調整すると過去ブロック
						// になってしまうため自身のオンを１サンプル後へずらす
						prev->offTime = currentSampleTime;
						onTime = onTime + 1;
					}
					else
					{
						// 直前のオフを自身のオンの１サンプル前へ移動
						prev->offTime = onTime - 1;
					}

					if (offTime <= onTime)
					{
						// 調整の結果、自身の音価が１サンプル以下にならないようにする
						offTime = onTime + 1;
					}

					if (isBlockAdust && prev->pitch == pitch)
					{
						// 同じブロックにオン・オフが混在できない音源対応
						onTime += numSamples;
						offTime += numSamples;
					}
				}
			}

			ScheduledNote note{};

			note.valid = true;
			note.isSendNoteOn = false;
			note.onTime = onTime;
			note.offTime = offTime;
			note.pitch = pitch;
			note.velocity = velocity;
			note.channel = channel;
			note.noteId = getNewNoteId();

			q.push(note);
		}

		void dispatch()
		{
			for (int i = SPECIAL_NOTES; i < STRING_COUNT; ++i)
			{
				auto& q = buffer(i);

				while (!q.empty())
				{
					auto& note = q.current();

					if (isWithinRange(note.onTime))
					{
						outQueue.push(note);
						note.isSendNoteOn = true;
					}

					if (isWithinRange(note.offTime))
					{
						outQueue.push(note);
						q.eraseCurrent();
					}
					else
					{
						break;
					}
				}
			}

			while (!outQueue.empty())
			{
				auto& note = outQueue.current();
				int sampleOffset = static_cast<int>((note.isSendNoteOn ? note.offTime : note.onTime) - currentSampleTime);
				onSendNoteEvent(!note.isSendNoteOn, sampleOffset, note, note.velocity);
				outQueue.eraseCurrent();
			}

			currentSampleTime += numSamples;
		}

		void allNotesOff()
		{
			for (auto& q : stringQueues)
			{
				while (!q.empty())
				{
					auto& note = q.current();
					onSendNoteEvent(false, 0, note, note.velocity);
					q.eraseCurrent();
				}
			}

			while (!specialQueue.empty())
			{
				auto& note = specialQueue.current();
				onSendNoteEvent(false, 0, note, note.velocity);
				specialQueue.eraseCurrent();
			}
		}

		void reset()
		{
			allNotesOff();
			currentSampleTime = 0;
			noteid = 0;
		}

		std::string toString() const
		{
			char buf[256];
			std::snprintf(
				buf, sizeof(buf),
				"EventScheduler{cur=%llu block=[%llu-%llu) sr=%.1f tempo=%.1f}",
				currentSampleTime, currentSampleTime, currentSampleTime + numSamples - 1, sampleRate, tempo
			);
			return buf;
		}

		bool isPlaying() const
		{
			return state & Steinberg::Vst::ProcessContext::kPlaying;
		}

		Steinberg::uint64 getCurrentSampleTime() const
		{
			return currentSampleTime;
		}

		void setNeedSampleblockAdust(bool value)
		{
			isBlockAdust = value;
		}

	private:

		bool isBlockAdust = false;

		Steinberg::uint32 numSamples = 0;

		Steinberg::uint64 currentSampleTime = 0;

		IScheduledEventListener* listener = nullptr;

		TimeQueue stringQueues[STRING_COUNT];
		TimeQueue specialQueue;

		TimeQueue outQueue;

		Steinberg::uint32 noteid = 0;

		// Context From DAW
		Steinberg::uint32 state = 0;
		double sampleRate = 0;
		Steinberg::Vst::TSamples projectTimeSamples = 0;
		Steinberg::int64 systemTime = 0;
		Steinberg::Vst::TSamples continousTimeSamples = 0;
		Steinberg::Vst::TQuarterNotes projectTimeMusic = 0;
		Steinberg::Vst::TQuarterNotes barPositionMusic = 0;
		Steinberg::Vst::TQuarterNotes cycleStartMusic = 0;
		Steinberg::Vst::TQuarterNotes cycleEndMusic = 0;
		double tempo = 0;
		Steinberg::int32 timeSigNumerator = 0;
		Steinberg::int32 timeSigDenominator = 0;
		Steinberg::Vst::Chord chord{};
		Steinberg::int32 smpteOffsetSubframes = 0;
		Steinberg::Vst::FrameRate frameRate{};
		Steinberg::int32 samplesToNextClock = 0;

		void newState(Steinberg::Vst::ProcessContext* ctx)
		{
			if (state != ctx->state)
			{
				state = ctx->state;
			}

			if (ctx->sampleRate > 0.0 && sampleRate != ctx->sampleRate)
			{
				sampleRate = ctx->sampleRate;
			}

			if (projectTimeSamples != ctx->projectTimeSamples)
			{
				projectTimeSamples = ctx->projectTimeSamples;
			}

			if (systemTime != ctx->systemTime)
			{
				systemTime = ctx->systemTime;
			}

			if (continousTimeSamples != ctx->continousTimeSamples)
			{
				continousTimeSamples = ctx->continousTimeSamples;
			}

			if (projectTimeMusic != ctx->projectTimeMusic)
			{
				projectTimeMusic = ctx->projectTimeMusic;
			}

			if (barPositionMusic != ctx->barPositionMusic)
			{
				barPositionMusic = ctx->barPositionMusic;
			}

			if (cycleStartMusic != ctx->cycleStartMusic)
			{
				cycleStartMusic = ctx->cycleStartMusic;
			}

			if (cycleEndMusic != ctx->cycleEndMusic)
			{
				cycleEndMusic = ctx->cycleEndMusic;
			}

			auto prevTempo = tempo;
			auto value = ctx->tempo <= 0 ? 120 : ctx->tempo;
			if (prevTempo != value)
			{
				tempo = value;
			}

			auto prevNum = timeSigNumerator;
			auto num = ctx->timeSigNumerator > 0 ? ctx->timeSigNumerator : 4;
			if (prevNum != num)
			{
				timeSigNumerator = num;
			}

			auto prevDen = timeSigDenominator;
			auto den = ctx->timeSigDenominator > 0 ? ctx->timeSigDenominator : 4;
			if (prevDen != den)
			{
				timeSigDenominator = den;
			}

			if (
				chord.keyNote != ctx->chord.keyNote ||
				chord.chordMask != ctx->chord.chordMask ||
				chord.rootNote != ctx->chord.rootNote
				)
			{
				chord.keyNote = ctx->chord.keyNote;
				chord.chordMask = ctx->chord.chordMask;
				chord.rootNote = ctx->chord.rootNote;
				if (listener) listener->chordChanged(chord);
			}

			if (smpteOffsetSubframes != ctx->smpteOffsetSubframes)
			{
				smpteOffsetSubframes = ctx->smpteOffsetSubframes;
			}

			if (frameRate.flags != ctx->frameRate.flags ||
				frameRate.framesPerSecond != ctx->frameRate.framesPerSecond)
			{
				frameRate.flags = ctx->frameRate.flags;
				frameRate.framesPerSecond = ctx->frameRate.framesPerSecond;
			}

			if (samplesToNextClock != ctx->samplesToNextClock)
			{
				samplesToNextClock = ctx->samplesToNextClock;
			}
		}

		TimeQueue& buffer(Steinberg::uint32 number)
		{
			if (number == SPECIAL_NOTES) return specialQueue;
			if (number < STRING_COUNT) return stringQueues[number];
			return specialQueue;
		}

		bool isWithinRange(Steinberg::uint64 time)
		{
			return time >= currentSampleTime && time < currentSampleTime + numSamples;
		}

		void onSendNoteEvent(bool on, Steinberg::uint32 sampleOffset, const ScheduledNote& note, float velocity)
		{
			NoteEvent e{};

			e.eventType = on ? NoteEventType::On : NoteEventType::Off;
			e.channel = note.channel;
			e.noteId = note.noteId;
			e.sampleOffset = sampleOffset;
			e.pitch = note.pitch;
			e.velocity = velocity;

			if (listener) listener->sendNoteEvent(e);
		}

		int getNewNoteId()
		{
			noteid++;
			return noteid;
		}
	};
}
