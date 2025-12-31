#pragma once

#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/vsttypes.h>
#include <pluginterfaces/vst/ivstprocesscontext.h>
#include <pluginterfaces/base/ftypes.h>

#include "timequeue.h"

#define IS_DEBUG 0

namespace MinMax
{
	inline constexpr int SPECIAL_NOTES = -1;

	enum class NoteEventType : uint8_t
	{
		On,
		Off,
	};

	// NoteEvent:
	//   Scheduler から Processor へ渡される
	//   送信用の MIDI ノートイベント。
	//   状態やロジックは持たない。
	class NoteEvent
	{
	public:
		NoteEventType eventType;

		int channel;
		int noteId;

		// 現在の ProcessBlock 先頭からのサンプルオフセット
		int sampleOffset;

		int pitch;

		// 0.0f – 1.0f 正規化ベロシティ
		float velocity;
	};

	struct IScheduledEventListener
	{
		virtual ~IScheduledEventListener() = default;
		virtual void sendNoteEvent(const NoteEvent& ev) = 0;
	};

	struct EventScheduler
	{
		static EventScheduler& Instance()
		{
			static EventScheduler instance;
			return instance;
		}

		void setListener(IScheduledEventListener* l)
		{
			listener = l;
		}

		double getSamplesPerMs() const { return sampleRate / 1000.0; }
		double getSampleRate() const { return sampleRate; }
		double getTempo() const { return tempo; }
		int32 getTimeSigNumerator() const { return timeSigNumerator; }
		int32 getTimeSigDenominator() const { return timeSigDenominator; }

		void setSampleRate(double value) { sampleRate = value; }

		void process(ProcessData& data)
		{
			numSamples = data.numSamples;
			blockStart = currentSampleTime;
			blockEnd = blockStart + numSamples;

			if (data.processContext != nullptr)
			{
				newState(data.processContext);
			}
		}

		void addNoteOn(uint64 onTime, uint64 offTime, int stringIndex, int pitch, float velocity, int channel = 0)
		{
			auto& q = buffer(stringIndex);

			// ============================================================
			// 1. 同一 onTime ノートの確認
			// ============================================================
			if (ScheduledNote* same = q.findNoteWithOnTime(onTime))
			{
				if (same->isSendNoteOn)
				{
					return;
				}

				// 未発音 → 内容上書き（channel / noteId は維持）
				same->offTime = offTime;
				same->pitch = pitch;
				same->velocity = velocity;
				same->valid = true;

				return;
			}

			// ============================================================
			// 2. 直前ノート（onTime より前で最後）の補正
			// ============================================================
			if (ScheduledNote* prev = q.findLastBefore(onTime))
			{
				if (prev->offTime > onTime)
				{
					uint64 adjustedOff = onTime - 1;

					// underflow 対策
					if (adjustedOff <= currentSampleTime)
					{
						// offTime == currentSampleTime は block 内 NoteOff を保証するため意図的
						prev->offTime = currentSampleTime;
						onTime = currentSampleTime + 1;
					}
					else
					{
						prev->offTime = adjustedOff;
					}
				}
			}

			// ============================================================
			// 3. 新規ノート生成・追加
			// ============================================================
			ScheduledNote note{};
			note.valid = true;
			note.isSendNoteOn = false;
			note.onTime = onTime;
			note.offTime = offTime;
			note.pitch = pitch;
			note.velocity = velocity;
			note.channel = channel;
			note.noteId = getNewNoteId();

			q.push(note); // onTime 昇順で挿入される前提

		}

