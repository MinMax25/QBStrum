//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------

#include <pluginterfaces/vst/ivstparameterchanges.h>
#include <vstgui/vstgui_uidescription.h>
#include <pluginterfaces/base/ibstream.h>

#include "plugcids.h"
#include "plugprocessor.h"
#include "plugdefine.h"
#include "parameterframework.h"
#include "myparameters.h"

#include "debug_log.h"

namespace MinMax
{
#pragma region Implements

	void MyVSTProcessor::sendNoteEvent(const NoteEvent& e)
	{
		if (!processData) return;
		if (!processData->outputEvents) return;

		Event event{};

		event.flags = 0; //Event::kIsLive;
		event.busIndex = 0;
		event.sampleOffset = e.sampleOffset;

		if (e.eventType == NoteEventType::On)
		{
			event.type = Event::kNoteOnEvent;
			event.noteOn.channel = e.channel;
			event.noteOn.noteId = e.noteId;
			event.noteOn.pitch = e.pitch;
			event.noteOn.velocity = e.velocity;

#if DEBUG
			DLogWrite(EventScheduler::Instance().toString().c_str());
			DLogWriteLine(
				"[NoteOn ] flg=%d ch=%d noteId=%d pitch%d vel=%.2f offset=%d",
				event.flags,
				event.noteOn.channel,
				event.noteOn.noteId,
				event.noteOn.pitch,
				event.noteOn.velocity,
				event.sampleOffset
			);
#endif
		}
		else
		{
			event.type = Event::kNoteOffEvent;
			event.noteOff.channel = e.channel;
			event.noteOff.noteId = e.noteId;
			event.noteOff.pitch = e.pitch;
			event.noteOff.velocity = 0.0f;

#if DEBUG
			DLogWrite(EventScheduler::Instance().toString().c_str());
			DLogWriteLine(
				"[NoteOff] flg=%d ch=%d noteId=%d pitch=%d vel=%.2f offset=%d",
				event.flags,
				event.noteOff.channel,
				event.noteOff.noteId,
				event.noteOff.pitch,
				event.noteOff.velocity,
				event.sampleOffset
			);
#endif
		}

		processData->outputEvents->addEvent(event);
	}

#pragma endregion

#pragma region ctor etc.

	MyVSTProcessor::MyVSTProcessor()
	{
		processData = nullptr;

		setControllerClass(kMyVSTControllerUID);

		initParameters();
	}

	MyVSTProcessor::~MyVSTProcessor()
	{
	}

