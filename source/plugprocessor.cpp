//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------

#include <pluginterfaces/vst/ivstparameterchanges.h>
#include <base/source/fstreamer.h>
#include <vstgui/vstgui_uidescription.h>

#include "plugprocessor.h"
#include "plugcids.h"

namespace MinMax
{
	using namespace Steinberg;
	using namespace ParameterFramework;

#pragma region Implements

	void MyVSTProcessor::sendNoteEvent(const NoteEvent& e)
	{
		if (!processData) return;
		if (!processData->outputEvents) return;

		Event event{};

		event.flags = Event::kIsLive;
		event.busIndex = 0;
		event.sampleOffset = e.sampleOffset;

		if (e.on)
		{
			event.type = Event::kNoteOnEvent;
			event.noteOn.channel = e.channel;
			event.noteOn.noteId = e.noteid;
			event.noteOn.pitch = e.pitch;
			event.noteOn.velocity = e.velocity;
		}
		else
		{
			event.type = Event::kNoteOffEvent;
			event.noteOff.channel = e.channel;
			event.noteOff.noteId = e.noteid;
			event.noteOff.pitch = e.pitch;
			event.noteOff.velocity = 0.0f;
		}

		processData->outputEvents->addEvent(event);
	}

#pragma endregion

#pragma region ctor etc.

	MyVSTProcessor::MyVSTProcessor()
	{
		processData = nullptr;

		setControllerClass(kMyVSTControllerUID);

		// コード一覧読込
		const char* STR_USERPROFILE = "USERPROFILE";
		const char* PRESET_ROOT = "Documents/VST3 Presets/MinMax/QBStrum/Standard 6Strings.json";
		std::filesystem::path path = std::filesystem::path(getenv(STR_USERPROFILE)).append(PRESET_ROOT).make_preferred();
		ChordMap::initFromPreset(path);
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
		if (!state) return kResultFalse;
		IBStreamer streamer(state, kLittleEndian);

		return kResultOk;
	}

