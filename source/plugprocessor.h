//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------

#pragma once

#include <pluginterfaces/base/ftypes.h>
#include <public.sdk/source/vst/utility/ringbuffer.h>
#include <public.sdk/source/vst/vstaudioeffect.h>

#include "chordmap.h"
#include "myparameters.h"
#include "parameterframework.h"
#include "plugdefine.h"

#include "eventscheduler.h"

namespace MinMax
{
	class MyVSTProcessor 
		: public Steinberg::Vst::AudioEffect
		, public IScheduledEventListener
	{
		using RingBuff =OneReaderOneWriter::RingBuffer<Event>;
		using ProcessorParamStorage = ParameterFramework::ProcessorParamStorage;

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

		// IScheduledEventListenerの実装
		void PLUGIN_API sendNoteEvent(const NoteEvent& ev) override;

		// 追加メソッド
		void PLUGIN_API processContext();
		void PLUGIN_API processParameter();
		void PLUGIN_API articulationChanged(int sampleOffset);
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
		StringSet PLUGIN_API getTargetStrings(StringSet fretPos, bool isAbove, bool isDown, int maxStrings);
		void PLUGIN_API notifyChordNumberChanged(int chordNumber);
		tresult PLUGIN_API notify(IMessage* message) SMTG_OVERRIDE;
		tresult PLUGIN_API notifyStrumTrigger(IMessage* message);
		void PLUGIN_API processAudio(Vst::ProcessData& data);

		// ストラムイベントスケジューラー
		EventScheduler& scheduler = EventScheduler::Instance();

		// コードマップ
		ChordMap& chordMap = ChordMap::Instance();

		// プロセスデータの写し
		ProcessData* processData{};

		// パラメータキャッシュ値
		ProcessorParamStorage prm;

		// コード変更監視用
		double lastChordNum = 0;

		// コード変更時の平均フレットポジションの移動距離
		float distance = 0.0f;

		// DAWの再生ステータス
		bool isPlaying = false;

		// 内部MIDIイベントバッファ
		RingBuff InnerEvents{ 32 };
	};
}