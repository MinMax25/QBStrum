//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <pluginterfaces/base/fplatform.h>
#include <pluginterfaces/base/fstrdefs.h>
#include <pluginterfaces/base/ftypes.h>
#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/base/ibstream.h>
#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivstevents.h>
#include <pluginterfaces/vst/ivstmessage.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>
#include <pluginterfaces/vst/vstspeaker.h>
#include <pluginterfaces/vst/vsttypes.h>
#include <public.sdk/source/vst/vstaudioeffect.h>
#include <string>
#include <string.h>
#include <vector>

#include "chordmap.h"
#include "cnotemsg.h"
#include "EventScheduler.h"
#include "myparameters.h"
#include "parameterhelper.h"
#include "plugcids.h"
#include "plugdefine.h"
#include "plugprocessor.h"
#include "stateio.h"

namespace MinMax
{
#pragma region Implements

	void MyVSTProcessor::sendNoteEvent(const NoteEvent& e)
	{
		if (!processData) return;
		if (!processData->outputEvents) return;

		Steinberg::Vst::Event event{};

		event.flags = 0;
		event.busIndex = 0;
		event.sampleOffset = e.sampleOffset;

		if (e.eventType == NoteEventType::On)
		{
			event.type = Steinberg::Vst::Event::kNoteOnEvent;
			event.noteOn.channel = e.channel;
			event.noteOn.noteId = e.noteId;
			event.noteOn.pitch = e.pitch;
			event.noteOn.velocity = e.velocity;
		}
		else
		{
			event.type = Steinberg::Vst::Event::kNoteOffEvent;
			event.noteOff.channel = e.channel;
			event.noteOff.noteId = e.noteId;
			event.noteOff.pitch = e.pitch;
			event.noteOff.velocity = 0.0f;
		}

		processData->outputEvents->addEvent(event);
	}

	void PLUGIN_API MyVSTProcessor::chordChanged(const Steinberg::Vst::Chord chord)
	{
		// not implements
	}

#pragma endregion

#pragma region ctor etc.

	MyVSTProcessor::MyVSTProcessor()
	{
		processData = nullptr;

		setControllerClass(kMyVSTControllerUID);

		// Initialize Parameters
		Files::createPluginDirectory();
		Files::clearTempFiles();

		chordMap.loadChordPreset();

		PF::ParamHelper::instance().initialize(&rangeResolver, &optionProvider);
		prm.initialize(paramTable, &rangeResolver);
	}

	MyVSTProcessor::~MyVSTProcessor()
	{
	}