		void dispatch()
		{
			// dispatch:
			//
			// 役割:
			//   現在の process block（blockStart ～ blockEnd）に基づき、
			//   TimeQueue に格納された ScheduledNote から
			//   NoteOn / NoteOff イベントを Listener に送信する。
			//
			// 前提条件:
			//   - ScheduledNote.onTime < ScheduledNote.offTime が保証されている
			//   - ScheduledNote は onTime 昇順で TimeQueue に格納されている
			//   - addNoteOn で音価補正・衝突解決済み
			//
			// 処理:
			//   - block 内に onTime が含まれる場合、まだ送信していなければ NoteOn を送信
			//   - block 内に offTime が含まれる場合、NoteOff を送信しキューから削除
			//   - 先頭要素が block 外（future）の場合、それ以降も future のため処理を終了
			//   - 同一 block 内で onTime / offTime が同時に存在する場合、NoteOn → NoteOff の順で送信
			for (int i = 0; i < STRING_COUNT; ++i)
			{
				auto& q = stringQueues[i];

				while (!q.empty())
				{
					auto& note = q.current();

					// ノートオン送信条件: block 内かつ未送信
					if (!note.isSendNoteOn && note.onTime >= blockStart && note.onTime < blockEnd)
					{
						int sampleOffset = static_cast<int>(note.onTime - blockStart);
						sendNoteEventCommon(true, sampleOffset, note, note.velocity);
						note.isSendNoteOn = true;
					}

					// ノートオフ送信条件: block 内に offTime がある
					if (note.offTime >= blockStart && note.offTime < blockEnd)
					{
						int sampleOffset = static_cast<int>(note.offTime - blockStart);
						sendNoteEventCommon(false, sampleOffset, note, note.velocity);
						q.eraseCurrent(); // 送信済みノートは削除
					}
					else
					{
						// まだ future ノートなので処理終了
						break;
					}
				}
			}

			// 特殊キューも同様に処理
			{
				auto& q = specialQueue;
				while (!q.empty())
				{
					auto& note = q.current();

					if (!note.isSendNoteOn && note.onTime >= blockStart && note.onTime < blockEnd)
					{
						int sampleOffset = static_cast<int>(note.onTime - blockStart);
						sendNoteEventCommon(true, sampleOffset, note, note.velocity);
						note.isSendNoteOn = true;
					}

					if (note.offTime >= blockStart && note.offTime < blockEnd)
					{
						int sampleOffset = static_cast<int>(note.offTime - blockStart);
						sendNoteEventCommon(false, sampleOffset, note, note.velocity);
						q.eraseCurrent();
					}
					else
					{
						break;
					}
				}
			}

			// ブロック終了後のサンプル時刻更新
			currentSampleTime += numSamples;
		}

		void allNotesOff()
		{
			for (auto& q : stringQueues)
			{
				while (!q.empty())
				{
					sendNoteEventCommon(false, 0, q.current(), 0.0f);
					q.eraseCurrent();
				}
			}

			while (!specialQueue.empty())
			{
				sendNoteEventCommon(false, 0, specialQueue.current(), 0.0f);
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
				currentSampleTime, blockStart, blockEnd, sampleRate, tempo
			);
			return buf;
		}

		bool isPlaying() const { return state & ProcessContext::kPlaying; }
		uint64 getCurrentSampleTime() const { return currentSampleTime; }

	private:
		EventScheduler() = default;
		EventScheduler(const EventScheduler&) = delete;
		EventScheduler& operator=(const EventScheduler&) = delete;
		EventScheduler(EventScheduler&&) = default;
		EventScheduler& operator=(EventScheduler&&) = default;

		uint32 numSamples = 0;
		uint64 currentSampleTime = 0;
		uint64 blockStart = 0;
		uint64 blockEnd = 0;

		IScheduledEventListener* listener = nullptr;
		TimeQueue stringQueues[STRING_COUNT];
		TimeQueue specialQueue;

		TimeQueue& buffer(uint32 number)
		{
			if (number == SPECIAL_NOTES) return specialQueue;
			if (number < STRING_COUNT) return stringQueues[number];
			return specialQueue;
		}

		uint32 noteid = 0;

		uint32 state = 0;
		double sampleRate = 0;
		TSamples projectTimeSamples = 0;
		int64 systemTime = 0;
		TSamples continousTimeSamples = 0;
		TQuarterNotes projectTimeMusic = 0;
		TQuarterNotes barPositionMusic = 0;
		TQuarterNotes cycleStartMusic = 0;
		TQuarterNotes cycleEndMusic = 0;
		double tempo = 0;
		int32 timeSigNumerator = 0;
		int32 timeSigDenominator = 0;
		Vst::Chord chord{};
		int32 smpteOffsetSubframes = 0;
		FrameRate frameRate{};
		int32 samplesToNextClock = 0;

		void newState(ProcessContext* ctx)
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

			if (systemTime != ctx->systemTime) systemTime = ctx->systemTime;

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

			if (chord.keyNote != ctx->chord.keyNote ||
				chord.chordMask != ctx->chord.chordMask ||
				chord.rootNote != ctx->chord.rootNote)
			{
				chord.keyNote = ctx->chord.keyNote;
				chord.chordMask = ctx->chord.chordMask;
				chord.rootNote = ctx->chord.rootNote;
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

		void sendNoteEventCommon(bool on, uint32 sampleOffset, const ScheduledNote& note, float velocity)
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
