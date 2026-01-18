//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <array>
#include <cassert>
#include <optional>
#include <pluginterfaces/base/fstrdefs.h>
#include <pluginterfaces/base/ftypes.h>
#include <string>
#include <vector>

#include "chordmap.h"
#include "parameterhelper.h"	

namespace MinMax
{
	// ユニット識別子
	enum UNIT
	{
		SYSTEM = 0,
		CHORD,
		STRUM,
		TRIGGER,
		ARTICULATION,
	};

	// 共通レンジ識別子
	enum RANGE
	{
		MIDI_DATA,
		PITCH,
		MIDI_CHANNEL,
		TRANSPOSE_RANGE,
		FRET_DISTANCE,
		STRUM_STRINGS_RANGE,
		STRING_OFFSET,
		BEAT_LENGTH,
		ARTICULATION_RANGE,
		CTL_TABINDEX1_RANGE,
	};

	// パラメータ識別子
	enum PARAM
	{
		// SYSTEM
		BYPASS = 0,

		// GENERAL
		CHANNEL_SEPALATE = 1001,
		TRANSPOSE,
		SELECTED_ARTICULATION,
		NEED_SAMPLEBLOCK_ADUST,
		CTL_TABINDEX1,
		OCTAVE,
		ENABLED_MUTE_FNOIZE,
		ENABLED_ARTICULATION,

		// コード指定パラメータ
		CHORD_LSB = 1101,		// DAW受信用
		CHORD_MSB,				// DAW受信用
		CHORD_NUM,

		// ストラムパラメータ
		MUTE_CHANNEL = 1201,	// Mute Note送信チャンネル
		MUTE_NOTE_1,
		MUTE_NOTE_2,
	
		STRUM_SPEED = 1211,
		STRUM_DECAY,
		STRUM_LENGTH,
		
		BRUSH_TIME = 1221,
		
		ARP_LENGTH = 1231,
		
		FNOISE_CHANNEL = 1241,	// Fret Noize送信チャンネル
		FNOISE_NOTE_NEAR,
		FNOISE_NOTE_FAR,
		FNOISE_DIST_NEAR,
		FNOISE_DIST_FAR,
		FNOISE_VELOCITY,
		
		STRINGS_UP_HIGH = 1251,
		STRINGS_DOWN_HIGH,
		STRINGS_DOWN_LOW,
		
		STR1_OFFSET = 1261,
		STR2_OFFSET,
		STR3_OFFSET,
		STR4_OFFSET,
		STR5_OFFSET,
		STR6_OFFSET,
		STR7_OFFSET,

		// ストラムキースイッチ
		ALL_NOTES_OFF = 1301,
		BRUSH_UP,
		BRUSH_DOWN,
		UP_HIGH,
		UP,
		DOWN_HIGH,
		DOWN,
		DOWN_LOW,
		MUTE_1,
		MUTE_2,
		ARPEGGIO_1,
		ARPEGGIO_2,
		ARPEGGIO_3,
		ARPEGGIO_4,
		ARPEGGIO_5,
		ARPEGGIO_6,

		// アーティキュレーションキースイッチ
		OPEN1 = 1401,
		OPEN2,
		HAM_LEG,
		MUTE,
		DEAD,
		HARMONICS,
		SLIDE,

		// コード変更通知用特殊パラメータ
		CHORD_STATE_REVISION = 9001,
	};

	// レンジ定義
	class RangeResolver
		: public PF::IRangeResolver
	{
	public:
		bool resolve(Steinberg::uint32 id, PF::ValueRange& out) const override
		{
			switch (static_cast<RANGE>(id))
			{
			case RANGE::MIDI_DATA:           out = { 0, 127, false }; return true;
			case RANGE::PITCH:               out = { 0, 127, false }; return true;
			case RANGE::MIDI_CHANNEL:        out = { 1, 16, false }; return true;
			case RANGE::TRANSPOSE_RANGE:     out = { -6, 6, false }; return true;
			case RANGE::FRET_DISTANCE:       out = { 1, 6, false }; return true;
			case RANGE::STRUM_STRINGS_RANGE: out = { 1, 5, false }; return true;
			case RANGE::STRING_OFFSET:       out = { -5, 4, true }; return true;
			case RANGE::BEAT_LENGTH:         out = { 0.25, 8, false }; return true;
			case RANGE::ARTICULATION_RANGE:  out = { 0, 1, true }; return true;
			case RANGE::CTL_TABINDEX1_RANGE: out = { 0, 1, true }; return true;
			default: return false;
			}
		}
	};