	Steinberg::tresult PLUGIN_API MyVSTProcessor::initialize(FUnknown* context)
	{
		Steinberg::tresult result = AudioEffect::initialize(context);
		if (result != Steinberg::kResultOk) return result;

		addAudioOutput(STR16("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);
		addEventInput(STR16("Event In"), 1);
		addEventOutput(STR16("Event Out"), 16);

		return Steinberg::kResultOk;
	}

	Steinberg::tresult PLUGIN_API MyVSTProcessor::terminate()
	{
		return AudioEffect::terminate();
	}

	Steinberg::tresult PLUGIN_API MyVSTProcessor::setActive(Steinberg::TBool state)
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

	Steinberg::tresult PLUGIN_API MyVSTProcessor::setupProcessing(Steinberg::Vst::ProcessSetup& newSetup)
	{
		scheduler.setSampleRate(newSetup.sampleRate);
		return AudioEffect::setupProcessing(newSetup);
	}

	Steinberg::tresult PLUGIN_API MyVSTProcessor::canProcessSampleSize(Steinberg::int32 symbolicSampleSize)
	{
		if (symbolicSampleSize == Steinberg::Vst::kSample32) return Steinberg::kResultTrue;
		return Steinberg::kResultFalse;
	}

#pragma region

#pragma region State

	Steinberg::tresult PLUGIN_API MyVSTProcessor::setState(Steinberg::IBStream* state)
	{
		if (!state) return Steinberg::kResultFalse;
		StateIO io(state);

		for (const auto& def : paramTable)
		{
			double plain = 0.0;
			if (!io.readDouble(plain)) return Steinberg::kResultFalse;
			prm.set(def.tag, plain);
		}

		std::wstring path{};
		if (io.readWString(path))
		{
			std::filesystem::path p(path);
			chordMap.loadChordPreset(p);
		}

		return Steinberg::kResultOk;
	}

	Steinberg::tresult PLUGIN_API MyVSTProcessor::getState(Steinberg::IBStream* state)
	{
		if (!state) return Steinberg::kResultFalse;
		StateIO io(state);

		for (const auto& def : paramTable)
		{
			double plain = prm.get(def.tag);
			if (!io.writeDouble(plain)) return Steinberg::kResultFalse;
		}

		const std::wstring path = chordMap.getPresetPath().wstring();
		if (!io.writeWString(path)) return Steinberg::kResultFalse;

		return Steinberg::kResultOk;
	}

#pragma region process

	Steinberg::tresult PLUGIN_API MyVSTProcessor::process(Steinberg::Vst::ProcessData& data)
	{
		processData = &data;

		scheduler.process(data);

		processContext();
		processParameter();
		processEvent();
		processInnerEvent();
		processAudio(data);

		scheduler.dispatch();

		processData = nullptr;

		return Steinberg::kResultOk;
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

		Steinberg::int32 paramChangeCount = processData->inputParameterChanges->getParameterCount();

		for (Steinberg::int32 i = 0; i < paramChangeCount; i++)
		{
			Steinberg::Vst::IParamValueQueue* queue = processData->inputParameterChanges->getParameterData(i);
			if (queue == NULL) continue;

			Steinberg::Vst::ParamID tag = queue->getParameterId();

			Steinberg::int32 valueChangeCount = queue->getPointCount();
			Steinberg::int32 sampleOffset;
			Steinberg::Vst::ParamValue value;

			if (!(queue->getPoint(valueChangeCount - 1, sampleOffset, value) == Steinberg::kResultTrue)) continue;

			// cash parameter
			prm.setNormalized(tag, value);

			switch (tag)
			{
			case SELECTED_ARTICULATION:
			{
				if (prm.isChanged(tag))
				{
					articulationChanged(sampleOffset);
				}
				break;
			}
			case CHORD_MSB:
			{
				if (prm.isChanged(CHORD_MSB) || prm.isChanged(CHORD_LSB))
				{
					Steinberg::Vst::ParamValue num = prm.get(CHORD_MSB) * 128 + prm.get(CHORD_LSB);
					prm.setInt(CHORD_NUM, num);
					notifyChordNumberChanged();
					prm.clearChangedFlags(CHORD_MSB);
					prm.clearChangedFlags(CHORD_LSB);
				}
				break;
			}
			case CHORD_NUM:
			{
				if (prm.isChanged(tag))
				{
					notifyChordNumberChanged();
					prm.clearChangedFlags(tag);
				}
				break;
			}
			case NEED_SAMPLEBLOCK_ADJUST:
			{
				bool state = prm.get(tag) > 0.5;
				scheduler.setNeedSampleblockAdust(state);
				break;
			}
			default:
			{
				if (STR1_OFFSET <= tag && tag <= STR7_OFFSET)
				{
					if (prm.isChanged(tag))
					{
						notifyChordNumberChanged();
						prm.clearChangedFlags(tag);
					}
				}
				break;
			}
			}
		}
	}

	void PLUGIN_API MyVSTProcessor::articulationChanged(int sampleOffset)
	{
		constexpr int SPECIAL_NOTES_SAMPLES = 3;

		prm.clearChangedFlags(SELECTED_ARTICULATION);

		if (!prm.getInt(ENABLED_ARTICULATION)) return;

		int newArtic = prm.getInt(SELECTED_ARTICULATION);

		std::array<const PF::ParamDef*, PARAM_ARTICULATION_COUNT> artics;
		size_t articCount = 0;
		getArticulationParams(artics, articCount);

		int keySW = 0;

		for (int i = 0; i < (int)articCount; ++i)
		{
			const PF::ParamDef* def = artics[i];

			if (i == newArtic)
			{
				keySW = prm.get(def->tag);
				break;
			}
		}
		if (keySW == 0) return;

		Steinberg::uint64 onTime = scheduler.getCurrentSampleTime() + sampleOffset;
		Steinberg::uint64 offTime = onTime + SPECIAL_NOTES_SAMPLES;

		for (int i = 0; i < (prm.get(CHANNEL_SEPARATE) ? STRING_COUNT : 1); i++)
		{
			scheduler.addNoteOn(onTime, offTime, SPECIAL_NOTES, keySW, 127, i);
		}
	}

	void PLUGIN_API MyVSTProcessor::processEvent()
	{
		if (!processData) return;
		if (processData->inputEvents == NULL) return;

		Steinberg::Vst::Event event;

		for (Steinberg::int32 i = 0; i < processData->inputEvents->getEventCount(); i++)
		{
			if (processData->inputEvents->getEvent(i, event) != Steinberg::kResultOk) continue;

			if (event.type == Steinberg::Vst::Event::kNoteOnEvent)
			{
				Steinberg::Vst::ParamID tag = getParamIdByPitch(event);
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

		Steinberg::Vst::Event event{};

		while (InnerEvents.pop(event))
		{
			Steinberg::Vst::ParamID paramid = getParamIdByPitch(event);
			if (paramid < 0) continue;

			switch (event.type)
			{
			case Steinberg::Vst::Event::kNoteOnEvent:
				routingProcess(paramid, event);
				break;

			case Steinberg::Vst::Event::kNoteOffEvent:
				break;

			default:
				break;
			}
		}
	}

	void PLUGIN_API MyVSTProcessor::routingProcess(Steinberg::Vst::ParamID paramid, Steinberg::Vst::Event event)
	{
		switch (paramid)
		{
		case ALL_NOTES_OFF:
			trigAllNotesOff();
			break;
		case BRUSH_DOWN:
			trigBrush(event, true, prm.getInt(STRINGS_DOWN) + 1, prm.get(BRUSH_DOWN_TIME));
			break;
		case BRUSH_UP:
			trigBrush(event, false, prm.getInt(STRINGS_UP) + 1, prm.get(BRUSH_UP_TIME));
			break;
		case UP_HIGH:
			trigStrum(event, true, false, prm.getInt(STRINGS_UP_HIGH) + 1);
			break;
		case UP:
			trigStrum(event, true, false, STRING_COUNT);
			break;
		case UP_LOW:
			trigStrum(event, false, false, prm.getInt(STRINGS_UP_LOW) + 1);
			break;
		case DOWN_HIGH:
			trigStrum(event, true, true, prm.getInt(STRINGS_DOWN_HIGH) + 1);
			break;
		case DOWN:
			trigStrum(event, false, true, STRING_COUNT);
			break;
		case DOWN_LOW:
			trigStrum(event, false, true, prm.getInt(STRINGS_DOWN_LOW) + 1);
			break;
		case MUTE_1:
			trigMute(MUTE_1, event);
			break;
		case MUTE_2:
			trigMute(MUTE_2, event);
			break;
		case ARPEGGIO_1:
			trigArpeggio(0, event);
			break;
		case ARPEGGIO_2:
			trigArpeggio(1, event);
			break;
		case ARPEGGIO_3:
			trigArpeggio(2, event);
			break;
		case ARPEGGIO_4:
			trigArpeggio(3, event);
			break;
		case ARPEGGIO_5:
			trigArpeggio(4, event);
			break;
		case ARPEGGIO_6:
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

	void MyVSTProcessor::trigBrush(Steinberg::Vst::Event event, bool isDown, int maxStrings, Steinberg::Vst::ParamValue time)
	{
		float decay = prm.get(BRUSH_DECAY) / 100.0f;

		scheduler.allNotesOff();

		trigFretNoise(event);

		auto& voicing = getChordVoicing(prm.get(CHORD_NUM));
		auto& strnum = getTargetStrings(voicing, true, isDown, maxStrings);

		double samplesPerMs = scheduler.getSamplesPerMs();
		Steinberg::uint64 baseOnTime = scheduler.getCurrentSampleTime() + event.sampleOffset;

		float baseVelocity = std::clamp(event.noteOn.velocity, 0.0f, 1.0f);

		int strcnt = 0;

		Steinberg::uint64 offTime = baseOnTime + samplesPerMs * time;

		for (size_t s = 0; s < strnum.state; s++)
		{
			int i = strnum.data[s];

			Steinberg::uint64 offsetSamples = samplesPerMs * strcnt;
			Steinberg::uint64 onTime = baseOnTime + offsetSamples;

			int channel = prm.get(CHANNEL_SEPARATE) ? i % 16 : 0;
			
			int pitch = getStringPitch(voicing, i);
			if (pitch < 0) continue;

			float velocity = baseVelocity * std::pow(decay, strcnt);

			scheduler.addNoteOn(onTime, offTime, i, pitch, velocity, channel);

			strcnt++;
		}
	}

	void MyVSTProcessor::trigStrum(Steinberg::Vst::Event event, bool isAbove, bool isDown, int maxStrings)
	{
		trigFretNoise(event);

		auto& voicing = getChordVoicing(prm.getInt(CHORD_NUM));
		auto& strnum = getTargetStrings(voicing, isAbove, isDown, maxStrings);
		
		double samplesPerMs = scheduler.getSamplesPerMs();
		Steinberg::uint64 baseOnTime = scheduler.getCurrentSampleTime() + event.sampleOffset;

		float baseVelocity = std::clamp(event.noteOn.velocity, 0.0f, 1.0f);

		double samplesPerBeat = scheduler.getSampleRate() * 60.0 / scheduler.getTempo();
		Steinberg::uint64 lengthSamples = static_cast<Steinberg::uint64>(std::lround(samplesPerBeat * prm.get(STRUM_LENGTH)));
		Steinberg::uint64 offTime = baseOnTime + lengthSamples;

		int strcnt = 0;

		for (size_t s = 0; s < strnum.state; s++)
		{
			int i = strnum.data[s];

			double delayMs = (prm.get(STRUM_SPEED) / double(STRING_COUNT)) * strcnt;

			Steinberg::uint64 onTime = baseOnTime + static_cast<Steinberg::uint64>(delayMs * samplesPerMs);

			int channel = prm.getInt(CHANNEL_SEPARATE) ? i % 16 : 0;
			
			int pitch = getStringPitch(voicing, i);
			if (pitch < 0) continue;

			float velocity = baseVelocity * std::pow(prm.get(STRUM_DECAY) / 100.0f, strcnt);

			scheduler.addNoteOn(onTime, offTime, i, pitch, velocity, channel);

			++strcnt;
		}
	}

	int MyVSTProcessor::getStringPitch(const StringSet& set, int stringNumber)
	{
		int result = chordMap.getTunings().data[stringNumber] + (prm.getInt(TRANSPOSE) - 6) + (prm.getInt(OCTAVE) ? 12 : 0);
		int offset = (int)prm.getInt(STR1_OFFSET + stringNumber);

		switch (offset)
		{
		case 0:	// mute
			result = -1;
			break;
		case 1:	// open
			break;
		default:
			result += set.data[stringNumber] + offset - StringSet::CENTER_OFFSET;
			break;
		}

		return result;
	}

	void MyVSTProcessor::trigMute(PARAM trigger, Steinberg::Vst::Event event)
	{
		const double NOTE_LENGTH = 40.0;

		if (!prm.getInt(ENABLED_MUTE)) return;

		scheduler.allNotesOff();

		trigFretNoise(event);

		int muteNote =
			(trigger == MUTE_1)
			? prm.getInt(MUTE_NOTE_1)
			: prm.getInt(MUTE_NOTE_2);

		float velocity = std::clamp(event.noteOn.velocity, 0.01f, 1.0f);

		Steinberg::uint64 onTime = scheduler.getCurrentSampleTime() + event.sampleOffset;
		Steinberg::uint64 offTime = onTime + static_cast<Steinberg::uint64>(NOTE_LENGTH * scheduler.getSamplesPerMs());
		scheduler.addNoteOn(onTime, offTime, 0, muteNote, velocity, prm.getInt(MUTE_CHANNEL));
	}

	void MyVSTProcessor::trigArpeggio(int stringNumber, Steinberg::Vst::Event event)
	{
		trigFretNoise(event);

		auto& voicing = getChordVoicing(prm.getInt(CHORD_NUM));

		if (stringNumber < 0 || stringNumber >= STRING_COUNT || voicing.data[stringNumber] < 0)
		{
			return;
		}

		Steinberg::uint64 onTime = scheduler.getCurrentSampleTime() + event.sampleOffset;

		double samplesPerBeat = scheduler.getSampleRate() * 60.0 / scheduler.getTempo();
		Steinberg::uint64 lengthSamples = static_cast<Steinberg::uint64>(std::lround(samplesPerBeat * prm.get(ARP_LENGTH)));
		Steinberg::uint64 offTime = onTime + lengthSamples;

		int channel = prm.getInt(CHANNEL_SEPARATE) ? stringNumber % 16 : 0;

		int pitch = getStringPitch(voicing, stringNumber);
		if (pitch < 0) return;

		float velocity = std::clamp(event.noteOn.velocity, 0.0f, 1.0f);

		scheduler.addNoteOn(onTime, offTime, stringNumber, pitch, velocity, channel);
	}

	void MyVSTProcessor::trigFretNoise(Steinberg::Vst::Event event)
	{
		constexpr double FRET_NOISE_LENGTH = 40.0;

		if (!prm.getInt(ENABLED_NOIZE)) return;

		float distance = 0.0f;

		if (lastChordNum != prm.getInt(CHORD_NUM))
		{
			trigAllNotesOff();

			distance =
				std::abs(
					chordMap.getPositionAverage(lastChordNum) -
					chordMap.getPositionAverage(prm.get(CHORD_NUM)));

			lastChordNum = prm.getInt(CHORD_NUM);
		}

		if (prm.get(NOISE_NOTE_NEAR) == 0 && prm.get(NOISE_NOTE_FAR) == 0) return;

		int pitch = 0;

		if (distance < prm.getInt(NOISE_NOTE_NEAR)) return;

		if (distance < prm.getInt(NOISE_NOTE_FAR))
		{
			pitch =
				prm.getInt(NOISE_NOTE_NEAR) == 0
				? prm.getInt(NOISE_NOTE_FAR)
				: prm.getInt(NOISE_NOTE_NEAR);
		}
		else
		{
			pitch = 
				prm.getInt(NOISE_NOTE_FAR) == 0
				? prm.getInt(NOISE_NOTE_NEAR)
				: prm.getInt(NOISE_NOTE_FAR);
		}

		if (pitch == 0) return;

		Steinberg::uint64 onTime = scheduler.getCurrentSampleTime() + event.sampleOffset;
		Steinberg::uint64 offTime = onTime + static_cast<Steinberg::uint64>(FRET_NOISE_LENGTH * scheduler.getSamplesPerMs());
		float velocity = prm.get(NOISE_VELOCITY) / 127;

		scheduler.addNoteOn(onTime, offTime, SPECIAL_NOTES, pitch, velocity, prm.get(NOISE_CHANNEL));
	}

	Steinberg::Vst::ParamID MyVSTProcessor::getParamIdByPitch(Steinberg::Vst::Event event)
	{
		// key switch -> ParamID
		int pitch = -1;

		if (event.type == Steinberg::Vst::Event::kNoteOnEvent)
		{
			pitch = event.noteOn.pitch;
		}
		else
		{
			return 0;
		}

		std::array<const PF::ParamDef*, PARAM_TRIGGER_COUNT> triggerParams;
		size_t triggerCount = 0;
		getTriggerParams(triggerParams, triggerCount);

		for (size_t i = 0; i < triggerCount; ++i)
		{
			const PF::ParamDef* def = triggerParams[i];

			if (prm.getInt(def->tag) == pitch)
			{
				return def->tag;
			}
		}
		return 0;
	}

	StringSet PLUGIN_API MyVSTProcessor::getChordVoicing(int flatIndex)
	{
		if (prm.getInt(CHORD_EDITING))
		{
			return editChordVoicing;
		}
		else
		{
			return chordMap.getChordVoicing(flatIndex);
		}
	}

	StringSet MyVSTProcessor::getTargetStrings(StringSet voicing, bool isAbove, bool isDown, int maxStrings = STRING_COUNT)
	{
		StringSet result{};

		std::vector<int> values{};

		int cnt = 0;

		for (int i = 0; i < STRING_COUNT; i++)
		{
			int s = isAbove ? i : STRING_COUNT - i - 1;
			if (voicing.data[s] < 0) continue;

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
		}

		result.state = cnt;

		return result;
	}

	void MyVSTProcessor::notifyChordNumberChanged()
	{
		StringSet set{};

		if (++chordseq > 999999)
		{
			chordseq = 0;
		}

		set.state = chordseq;
		set.flatIndex = (int)prm.getInt(CHORD_NUM);

		auto v = chordMap.getChordVoicing(set.flatIndex);

		for (int i = 0; i < STRING_COUNT; i++)
		{
			int o = (int)prm.getInt(STR1_OFFSET + i);
			set.data[i] += v.data[i];
			set.setOffset(i, o - StringSet::CENTER_OFFSET);
		}

		auto* msg = allocateMessage();
		if (!msg) return;

		msg->setMessageID(MSG_CHORD_CHANGED);
		msg->getAttributes()->setBinary(MSG_CHORD_CHANGED, &set, sizeof(StringSet));

		sendMessage(msg);
	}

	Steinberg::tresult PLUGIN_API MyVSTProcessor::notify(Steinberg::Vst::IMessage* message)
	{
		auto msgID = message->getMessageID();

		if (strcmp(msgID, MSG_SOUND_CHECK) == 0)
		{
			return notifyStrumTrigger(message);
		}
		else if (strcmp(msgID, MSG_CHORD_EDIT) == 0)
		{
			return notifyEditChord(message);
		}

		return Steinberg::kResultFalse;
	}

	Steinberg::tresult PLUGIN_API MyVSTProcessor::notifyStrumTrigger(Steinberg::Vst::IMessage* message)
	{
		Steinberg::Vst::Event event{};
		const void* msgData;
		Steinberg::uint32 msgSize;

		const auto attr = message->getAttributes();
		if (attr == nullptr) return Steinberg::kResultFalse;

		if (!(attr->getBinary(MSG_SOUND_CHECK, msgData, msgSize) == Steinberg::kResultTrue && msgSize == sizeof(CNoteMsg)))
		{
			return Steinberg::kResultFalse;
		}

		const auto note = reinterpret_cast<const CNoteMsg*>(msgData);

		int pitch = prm.getInt(note->tag);

		if (pitch <= 0) return Steinberg::kResultOk;

		event.flags = Steinberg::Vst::Event::EventFlags::kIsLive;
		event.type = note->isOn ? Steinberg::Vst::Event::EventTypes::kNoteOnEvent : Steinberg::Vst::Event::EventTypes::kNoteOffEvent;

		if (note->isOn)
		{
			event.noteOn.noteId = -1;
			event.noteOn.pitch = pitch;
			event.noteOn.velocity = note->velocity / (float)127;
		}
		else
		{
			event.noteOff.noteId = -1;
			event.noteOff.pitch = pitch;
			event.noteOff.velocity = 0;
		}

		InnerEvents.push(event);

		return Steinberg::kResultOk;
	}

	Steinberg::tresult PLUGIN_API MyVSTProcessor::notifyEditChord(Steinberg::Vst::IMessage* message)
	{
		const void* msgData;
		Steinberg::uint32 msgSize;

		const auto attr = message->getAttributes();
		if (attr == nullptr) return Steinberg::kResultFalse;

		if (!(attr->getBinary(MSG_CHORD_EDIT, msgData, msgSize) == Steinberg::kResultTrue && msgSize == sizeof(StringSet)))
		{
			return Steinberg::kResultFalse;
		}

		const auto set = reinterpret_cast<const StringSet*>(msgData);

		editChordVoicing.flatIndex = set->flatIndex;
		editChordVoicing.state = set->state;

		for (int i = 0; i < STRING_COUNT; i++)
		{
			editChordVoicing.data[i] = set->data[i];
			editChordVoicing.setOffset(i, 0);
		}

		return Steinberg::kResultOk;
	}

	void PLUGIN_API MyVSTProcessor::processAudio(Steinberg::Vst::ProcessData& data)
	{
		if (data.numSamples > 0)
		{
			Steinberg::int32 minBus = (std::min)(data.numInputs, data.numOutputs);

			for (Steinberg::int32 i = 0; i < minBus; i++)
			{
				Steinberg::int32 minChan = (std::min)(data.inputs[i].numChannels, data.outputs[i].numChannels);
				for (Steinberg::int32 c = 0; c < minChan; c++)
				{
					if (data.outputs[i].channelBuffers32[c] != data.inputs[i].channelBuffers32[c])
					{
						memcpy(data.outputs[i].channelBuffers32[c], data.inputs[i].channelBuffers32[c], data.numSamples * sizeof(Steinberg::Vst::Sample32));
					}
				}
				data.outputs[i].silenceFlags = data.inputs[i].silenceFlags;

				for (Steinberg::int32 c = minChan; c < data.outputs[i].numChannels; c++)
				{
					memset(data.outputs[i].channelBuffers32[c], 0, data.numSamples * sizeof(Steinberg::Vst::Sample32));
					data.outputs[i].silenceFlags |= ((Steinberg::uint64)1 << c);
				}
			}

			for (Steinberg::int32 i = minBus; i < data.numOutputs; i++)
			{
				for (Steinberg::int32 c = 0; c < data.outputs[i].numChannels; c++)
				{
					memset(data.outputs[i].channelBuffers32[c], 0, data.numSamples * sizeof(Steinberg::Vst::Sample32));
				}
				data.outputs[i].silenceFlags = ((Steinberg::uint64)1 << data.outputs[i].numChannels) - 1;
			}
		}
	}
}