	tresult PLUGIN_API MyVSTProcessor::initialize(FUnknown* context)
	{
		tresult result = AudioEffect::initialize(context);
		if (result != kResultOk) return result;

		addAudioOutput(STR16("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);
		addEventInput(STR16("Event In"), 1);
		addEventOutput(STR16("Event Out"), 16);

		// init parameter strage
		prm.initialize(paramTable);

		return kResultOk;
	}

	tresult PLUGIN_API MyVSTProcessor::terminate()
	{
		return AudioEffect::terminate();
	}

	tresult PLUGIN_API MyVSTProcessor::setActive(TBool state)
	{
		if (state)
		{
			scheduler.setListener(this);
		}
		else
		{
			scheduler.setListener(nullptr);
		}
		return AudioEffect::setActive(state);
	}

	tresult PLUGIN_API MyVSTProcessor::setupProcessing(Vst::ProcessSetup& newSetup)
	{
		scheduler.setSampleRate(newSetup.sampleRate);
		return AudioEffect::setupProcessing(newSetup);
	}

	tresult PLUGIN_API MyVSTProcessor::canProcessSampleSize(int32 symbolicSampleSize)
	{
		if (symbolicSampleSize == Vst::kSample32) return kResultTrue;
		return kResultFalse;
	}

#pragma region

#pragma region State

	tresult PLUGIN_API MyVSTProcessor::setState(IBStream* state)
	{
		if (!state) return kInvalidArgument;

		for (const auto& def : paramTable)
		{
			double plain = 0.0;
			if (state->read(&plain, sizeof(double), nullptr) != kResultOk) return kResultFalse;
			prm.set(def.tag, plain);
		}

		return kResultOk;
	}

	tresult PLUGIN_API MyVSTProcessor::getState(IBStream* state)
	{
		if (!state) return kInvalidArgument;

		for (const auto& def : paramTable)
		{
			double plain = prm.get(def.tag);
			if (state->write(&plain, sizeof(double), nullptr) != kResultOk) return kResultFalse;
		}

		return kResultOk;
	}

#pragma region process

	tresult PLUGIN_API MyVSTProcessor::process(Vst::ProcessData& data)
	{
		processData = &data;

		scheduler.process(data);

		// プロセスコンテキスト処理
		processContext();

		// パラメータ処理
		processParameter();

		// DAWからの受信イベント処理
		processEvent();

		// UIからの受信イベント処理（サウンドチェックトリガー etc.）
		processInnerEvent();

		// オーディオ処理
		processAudio(data);

		// 予約イベント処理
		scheduler.dispatch();

		processData = nullptr;

		return kResultOk;
	}

	void PLUGIN_API MyVSTProcessor::processContext()
	{
		if (!processData) return;

		// Play or Stop
		if (isPlaying == false && scheduler.isPlaying() == true)
		{
			isPlaying = true;
		}
		else if (isPlaying == true && scheduler.isPlaying() == false)
		{
			isPlaying = false;
			scheduler.reset();
		}
	}

	void PLUGIN_API MyVSTProcessor::processParameter()
	{
		if (!processData) return;
		if (processData->inputParameterChanges == NULL) return;

		int32 paramChangeCount = processData->inputParameterChanges->getParameterCount();

		for (int32 i = 0; i < paramChangeCount; i++)
		{
			IParamValueQueue* queue = processData->inputParameterChanges->getParameterData(i);
			if (queue == NULL) continue;

			ParamID tag = queue->getParameterId();

			int32 valueChangeCount = queue->getPointCount();
			int32 sampleOffset;
			ParamValue value;

			if (!(queue->getPoint(valueChangeCount - 1, sampleOffset, value) == kResultTrue)) continue;

			// パラメータ値をキャッシュ
			prm.setNormalized(tag, value);

			switch (tag)
			{
			case PARAM::SELECTED_ARTICULATION:
			{
				if (prm.isChanged(tag))
				{
					articulationChanged(sampleOffset);
				}
				break;
			}
			case PARAM::CHORD_MSB:
			{
				ParamValue num = prm.get(PARAM::CHORD_MSB) * 128 + prm.get(PARAM::CHORD_LSB);
				notifyChordNumberChanged(num);
				prm.set(PARAM::CHORD_NUM, num);
				break;
			}
			case PARAM::NEED_SAMPLEBLOCK_ADUST:
			{
				bool state = prm.get(PARAM::NEED_SAMPLEBLOCK_ADUST) > 0.5;
				EventScheduler::Instance().setNeedSampleblockAdust(state);
				break;
			}
			}
		}
	}

	void PLUGIN_API MyVSTProcessor::articulationChanged(int sampleOffset)
	{
		constexpr int SPECIAL_NOTES_SAMPLES = 3;

		double newArtic = prm.get(PARAM::SELECTED_ARTICULATION);

		std::array<const ParamDef*, PARAM_ARTICULATION_COUNT> artics;
		size_t articCount = 0;
		getArticulationParams(artics, articCount);

		int keySW = 0;

		for (size_t i = 0; i < articCount; ++i)
		{
			const ParamDef* def = artics[i];

			if (i == newArtic)
			{
				keySW = prm.get(def->tag);
				break;
			}
		}
		if (keySW == 0) return;

		uint64 onTime = scheduler.getCurrentSampleTime() + sampleOffset;
		uint64 offTime = onTime + SPECIAL_NOTES_SAMPLES;

		for (int i = 0; i < (prm.get(PARAM::CHANNEL_SEPALATE) ? STRING_COUNT : 1); i++)
		{
			scheduler.addNoteOn(onTime, offTime, SPECIAL_NOTES, keySW, 127, i);
		}

		prm.clearChangedFlags(PARAM::SELECTED_ARTICULATION);
	}

	void PLUGIN_API MyVSTProcessor::processEvent()
	{
		if (!processData) return;
		if (processData->inputEvents == NULL) return;

		// イベント処理
		Event event;

		for (int32 i = 0; i < processData->inputEvents->getEventCount(); i++)
		{
			if (processData->inputEvents->getEvent(i, event) != kResultOk) continue;

			// ピッチからパラメータＩＤ取得
			if (event.type == Event::kNoteOnEvent)
			{
				ParamID tag = getParamIdByPitch(event);
				if (tag > 0)
				{
					routingProcess(tag, event);
				}
			}
		}
	}

	void PLUGIN_API MyVSTProcessor::processInnerEvent()
	{
		if (!processData) return;

		// イベント処理
		Event event{};

		while (InnerEvents.pop(event))
		{
			// ピッチからパラメータＩＤ取得
			ParamID paramid = getParamIdByPitch(event);
			if (paramid < 0) continue;

			switch (event.type)
			{
			case Event::kNoteOnEvent:
				routingProcess(paramid, event);
				break;

			case Event::kNoteOffEvent:
				break;

			default:
				break;
			}
		}
	}

	void PLUGIN_API MyVSTProcessor::routingProcess(ParamID paramid, Event event)
	{
		// ストラム処理振分け
		switch (paramid)
		{
		case PARAM::ALL_NOTES_OFF:
			trigAllNotesOff();
			break;
		case PARAM::BRUSH_UP:
			trigBrush(event, false);
			break;
		case PARAM::BRUSH_DOWN:
			trigBrush(event, true);
			break;
		case PARAM::UP_HIGH:
			trigStrum(event, true, false, prm.get(PARAM::STRINGS_UP_HIGH));
			break;
		case PARAM::UP:
			trigStrum(event, true, false, STRING_COUNT);
			break;
		case PARAM::DOWN_HIGH:
			trigStrum(event, true, true, prm.get(PARAM::STRINGS_DOWN_HIGH));
			break;
		case PARAM::DOWN:
			trigStrum(event, false, true, STRING_COUNT);
			break;
		case PARAM::DOWN_LOW:
			trigStrum(event, false, true, prm.get(PARAM::STRINGS_DOWN_LOW));
			break;
		case PARAM::MUTE_1:
			trigMute(PARAM::MUTE_1, event);
			break;
		case PARAM::MUTE_2:
			trigMute(PARAM::MUTE_2, event);
			break;
		case PARAM::ARPEGGIO_1:
			trigArpeggio(0, event);
			break;
		case PARAM::ARPEGGIO_2:
			trigArpeggio(1, event);
			break;
		case PARAM::ARPEGGIO_3:
			trigArpeggio(2, event);
			break;
		case PARAM::ARPEGGIO_4:
			trigArpeggio(3, event);
			break;
		case PARAM::ARPEGGIO_5:
			trigArpeggio(4, event);
			break;
		case PARAM::ARPEGGIO_6:
			trigArpeggio(5, event);
			break;
		default:
			break;
		}
	}

	void MyVSTProcessor::trigAllNotesOff()
	{
		scheduler.allNotesOff();
	}

	void MyVSTProcessor::trigBrush(Event event, bool isDown)
	{
		const int BRUSH_PER_MS = 2;
		const float BRUSH_DECAY = 0.98f;

		// 現在鳴っている音をすべて消す
		scheduler.allNotesOff();

		trigFretNoise(event);

		// コード情報を取得する
		auto& fretpos = chordMap.getChordVoicing(prm.get(PARAM::CHORD_NUM));
		auto& strnum = getTargetStrings(fretpos, false, isDown, STRING_COUNT);

		double samplesPerMs = scheduler.getSamplesPerMs();
		uint64 baseOnTime = scheduler.getCurrentSampleTime() + event.sampleOffset;

		float baseVelocity = std::clamp(event.noteOn.velocity, 0.0f, 1.0f);

		// 発音弦のカウンタ
		int strcnt = 0;

		// 音価はms指定のパラメータ
		uint64 offTime = baseOnTime + samplesPerMs * prm.get(PARAM::BRUSH_TIME); // brushTime;

		for (size_t s = 0; s < strnum.size; s++)
		{
			int i = strnum.data[s];

			uint64 offsetSamples = samplesPerMs * (BRUSH_PER_MS * strcnt);

			uint64 onTime = baseOnTime + offsetSamples;

			int pitch = chordMap.getTunings().data[i] + fretpos.data[i] + prm.get(PARAM::TRANSPOSE);
			float velocity = baseVelocity * std::pow(BRUSH_DECAY, strcnt);

			int channel = prm.get(PARAM::CHANNEL_SEPALATE) ? i % 16 : 0;
			scheduler.addNoteOn(onTime, offTime, i, pitch, velocity, channel);

			strcnt++;
		}
	}

	void MyVSTProcessor::trigStrum(Event event, bool isAbove, bool isDown, int maxStrings)
	{
		trigFretNoise(event);

		// コード情報を取得する
		auto& fretpos = chordMap.getChordVoicing(prm.get(PARAM::CHORD_NUM));
		auto& strnum = getTargetStrings(fretpos, isAbove, isDown, maxStrings);
		
		uint64 baseOnTime = scheduler.getCurrentSampleTime() + event.sampleOffset;

		double samplesPerMs = scheduler.getSamplesPerMs();

		float baseVelocity = std::clamp(event.noteOn.velocity, 0.0f, 1.0f);

		// 音価はビート数指定のパラメータ
		double samplesPerBeat = scheduler.getSampleRate() * 60.0 / scheduler.getTempo();
		uint64 lengthSamples = static_cast<uint64>(std::lround(samplesPerBeat * prm.get(PARAM::STRUM_LENGTH)));
		uint64 offTime = baseOnTime + lengthSamples;

		// 発音弦のカウンタ
		int strcnt = 0;

		for (size_t s = 0; s < strnum.size; s++)
		{
			int i = strnum.data[s];

			double delayMs =
				(strnum.size > 1)
				? (prm.get(PARAM::STRUM_SPEED) / double(STRING_COUNT)) * strcnt
				: 0.0;

			uint64 onTime = baseOnTime + static_cast<uint64>(delayMs * samplesPerMs);

			int pitch = chordMap.getTunings().data[i] + fretpos.data[i] + prm.get(PARAM::TRANSPOSE);
			float velocity = baseVelocity * std::pow(prm.get(PARAM::STRUM_DECAY) / 100.0f, strcnt);

			int channel = prm.get(PARAM::CHANNEL_SEPALATE) ? i % 16 : 0;
			scheduler.addNoteOn(onTime, offTime, i, pitch, velocity, channel);

			++strcnt;
		}
	}

	void MyVSTProcessor::trigMute(PARAM trigger, Event event)
	{
		const double NOTE_LENGTH = 40.0;

		// 現在鳴っている音をすべて消す
		scheduler.allNotesOff();

		trigFretNoise(event);

		// ミュート用ノート選択
		int muteNote =
			(trigger == PARAM::MUTE_1)
			? prm.get(PARAM::MUTE_NOTE_1)
			: prm.get(PARAM::MUTE_NOTE_2);

		float velocity = std::clamp(event.noteOn.velocity, 0.01f, 1.0f);

		// 音価は固定
		uint64 onTime = scheduler.getCurrentSampleTime() + event.sampleOffset;

		uint64 offTime = onTime + static_cast<uint64>(NOTE_LENGTH * scheduler.getSamplesPerMs());

		scheduler.addNoteOn(onTime, offTime, 0, muteNote, velocity, prm.get(PARAM::MUTE_CHANNEL) - 1);
	}

	void MyVSTProcessor::trigArpeggio(int stringindex, Event event)
	{
		trigFretNoise(event);

		auto& fretpos = chordMap.getChordVoicing(prm.get(PARAM::CHORD_NUM));

		// 弦が鳴らせない場合
		if (stringindex < 0 || stringindex >= STRING_COUNT || fretpos.data[stringindex] < 0)
		{
			return;
		}

		// 音価はビート数指定のパラメータ
		uint64 onTime = scheduler.getCurrentSampleTime() + event.sampleOffset;

		double samplesPerBeat = scheduler.getSampleRate() * 60.0 / scheduler.getTempo();
		uint64 lengthSamples = static_cast<uint64>(std::lround(samplesPerBeat * prm.get(PARAM::ARP_LENGTH)));
		uint64 offTime = onTime + lengthSamples;

		int pitch = chordMap.getTunings().data[stringindex] + fretpos.data[stringindex] + prm.get(PARAM::TRANSPOSE);
		float velocity = std::clamp(event.noteOn.velocity, 0.0f, 1.0f);

		int channel = prm.get(PARAM::CHANNEL_SEPALATE) ? stringindex % 16 : 0;
		scheduler.addNoteOn(onTime, offTime, stringindex, pitch, velocity, channel);
	}

	void MyVSTProcessor::trigFretNoise(Event event)
	{
		constexpr double FRET_NOISE_LENGTH = 40.0;

		float distance = 0.0f;

		// コード変更
		if (lastChordNum != prm.get(PARAM::CHORD_NUM))
		{
			distance =
				std::abs(
					chordMap.getPositionAverage(lastChordNum) -
					chordMap.getPositionAverage(prm.get(PARAM::CHORD_NUM)));

			// コード変更時は全消音
			trigAllNotesOff();

			// 現在のコードを記録
			lastChordNum = prm.get(PARAM::CHORD_NUM);
		}

		if (prm.get(PARAM::FNOISE_NOTE_NEAR) == 0 && prm.get(PARAM::FNOISE_NOTE_FAR) == 0) return;

		int pitch = 0;

		if (distance < prm.get(PARAM::FNOISE_NOTE_NEAR)) return;

		if (distance < prm.get(PARAM::FNOISE_NOTE_FAR))
		{
			pitch =
				prm.get(PARAM::FNOISE_NOTE_NEAR) == 0 
				? prm.get(PARAM::FNOISE_NOTE_FAR) 
				: prm.get(PARAM::FNOISE_NOTE_NEAR);
		}
		else
		{
			pitch = 
				prm.get(PARAM::FNOISE_NOTE_FAR) == 0 
				? prm.get(PARAM::FNOISE_NOTE_NEAR) 
				: prm.get(PARAM::FNOISE_NOTE_FAR);
		}

		if (pitch == 0) return;

		uint64 onTime = scheduler.getCurrentSampleTime() + event.sampleOffset;
		uint64 offTime = onTime + static_cast<uint64>(FRET_NOISE_LENGTH * scheduler.getSamplesPerMs());
		float velocity = prm.get(PARAM::FNOISE_VELOCITY) / 127;

		scheduler.addNoteOn(onTime, offTime, SPECIAL_NOTES, pitch, velocity, prm.get(PARAM::FNOISE_CHANNEL) - 1);
	}

	ParamID MyVSTProcessor::getParamIdByPitch(Event event)
	{
		// ピッチからパラメータID取得
		int pitch = -1;

		if (event.type == Event::kNoteOnEvent)
		{
			pitch = event.noteOn.pitch;
		}
		else
		{
			return 0;
		}

		std::array<const ParamDef*, PARAM_TRIGGER_COUNT> triggerParams;
		size_t triggerCount = 0;
		getTriggerParams(triggerParams, triggerCount);

		for (size_t i = 0; i < triggerCount; ++i)
		{
			const ParamDef* def = triggerParams[i];

			if (prm.get(def->tag) == pitch)
			{
				return def->tag;
			}
		}
		return 0;
	}

	StringSet MyVSTProcessor::getTargetStrings(StringSet fretPos, bool isAbove, bool isDown, int maxStrings = STRING_COUNT)
	{
		//
		// 鳴らす弦の番号を昇順に取得する
		// fretPos        : コードフォーム
		// isDown = true  : ６弦方向から
		// isDown = false : １弦方向から
		// maxStrings     : 弦数

		StringSet result{};

		std::vector<int> values{};

		int cnt = 0;

		for (int i = 0; i < STRING_COUNT; i++)
		{
			int s = isAbove ? i : STRING_COUNT - i - 1;
			if (fretPos.data[s] < 0) continue;

			values.push_back(s);
			++cnt;

			if (cnt >= maxStrings) break;
		}

		if (isAbove && isDown)
		{
			std::reverse(values.begin(), values.end());
		}

		for (size_t i = 0; i < values.size(); i++)
		{
			result.data[i] = values[i];
			result.size++;
		}

		return result;
	}

	void MyVSTProcessor::notifyChordNumberChanged(int chordNumber)
	{
		FUnknownPtr<IMessage> msg = allocateMessage();
		if (!msg) return;

		msg->setMessageID(MSG_CHORD_CHANGED);
		msg->getAttributes()->setBinary(MSG_CHORD_VALUE, &chordNumber, sizeof(int));
		sendMessage(msg);
	}

	tresult PLUGIN_API MyVSTProcessor::notify(IMessage* message)
	{
		auto msgID = message->getMessageID();

		if (strcmp(msgID, MSG_SOUND_CHECK) == 0)
		{
			return notifyStrumTrigger(message);
		}

		return kResultFalse;
	}

	tresult PLUGIN_API MyVSTProcessor::notifyStrumTrigger(IMessage* message)
	{
		Event event{};
		const void* msgData;
		uint32 msgSize;

		const auto attr = message->getAttributes();
		if (attr == nullptr)return kResultFalse;

		if (!(attr->getBinary(MSG_SOUND_CHECK, msgData, msgSize) == kResultTrue && msgSize == sizeof(CNoteMsg)))
		{
			return kResultFalse;
		}

		const auto note = reinterpret_cast<const CNoteMsg*>(msgData);

		int pitch = prm.get(note->tag);

		if (pitch <= 0) return kResultOk;

		event.flags = Event::EventFlags::kIsLive;
		event.type = note->isOn ? Event::EventTypes::kNoteOnEvent : Event::EventTypes::kNoteOffEvent;

		if (note->isOn)
		{
			event.noteOn.noteId = -1;
			event.noteOn.pitch = pitch;
			event.noteOn.velocity = note->velocity / (float)128;
		}
		else
		{
			event.noteOff.noteId = -1;
			event.noteOff.pitch = pitch;
			event.noteOff.velocity = 0;
		}

		// プラグインプロセッサのノートバッファに追加
		InnerEvents.push(event);

		return kResultOk;
	}

	void PLUGIN_API MyVSTProcessor::processAudio(Vst::ProcessData& data)
	{
		if (data.numSamples > 0)
		{
			int32 minBus = (std::min)(data.numInputs, data.numOutputs);

			for (int32 i = 0; i < minBus; i++)
			{
				int32 minChan = (std::min)(data.inputs[i].numChannels, data.outputs[i].numChannels);
				for (int32 c = 0; c < minChan; c++)
				{
					if (data.outputs[i].channelBuffers32[c] != data.inputs[i].channelBuffers32[c])
					{
						memcpy(data.outputs[i].channelBuffers32[c], data.inputs[i].channelBuffers32[c], data.numSamples * sizeof(Vst::Sample32));
					}
				}
				data.outputs[i].silenceFlags = data.inputs[i].silenceFlags;

				for (int32 c = minChan; c < data.outputs[i].numChannels; c++)
				{
					memset(data.outputs[i].channelBuffers32[c], 0, data.numSamples * sizeof(Vst::Sample32));
					data.outputs[i].silenceFlags |= ((uint64)1 << c);
				}
			}

			for (int32 i = minBus; i < data.numOutputs; i++)
			{
				for (int32 c = 0; c < data.outputs[i].numChannels; c++)
				{
					memset(data.outputs[i].channelBuffers32[c], 0, data.numSamples * sizeof(Vst::Sample32));
				}
				data.outputs[i].silenceFlags = ((uint64)1 << data.outputs[i].numChannels) - 1;
			}
		}
	}
}