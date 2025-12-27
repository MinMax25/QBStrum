//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------

#pragma once

#include <public.sdk/source/vst/vstaudioeffect.h>
#include <public.sdk/source/vst/utility/ringbuffer.h>
#include <pluginterfaces/base/ftypes.h>
#include <array>

#include "plugdefine.h"

#include "ParameterFramework.h"
#include "EventScheduler.h"
#include "ChordMap.h"
#include "NoteEvent.h"

namespace MinMax
{
	class MyVSTProcessor 
		: public Steinberg::Vst::AudioEffect
		, public IScheduledEventListener
	{
		using RingBuff =OneReaderOneWriter::RingBuffer<Event>;
		using ExParamContainer = ParameterFramework::ExParamContainer;
		using ParamValueStorage = ParameterFramework::ParamValueStorage;

	public:
		MyVSTProcessor();

		~MyVSTProcessor() SMTG_OVERRIDE;

		static Steinberg::FUnknown* createInstance(void* /*context*/)
		{
			return (Steinberg::Vst::IAudioProcessor*)new MyVSTProcessor;
		}

		Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
		Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;
		Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) SMTG_OVERRIDE;
		Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& newSetup) SMTG_OVERRIDE;
		Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;
		Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
		Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;
		Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;

	protected:

		void PLUGIN_API sendNoteEvent(const NoteEvent& ev) override;

		void PLUGIN_API processContext();

		void PLUGIN_API processParameter();

		void PLUGIN_API setParameterValue(ParamID tag, ParamValue value);

		void PLUGIN_API articulationChanged(int oldArticulation, int newArticulation);

		void PLUGIN_API processEvent();

		void PLUGIN_API processInnerEvent();

		void PLUGIN_API routingProcess(ParamID parmid, Event event);

		void PLUGIN_API trigAllNotesOff();

		void PLUGIN_API trigBrush(Event event, bool isDown);

		void PLUGIN_API trigStrum(Event event, bool isAbove, bool isDown, int maxStrings);

		void PLUGIN_API trigMute(PARAM_TRIGGER trigger, Event event);

		void PLUGIN_API trigArpeggio(int stringindex, Event event);

		void PLUGIN_API trigFretNoise(Event event);

		ParamID PLUGIN_API getParamIdByPitch(Event event);

		vector<int> PLUGIN_API getTargetStrings(vector<int> fretPos, bool isAbove, bool isDown, int maxStrings);

		void PLUGIN_API notifyChordChanged();

		tresult PLUGIN_API notify(IMessage* message) SMTG_OVERRIDE;

		tresult PLUGIN_API notifyStrumTrigger(IMessage* message);

		void PLUGIN_API processAudio(Vst::ProcessData& data);

		/***
		 * 
		 */

		EventScheduler& scheduler = EventScheduler::Instance();

		ChordMap& chordMap = ChordMap::Instance();

		ProcessData* processData{};

		bool isPlaying = false;

		CChord chord{};

		CChord lastChord{};

		float distance = 0.0f;

		// 内部MIDIイベントバッファ
		RingBuff InnerEvents{ 32 };

		const ExParamContainer paramDefs;

		ParamValueStorage paramValues;

		// パラメータ値キャッシュ
		bool bypass = false;

		int transpose = 12;
		bool channelSepalate = false;
		int selectedArticulation = 0;

		int muteChannel = 0;
		int mute1Note = 103;
		int mute2Note = 102;

		double strumSpeed = 20.0;
		double strumDecay = 95.0;
		double strumLength = 3.0;

		double arpLength = 2.0;

		double brushTime = 40;

		int fnoiseChannel = 0;
		int fnoiseNoteNear = 120;
		int fnoiseNoteFar = 123;
		int fnoiseDistNear = 1;
		int fnoiseDistFar = 3;
		int stringsUpHigh = 3;
		int stringsDownHigh = 4;
		int stringsDownLow = 2;

		std::array<int, PARAM_TRIGGER_COUNT> trigKeySW =
		{
			72, // TRIGGER_ALLMUTE
			68, // TRIGGER_BRUSH_UP
			67, // TRIGGER_BRUSH_DOWN
			66, // TRIGGER_UP_HIGH
			65, // TRIGGER_UP
			64, // TRIGGER_DOWN_HIGH
			63, // TRIGGER_DOWN
			62, // TRIGGER_DOWN_LOW
			61,	// TRIGGER_MUTE_1
			60,	// TRIGGER_MUTE_2
			57, // TRIGGER_ARPEGGIO_1
			55, // TRIGGER_ARPEGGIO_2
			53, // TRIGGER_ARPEGGIO_3
			52, // TRIGGER_ARPEGGIO_4
			50, // TRIGGER_ARPEGGIO_5
			48, // TRIGGER_ARPEGGIO_6
		};

		std::array<int, PARAM_ARTICULATION_COUNT> Articulations =
		{
			24, // OPEN1 C 0
			0,	// OPEN2 C -2
			25, // HAM_LEG C# 0
			26, // MUTE D 0
			27, // DEAD D# 0
			28, // HARMONICS E 0
			33, // SLIDE A 0
		};
	};
}