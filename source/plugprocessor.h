//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <pluginterfaces/base/fplatform.h>
#include <pluginterfaces/base/ftypes.h>
#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/base/ibstream.h>
#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivstevents.h>
#include <pluginterfaces/vst/ivstmessage.h>
#include <pluginterfaces/vst/vsttypes.h>
#include <public.sdk/source/vst/utility/ringbuffer.h>
#include <public.sdk/source/vst/vstaudioeffect.h>

#include "chordmap.h"
#include "eventscheduler.h"
#include "myparameters.h"
#include "parameterhelper.h"

namespace MinMax
{
	class MyVSTProcessor 
		: public Steinberg::Vst::AudioEffect
		, public IScheduledEventListener
	{
		using RingBuff = Steinberg::OneReaderOneWriter::RingBuffer<Steinberg::Vst::Event>;
		using ProcessorParamStorage = PF::ProcessorParamStorage;

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
		void PLUGIN_API chordChanged(const Steinberg::Vst::Chord chord) override;

		// 追加メソッド
		void PLUGIN_API processContext();
		void PLUGIN_API processParameter();
		void PLUGIN_API articulationChanged(int sampleOffset);
		void PLUGIN_API processEvent();
		void PLUGIN_API processInnerEvent();
		void PLUGIN_API routingProcess(Steinberg::Vst::ParamID parmid, Steinberg::Vst::Event event);
		void PLUGIN_API trigAllNotesOff();
		void PLUGIN_API trigBrush(Steinberg::Vst::Event event);
		void PLUGIN_API trigStrum(Steinberg::Vst::Event event, bool isAbove, bool isDown, int maxStrings);
		int getStringPitch(const StringSet& set, int stringNumber);
		void PLUGIN_API trigMute(PARAM trigger, Steinberg::Vst::Event event);
		void PLUGIN_API trigArpeggio(int stringindex, Steinberg::Vst::Event event);
		void PLUGIN_API trigFretNoise(Steinberg::Vst::Event event);
		Steinberg::Vst::ParamID PLUGIN_API getParamIdByPitch(Steinberg::Vst::Event event);
		StringSet PLUGIN_API getTargetStrings(StringSet fretPos, bool isAbove, bool isDown, int maxStrings);
		void PLUGIN_API notifyChordNumberChanged();
		Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage* message) SMTG_OVERRIDE;
		Steinberg::tresult PLUGIN_API notifyStrumTrigger(Steinberg::Vst::IMessage* message);
		void PLUGIN_API processAudio(Steinberg::Vst::ProcessData& data);

		// ストラムイベントスケジューラー
		EventScheduler& scheduler = EventScheduler::Instance();

		// コードマップ
		ChordMap& chordMap = ChordMap::instance();

		// プロセスデータの写し
		Steinberg::Vst::ProcessData* processData{};

		// パラメータキャッシュ値
		ProcessorParamStorage prm;

		// コード変更監視用
		double lastChordNum = 0;

		// コード変更時の平均フレットポジションの移動距離
		float distance = 0.0f;

		// DAWの再生ステータス
		bool isPlaying = false;

		// 内部MIDIイベントバッファ
		RingBuff InnerEvents{ 64 };

		uint32_t chordseq = 0;
	};
}