	// オプションリスト定義
	class OptionProvider
		: public PF::IOptionProvider
	{
	public:
		std::vector<std::string> getOptionNames(Steinberg::int32 rangeKind) const override
		{
			switch (rangeKind)
			{
			case RANGE::ARTICULATION_RANGE:
				return { "Open 1", "Open 2", "Hammer/Legato", "Mute", "Dead", "Harmonics", "Slide" };
			case RANGE::STRING_OFFSET:
				return { "Open", "-4 fret", "-3 fret", "-2 fret", "-1 fret", "0", "1 fret", "2 fret", "3 fret", "4 fret" };
			case RANGE::CTL_TABINDEX1_RANGE:
				return { "Strum Parameters", "Setings", "Plugin Info" };
			default:
				return { };
			}
		}
	};

	/***
	 * Parameter設定
	*/
	
	// 全パラメータ数
	inline constexpr size_t PARAM_MAX = 59;

	// 全パラメータ定義
	inline const std::array<PF::ParamDef, PARAM_MAX> paramTable =
	{ {
	{ PARAM::BYPASS, STR16("Bypass"), STR16(""), PF::VALUE::Bool, PF::SCALE::Linear, std::nullopt, PF::FLAG::SYS_BYPASS,                                         UNIT::SYSTEM, 0, 1, 0, 0 },

	{ PARAM::CHANNEL_SEPALATE, STR16("Channel Sepalate"), STR16(""), PF::VALUE::Bool, PF::SCALE::Linear, std::nullopt, PF::FLAG::HIDDEN,                         UNIT::SYSTEM, 0, 1, 0, 0 },
	{ PARAM::TRANSPOSE, STR16("Transpose"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::TRANSPOSE_RANGE, PF::FLAG::AUTOMATE,                            UNIT::SYSTEM, 0, 1, 0, 0 },
	{ PARAM::SELECTED_ARTICULATION, STR16("Selected Articulation"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::ARTICULATION_RANGE, PF::FLAG::AUTOMATE, UNIT::SYSTEM, 0, 1, 0, 0 },
	{ PARAM::NEED_SAMPLEBLOCK_ADUST, STR16("Need Sampleblock Adust"), STR16(""), PF::VALUE::Bool, PF::SCALE::Linear, std::nullopt, PF::FLAG::HIDDEN,             UNIT::SYSTEM, 0, 1, 0, 0 },
	{ PARAM::CTL_TABINDEX1, STR16("TabIndex1"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, CTL_TABINDEX1_RANGE, PF::FLAG::HIDDEN,                             UNIT::SYSTEM, 0, 1, 0, 0 },
	{ PARAM::OCTAVE, STR16("Octabe Up"), STR16(""), PF::VALUE::Bool, PF::SCALE::Linear, std::nullopt, PF::FLAG::HIDDEN,											 UNIT::SYSTEM, 0, 1, 0, 0 },
	{ PARAM::ENABLED_MUTE_FNOIZE, STR16("Enabled Mute/Fret Noize"), STR16(""), PF::VALUE::Bool, PF::SCALE::Linear, std::nullopt, PF::FLAG::HIDDEN,				 UNIT::SYSTEM, 0, 1, 1, 0 },
	{ PARAM::ENABLED_ARTICULATION, STR16("Enabled Articulation"), STR16(""), PF::VALUE::Bool, PF::SCALE::Linear, std::nullopt, PF::FLAG::HIDDEN,				 UNIT::SYSTEM, 0, 1, 1, 0 },

	{ PARAM::CHORD_LSB, STR16("Chord LSB"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::MIDI_DATA, PF::FLAG::HIDDEN,                                    UNIT::CHORD,  0,      1, 0, 0 },
	{ PARAM::CHORD_MSB, STR16("Chord MSB"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::MIDI_DATA, PF::FLAG::HIDDEN,                                    UNIT::CHORD,  0,      1, 0, 0 },
	{ PARAM::CHORD_NUM, STR16("Chord Number"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, std::nullopt, PF::FLAG::HIDDEN,                                     UNIT::CHORD,  0,   1163, 0, 0 },
	{ PARAM::CHORD_STATE_REVISION, STR16("Chord State"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, std::nullopt, PF::FLAG::HIDDEN,                           UNIT::CHORD,  0, 999999, 0, 0 },

	{ PARAM::MUTE_CHANNEL , STR16("Mute Channel"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::MIDI_CHANNEL, PF::FLAG::HIDDEN,                          UNIT::STRUM,  0,    1,   1, 0 },
	{ PARAM::MUTE_NOTE_1, STR16("Mute Note 1"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                    UNIT::STRUM,  0,    1, 103, 0 },
	{ PARAM::MUTE_NOTE_2, STR16("Mite Note 2"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                    UNIT::STRUM,  0,    1, 102, 0 },

	{ PARAM::STRUM_SPEED, STR16("Strum Speed"), STR16("ms"), PF::VALUE::Real, PF::SCALE::Exponential, std::nullopt, PF::FLAG::AUTOMATE,                          UNIT::STRUM,  1, 1000,  26, 1},
	{ PARAM::STRUM_DECAY, STR16("Strum Decay"), STR16("%"), PF::VALUE::Real, PF::SCALE::Linear, std::nullopt, PF::FLAG::AUTOMATE,                                UNIT::STRUM, 85,  100,  96, 1},
	{ PARAM::STRUM_LENGTH, STR16("Strum Length"), STR16("beat"), PF::VALUE::Real, PF::SCALE::Linear, RANGE::BEAT_LENGTH, PF::FLAG::AUTOMATE,                     UNIT::STRUM,  0,    1,   4, 2},

	{ PARAM::BRUSH_TIME, STR16("Brush Time"), STR16("ms"), PF::VALUE::Real, PF::SCALE::Exponential, std::nullopt, PF::FLAG::AUTOMATE,                            UNIT::STRUM,  5,  300,  30, 1},

	{ PARAM::ARP_LENGTH, STR16("Arpegio Length"), STR16("beat"), PF::VALUE::Real, PF::SCALE::Linear, RANGE::BEAT_LENGTH, PF::FLAG::AUTOMATE,                     UNIT::STRUM,  0,    1,   2, 2},

	{ PARAM::FNOISE_CHANNEL, STR16("Fret Noise Channel"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::MIDI_CHANNEL, PF::FLAG::HIDDEN,                   UNIT::STRUM,  0,    1,   1, 0 },
	{ PARAM::FNOISE_NOTE_NEAR, STR16("Fret Noise Note Near"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                      UNIT::STRUM,  0,    1, 120, 0 },
	{ PARAM::FNOISE_NOTE_FAR, STR16("Fret Noise Note Far"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                        UNIT::STRUM,  0,    1, 123, 0 },
	{ PARAM::FNOISE_DIST_NEAR, STR16("Fret Noise Distance Near"), STR16("fret"), PF::VALUE::Int, PF::SCALE::Linear, RANGE::FRET_DISTANCE, PF::FLAG::HIDDEN,      UNIT::STRUM,  0,    1,   1, 0 },
	{ PARAM::FNOISE_DIST_FAR, STR16("Fret Noise Distance Far"), STR16("fret"), PF::VALUE::Int, PF::SCALE::Linear, RANGE::FRET_DISTANCE, PF::FLAG::HIDDEN,        UNIT::STRUM,  0,    1,   3, 0 },
	{ PARAM::FNOISE_VELOCITY, STR16("Fret Noise Velocity"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::MIDI_DATA, PF::FLAG::AUTOMATE,                  UNIT::STRUM,  0,  127, 100, 0 },

	{ PARAM::STRINGS_UP_HIGH, STR16("Strings Up High"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::STRUM_STRINGS_RANGE, PF::FLAG::AUTOMATE,            UNIT::STRUM,  0,    1,   4, 0 },
	{ PARAM::STRINGS_DOWN_HIGH, STR16("Strings Down High"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::STRUM_STRINGS_RANGE, PF::FLAG::AUTOMATE,        UNIT::STRUM,  0,    1,   3, 0 },
	{ PARAM::STRINGS_DOWN_LOW, STR16("Strings Down Low"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::STRUM_STRINGS_RANGE, PF::FLAG::AUTOMATE,          UNIT::STRUM,  0,    1,   1, 0 },

	{ PARAM::STR1_OFFSET, STR16("String 1 Offset"), STR16("fret"), PF::VALUE::Int, PF::SCALE::Linear, RANGE::STRING_OFFSET, PF::FLAG::AUTOMATE,                  UNIT::STRUM,  0,    1,   5, 0 },
	{ PARAM::STR2_OFFSET, STR16("String 2 Offset"), STR16("fret"), PF::VALUE::Int, PF::SCALE::Linear, RANGE::STRING_OFFSET, PF::FLAG::AUTOMATE,                  UNIT::STRUM,  0,    1,   5, 0 },
	{ PARAM::STR3_OFFSET, STR16("String 3 Offset"), STR16("fret"), PF::VALUE::Int, PF::SCALE::Linear, RANGE::STRING_OFFSET, PF::FLAG::AUTOMATE,                  UNIT::STRUM,  0,    1,   5, 0 },
	{ PARAM::STR4_OFFSET, STR16("String 4 Offset"), STR16("fret"), PF::VALUE::Int, PF::SCALE::Linear, RANGE::STRING_OFFSET, PF::FLAG::AUTOMATE,                  UNIT::STRUM,  0,    1,   5, 0 },
	{ PARAM::STR5_OFFSET, STR16("String 5 Offset"), STR16("fret"), PF::VALUE::Int, PF::SCALE::Linear, RANGE::STRING_OFFSET, PF::FLAG::AUTOMATE,                  UNIT::STRUM,  0,    1,   5, 0 },
	{ PARAM::STR6_OFFSET, STR16("String 6 Offset"), STR16("fret"), PF::VALUE::Int, PF::SCALE::Linear, RANGE::STRING_OFFSET, PF::FLAG::AUTOMATE,                  UNIT::STRUM,  0,    1,   5, 0 },
	{ PARAM::STR7_OFFSET, STR16("String 7 Offset"), STR16("fret"), PF::VALUE::Int, PF::SCALE::Linear, RANGE::STRING_OFFSET, PF::FLAG::AUTOMATE,                  UNIT::STRUM,  0,    1,   5, 0 },

	{ PARAM::ALL_NOTES_OFF , STR16("All Notes Off"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                               UNIT::TRIGGER, 0, 1, 77, 0 },
	{ PARAM::BRUSH_DOWN, STR16("Brush"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                           UNIT::TRIGGER, 0, 1, 74, 0 },
	{ PARAM::UP_HIGH, STR16("Up High"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                            UNIT::TRIGGER, 0, 1, 63, 0 },
	{ PARAM::UP, STR16("Up"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                                      UNIT::TRIGGER, 0, 1, 62, 0 },
	{ PARAM::DOWN_HIGH, STR16("Down High"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                        UNIT::TRIGGER, 0, 1, 61, 0 },
	{ PARAM::DOWN, STR16("Down"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                                  UNIT::TRIGGER, 0, 1, 60, 0 },
	{ PARAM::DOWN_LOW, STR16("Down Low"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                          UNIT::TRIGGER, 0, 1, 59, 0 },
	{ PARAM::MUTE_1, STR16("Mute 1"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                              UNIT::TRIGGER, 0, 1, 58, 0 },
	{ PARAM::MUTE_2, STR16("Mute 2"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                              UNIT::TRIGGER, 0, 1, 57, 0 },
	{ PARAM::ARPEGGIO_1, STR16("String 1"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                        UNIT::TRIGGER, 0, 1, 72, 0 },
	{ PARAM::ARPEGGIO_2, STR16("String 2"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                        UNIT::TRIGGER, 0, 1, 71, 0 },
	{ PARAM::ARPEGGIO_3, STR16("String 3"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                        UNIT::TRIGGER, 0, 1, 69, 0 },
	{ PARAM::ARPEGGIO_4, STR16("String 4"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                        UNIT::TRIGGER, 0, 1, 67, 0 },
	{ PARAM::ARPEGGIO_5, STR16("String 5"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                        UNIT::TRIGGER, 0, 1, 65, 0 },
	{ PARAM::ARPEGGIO_6, STR16("String 6"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                        UNIT::TRIGGER, 0, 1, 64, 0 },
		
	{ PARAM::OPEN1 , STR16("Open 1"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                              UNIT::ARTICULATION, 0, 1, 24, 0 },
	{ PARAM::OPEN2, STR16("Open 2"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                               UNIT::ARTICULATION, 0, 1,  0, 0 },
	{ PARAM::HAM_LEG, STR16("Hammer/Legato"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                      UNIT::ARTICULATION, 0, 1, 25, 0 },
	{ PARAM::MUTE, STR16("Mute"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                                  UNIT::ARTICULATION, 0, 1, 26, 0 },
	{ PARAM::DEAD, STR16("Dead"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                                  UNIT::ARTICULATION, 0, 1, 27, 0 },
	{ PARAM::HARMONICS, STR16("Harmonics"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                        UNIT::ARTICULATION, 0, 1, 28, 0 },
	{ PARAM::SLIDE, STR16("Slide"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, RANGE::PITCH, PF::FLAG::HIDDEN,                                                UNIT::ARTICULATION, 0, 1, 33, 0 },	
	} };

	// ユニット指定によるパラメータ定義取り出し
	template<size_t N>
	inline bool getParamsByUnit(std::array<const PF::ParamDef*, N>& outResult, size_t& outCount, UNIT targetUnit)
	{
		outCount = 0;

		for (const auto& param : paramTable)
		{
			if (param.unitID == targetUnit)
			{
				assert(outCount < outResult.size());
				outResult[outCount++] = &param;
			}
		}

		return outCount > 0;
	}

	// トリガー系
	constexpr size_t PARAM_TRIGGER_COUNT = 16;
	static_assert(PARAM_TRIGGER_COUNT <= paramTable.size(), "Trigger param count mismatch");

	inline bool getTriggerParams(std::array<const PF::ParamDef*, PARAM_TRIGGER_COUNT>& outResult, size_t& outCount)
	{
		return getParamsByUnit(outResult, outCount, UNIT::TRIGGER);
	}

	// アーティキュレーション系
	constexpr size_t PARAM_ARTICULATION_COUNT = 7;
	static_assert(PARAM_ARTICULATION_COUNT <= paramTable.size(), "Articulation param count mismatch");

	inline bool getArticulationParams(std::array<const PF::ParamDef*, PARAM_ARTICULATION_COUNT>& outResult, size_t& outCount)
	{
		return getParamsByUnit(outResult, outCount, UNIT::ARTICULATION);
	}

	// パラメータヘルパー初期化
	inline const void initParameters()
	{
		static RangeResolver rangeResolver;
		static OptionProvider optionProvider;

		auto& helper = PF::ParamHelper::get();

		ChordMap::instance().loadChordPreset();

		helper.setKindResolver(&rangeResolver);
		helper.setOptionProvider(&optionProvider);
	}
}