	tresult PLUGIN_API MyVSTProcessor::getState(IBStream* state)
	{
		if (!state) return kResultFalse;
		IBStreamer streamer(state, kLittleEndian);


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

			setParameterValue(tag, value);
		}
	}

	void PLUGIN_API MyVSTProcessor::setParameterValue(ParamID tag, ParamValue value)
	{
	}

	void PLUGIN_API MyVSTProcessor::articulationChanged(int oldArticulation, int newArticulation)
	{
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
			ParamID paramid = getParamIdByPitch(event);
			if (paramid < 0) continue;

			switch (event.type)
			{
			case Event::kNoteOnEvent:
				routingProcess(getParamIdByPitch(event), event);
				break;

			default:
				break;
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

			default:
				break;
			}
		}
	}

	void PLUGIN_API MyVSTProcessor::routingProcess(ParamID paramid, Event event)
	{
		/*
		// ストラム処理振分け
		switch (static_cast<PARAM>(paramid))
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
			trigStrum(event, true, false, stringsUpHigh);
			break;
		case PARAM::UP:
			trigStrum(event, true, false, STRING_COUNT);
			break;
		case PARAM::DOWN_HIGH:
			trigStrum(event, true, true, stringsDownHigh);
			break;
		case PARAM::DOWN:
			trigStrum(event, false, true, STRING_COUNT);
			break;
		case PARAM::DOWN_LOW:
			trigStrum(event, false, true, stringsDownLow);
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
		*/
	}

	void MyVSTProcessor::trigAllNotesOff()
	{
		scheduler.allNotesOff();
	}

	void MyVSTProcessor::trigBrush(Event event, bool isDown)
	{
		/*
		const int BRUSH_PER_MS = 1;
		const float BRUSH_DECAY = 0.98f;

		// 現在鳴っている音をすべて消す
		scheduler.allNotesOff();

		trigFretNoise(event);

		// コード情報を取得する
		auto& fretpos = chordMap.getFretPositions(chord);
		auto& strnum = getTargetStrings(fretpos, false, isDown, STRING_COUNT);

		double samplesPerMs = scheduler.getSamplesPerMs();
		uint64 baseOnTime = scheduler.getCurrentSampleTime() + event.sampleOffset;

		float baseVelocity = std::clamp(event.noteOn.velocity, 0.0f, 1.0f);

		// 発音弦のカウンタ
		int strcnt = 0;

		// 音価はms指定のパラメータ
		uint64 offTime = baseOnTime + samplesPerMs * brushTime;

		for (auto& i : strnum)
		{
			uint64 offsetSamples = samplesPerMs * (BRUSH_PER_MS * strcnt);

			uint64 onTime = baseOnTime + offsetSamples;

			int pitch = chordMap.getTunings()[i] + fretpos[i] + (transpose - 12);
			float velocity = baseVelocity * std::pow(BRUSH_DECAY, strcnt);

			int channel = channelSepalate ? i % 16 : 0;
			scheduler.addNoteOn(onTime, offTime, i, pitch, velocity, channel);

			strcnt++;
		}
		*/
	}

	void MyVSTProcessor::trigStrum(Event event, bool isAbove, bool isDown, int maxStrings)
	{
		/*
		trigFretNoise(event);

		// コード情報を取得する
		auto& fretpos = chordMap.getFretPositions(chord);
		auto& strnum = getTargetStrings(fretpos, isAbove, isDown, maxStrings);
		
		uint64 baseOnTime = scheduler.getCurrentSampleTime() + event.sampleOffset;

		double samplesPerMs = scheduler.getSamplesPerMs();

		float baseVelocity = std::clamp(event.noteOn.velocity, 0.0f, 1.0f);

		// 音価はビート数指定のパラメータ
		double samplesPerBeat = scheduler.getSampleRate() * 60.0 / scheduler.getTempo();
		uint64 lengthSamples = static_cast<uint64>(std::lround(samplesPerBeat * strumLength));
		uint64 offTime = baseOnTime + lengthSamples;

		// 発音弦のカウンタ
		int strcnt = 0;

		for (auto& i : strnum)
		{
			double delayMs =
				(strnum.size() > 1)
				? (strumSpeed / double(STRING_COUNT)) * strcnt
				: 0.0;

			uint64 onTime = baseOnTime + static_cast<uint64>(delayMs * samplesPerMs);

			int pitch = chordMap.getTunings()[i] + fretpos[i] + (transpose - 12);
			float velocity = baseVelocity * std::pow(strumDecay / 100.0f, strcnt);

			int channel = channelSepalate ? i % 16 : 0;
			scheduler.addNoteOn(onTime, offTime, i, pitch, velocity, channel);

			++strcnt;
		}
		*/
	}

	void MyVSTProcessor::trigMute(PARAM trigger, Event event)
	{
		/*
		const double NOTE_LENGTH = 40.0;

		// 現在鳴っている音をすべて消す
		scheduler.allNotesOff();

		trigFretNoise(event);

		// ミュート用ノート選択
		int muteNote =
			(trigger == PARAM_TRIGGER::MUTE_1)
			? mute1Note
			: mute2Note;

		float velocity = std::clamp(event.noteOn.velocity, 0.01f, 1.0f);

		// 音価は固定
		uint64 onTime = scheduler.getCurrentSampleTime() + event.sampleOffset;

		uint64 offTime = onTime + static_cast<uint64>(NOTE_LENGTH * scheduler.getSamplesPerMs());

		scheduler.addNoteOn(onTime, offTime, 0, muteNote, velocity, muteChannel);
		*/
	}

	void MyVSTProcessor::trigArpeggio(int stringindex, Event event)
	{
		/*
		trigFretNoise(event);

		auto& fretpos = chordMap.getFretPositions(chord);

		// 弦が鳴らせない場合
		if (stringindex < 0 || stringindex >= STRING_COUNT || fretpos[stringindex] < 0)
		{
			return;
		}

		// 音価はビート数指定のパラメータ
		uint64 onTime = scheduler.getCurrentSampleTime() + event.sampleOffset;

		double samplesPerBeat = scheduler.getSampleRate() * 60.0 / scheduler.getTempo();
		uint64 lengthSamples = static_cast<uint64>(std::lround(samplesPerBeat * arpLength));
		uint64 offTime = onTime + lengthSamples;

		int pitch = chordMap.getTunings()[stringindex] + fretpos[stringindex] + (transpose - 12);
		float velocity = std::clamp(event.noteOn.velocity, 0.0f, 1.0f);

		int channel = channelSepalate ? stringindex % 16 : 0;
		scheduler.addNoteOn(onTime, offTime, stringindex, pitch, velocity, channel);
		*/
	}

	void MyVSTProcessor::trigFretNoise(Event event)
	{
		/*
		constexpr double FRET_NOISE_LENGTH = 40.0;

		float distance = 0.0f;

		// コード変更
		if (lastChord.root != chord.root || lastChord.type != chord.type || lastChord.position != chord.position)
		{
			distance =
				std::abs(
					chordMap.getPositionAverage(lastChord) -
					chordMap.getPositionAverage(chord));

			// コード変更時は全消音
			trigAllNotesOff();

			// 現在のコードを記録
			lastChord.root = chord.root;
			lastChord.type = chord.type;
			lastChord.position = chord.position;
		}

		if (fnoiseNoteNear == 0 && fnoiseNoteFar == 0) return;

		int pitch = 0;

		if (distance < fnoiseDistNear) return;

		if (distance < fnoiseDistFar)
		{
			pitch = fnoiseNoteNear == 0 ? fnoiseDistFar : fnoiseNoteNear;
		}
		else
		{
			pitch = fnoiseNoteFar == 0 ? fnoiseNoteNear : fnoiseNoteFar;
		}

		if (pitch == 0) return;

		uint64 onTime = scheduler.getCurrentSampleTime() + event.sampleOffset;
		uint64 offTime = onTime + static_cast<uint64>(FRET_NOISE_LENGTH * scheduler.getSamplesPerMs());

		scheduler.addNoteOn(onTime, offTime, SPECIAL_NOTES, pitch, 127, fnoiseChannel);
		*/
	}

	ParamID MyVSTProcessor::getParamIdByPitch(Event event)
	{
		/*
		// キースイッチからパラメータID取得
		int ksw = -1;

		if (event.type == Event::kNoteOnEvent)
		{
			ksw = event.noteOn.pitch;
		}
		else if (event.type == Event::kNoteOffEvent)
		{
			ksw = event.noteOff.pitch;
		}
		else
		{
			return ksw;
		}

		auto it = std::find(trigKeySW.begin(), trigKeySW.end(), ksw);
		if (it == trigKeySW.end()) return ksw;

		int index = std::distance(trigKeySW.begin(), it);

		return TriggerDef[index].value;
		*/
		return kResultOk;
	}

	vector<int> MyVSTProcessor::getTargetStrings(vector<int> fretPos, bool isAbove, bool isDown, int maxStrings = STRING_COUNT)
	{
		std::vector<int> result{};

		int cnt = 0;

		for (int i = 0; i < STRING_COUNT; i++)
		{
			int s = isAbove ? i : STRING_COUNT - i - 1;
			if (fretPos[s] < 0) continue;

			result.push_back(s);
			++cnt;

			if (cnt >= maxStrings) break;
		}

		if (isAbove && isDown)
		{
			std::reverse(result.begin(), result.end());
		}

		return result;
	}

	void MyVSTProcessor::notifyChordChanged()
	{
		FUnknownPtr<IMessage> msg = allocateMessage();
		msg->setMessageID(MSG_CHORD_CHANGED);
		msg->getAttributes()->setBinary(MSG_CHORD_VALUE, &chord, sizeof(CChord));
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
		/*
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

		int pitch = -1;

		int index = 0;
		for (const auto& def : TriggerDef)
		{
			if (def.value == note->value)
			{
				pitch = trigKeySW[index];
				break;
			}
			++index;
		}

		if (pitch < 0) return kResultOk;

		event.flags = Event::EventFlags::kIsLive;
		event.type = note->OnOff ? Event::EventTypes::kNoteOnEvent : Event::EventTypes::kNoteOffEvent;

		if (note->OnOff)
		{
			event.noteOn.pitch = pitch;
			event.noteOn.velocity = note->velocity / (float)NOTE_COUNT;
		}
		else
		{
			event.noteOff.pitch = pitch;
			event.noteOff.velocity = 0;
		}

		// プラグインプロセッサのノートバッファに追加
		InnerEvents.push(event);

		return kResultOk;
		*/
		return kResultOk;
	}

	void PLUGIN_API MyVSTProcessor::processAudio(Vst::ProcessData& data)
	{
		/*
		if (data.numSamples > 0)
		{
			int32 minBus = std::min(data.numInputs, data.numOutputs);

			for (int32 i = 0; i < minBus; i++)
			{
				int32 minChan = std::min(data.inputs[i].numChannels, data.outputs[i].numChannels);
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
		*/
	}
}