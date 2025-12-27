//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------

#pragma once

#include <public.sdk/source/vst/vstaudioeffect.h>
#include <public.sdk/source/vst/utility/ringbuffer.h>
#include <pluginterfaces/base/ftypes.h>
#include <array>

#include "plugdefine.h"

#include "parameterframework.h"
#include "myparameters.h"

#include "EventScheduler.h"
#include "chordmap.h"
#include "NoteEvent.h"

namespace MinMax
{
	class MyVSTProcessor 
		: public Steinberg::Vst::AudioEffect
		, public IScheduledEventListener
	{
		using RingBuff =OneReaderOneWriter::RingBuffer<Event>;

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

		void PLUGIN_API trigMute(PARAM trigger, Event event);

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
	};
}