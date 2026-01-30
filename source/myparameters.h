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
		U_SYSTEM = 0,
		U_GENERAL,
		U_CHORD,
		U_STRUM,
		U_BRUSH,
		U_ARP,
		U_STRINGS,
		U_MUTE,
		U_NOIZE,
		U_OFFSET,
		U_TRIGGER,
		U_ARTIC,
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
		CHANNEL_SEPARATE = 1001,
		TRANSPOSE,
		SELECTED_ARTICULATION,
		NEED_SAMPLEBLOCK_ADJUST,
		CTL_TABINDEX1,
		OCTAVE,
		ENABLED_MUTE,
		ENABLED_NOIZE,
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
		
		BRUSH_DECAY = 1221,
		BRUSH_DOWN_TIME,
		BRUSH_UP_TIME,

		ARP_LENGTH = 1231,
		
		NOISE_CHANNEL = 1241,	// Fret Noize送信チャンネル
		NOISE_NOTE_NEAR,
		NOISE_NOTE_FAR,
		NOISE_DIST_NEAR,
		NOISE_DIST_FAR,
		NOISE_VELOCITY,
		
		STRINGS_UP_HIGH = 1251,
		STRINGS_UP_LOW,
		STRINGS_DOWN_HIGH,
		STRINGS_DOWN_LOW,
		STRINGS_DOWN,
		STRINGS_UP,

		STR1_OFFSET = 1261,
		STR2_OFFSET,
		STR3_OFFSET,
		STR4_OFFSET,
		STR5_OFFSET,
		STR6_OFFSET,
		STR7_OFFSET,

		// ストラムキースイッチ
		ALL_NOTES_OFF = 1301,
		BRUSH_DOWN,
		BRUSH_UP,
		UP_HIGH,
		UP,
		UP_LOW,
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
			case MIDI_DATA:           out = {    0, 127, false }; return true;
			case PITCH:               out = {    0, 127, false }; return true;
			case MIDI_CHANNEL:        out = {    1,  16, true  }; return true;
			case TRANSPOSE_RANGE:     out = {   -6,   6, true  }; return true;
			case FRET_DISTANCE:       out = {    1,   6, true  }; return true;
			case STRUM_STRINGS_RANGE: out = {    1,   6, true  }; return true;
			case STRING_OFFSET:       out = {   -6,   4, true  }; return true;
			case BEAT_LENGTH:         out = { 0.25,   8, false }; return true;
			case ARTICULATION_RANGE:  out = {    0,   1, true  }; return true;
			case CTL_TABINDEX1_RANGE: out = {    0,   1, true  }; return true;
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
			case MIDI_CHANNEL:
				return { "Ch 1", "Ch 2", "Ch 3", "Ch 4", "Ch 5", "Ch 6", "Ch 7", "Ch 8", "Ch 9", "Ch10", "Ch11", "Ch12", "Ch13", "Ch14", "Ch15", "Ch16" };
			case TRANSPOSE_RANGE:
				return { "-6", "-5", "-4", "-3", "-2", "-1", "0", "1", "2", "3", "4", "5", "6"};
			case FRET_DISTANCE:
				return { "1", "2", "3", "4", "5", "6" };
			case STRUM_STRINGS_RANGE:
				return { "1", "2", "3", "4", "5", "Max" };
			case STRING_OFFSET:
				return { "Mute", "Open", "-4 fret", "-3 fret", "-2 fret", "-1 fret", "0", "1 fret", "2 fret", "3 fret", "4 fret" };
			case ARTICULATION_RANGE:
				return { "Open 1", "Open 2", "Hammer/Legato", "Mute", "Dead", "Harmonics", "Slide" };
			case CTL_TABINDEX1_RANGE:
				return { "Strum Parameters", "Settings", "Plugin Info" };
			default:
				return { };
			}
		}
	};

	/***
	 * Parameter設定
	*/
	
	// 全パラメータ数
	inline constexpr size_t PARAM_MAX = 67;

	// 全パラメータ定義
	inline const std::array<PF::ParamDef, PARAM_MAX> paramTable =
	{ {
	{ BYPASS, STR16("Bypass"), STR16(""), PF::VALUE::Bool, PF::SCALE::Linear, std::nullopt, PF::FLAG::FLAG_SYS_BYPASS,									U_SYSTEM, 0, 1, 0, 0 },

	{ CHANNEL_SEPARATE, STR16("Channel Separate"), STR16(""), PF::VALUE::Bool, PF::SCALE::Linear, std::nullopt, PF::FLAG::FLAG_HIDDEN,					U_GENERAL, 0, 1, 0, 0 },
	{ TRANSPOSE, STR16("Transpose"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, TRANSPOSE_RANGE, PF::FLAG::FLAG_AUTOMATE,							U_GENERAL, 0, 1, 6, 0 },
	{ SELECTED_ARTICULATION, STR16("Selected Articulation"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, ARTICULATION_RANGE, PF::FLAG::FLAG_AUTOMATE,	U_GENERAL, 0, 1, 0, 0 },
	{ NEED_SAMPLEBLOCK_ADJUST, STR16("Need Sampleblock Adjust"), STR16(""), PF::VALUE::Bool, PF::SCALE::Linear, std::nullopt, PF::FLAG::FLAG_HIDDEN,	U_GENERAL, 0, 1, 0, 0 },
	{ CTL_TABINDEX1, STR16("TabIndex1"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, CTL_TABINDEX1_RANGE, PF::FLAG::FLAG_HIDDEN,						U_GENERAL, 0, 1, 0, 0 },
	{ OCTAVE, STR16("Octave+"), STR16(""), PF::VALUE::Bool, PF::SCALE::Linear, std::nullopt, PF::FLAG::FLAG_HIDDEN,										U_GENERAL, 0, 1, 0, 0 },
	{ ENABLED_MUTE, STR16("Enabled Mute/Fret Noize"), STR16(""), PF::VALUE::Bool, PF::SCALE::Linear, std::nullopt, PF::FLAG::FLAG_HIDDEN,				U_GENERAL, 0, 1, 1, 0 },
	{ ENABLED_NOIZE, STR16("Enabled Mute/Fret Noize"), STR16(""), PF::VALUE::Bool, PF::SCALE::Linear, std::nullopt, PF::FLAG::FLAG_HIDDEN,				U_GENERAL, 0, 1, 1, 0 },
	{ ENABLED_ARTICULATION, STR16("Enabled Articulation"), STR16(""), PF::VALUE::Bool, PF::SCALE::Linear, std::nullopt, PF::FLAG::FLAG_HIDDEN,			U_GENERAL, 0, 1, 1, 0 },

	{ CHORD_LSB, STR16("Chord LSB"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, MIDI_DATA, PF::FLAG::FLAG_HIDDEN,									U_CHORD,  0,      1, 0, 0 },
	{ CHORD_MSB, STR16("Chord MSB"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, MIDI_DATA, PF::FLAG::FLAG_HIDDEN,									U_CHORD,  0,      1, 0, 0 },
	{ CHORD_NUM, STR16("Chord Number"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, std::nullopt, PF::FLAG::FLAG_HIDDEN,                              U_CHORD,  0,   1163, 0, 0 },
	{ CHORD_STATE_REVISION, STR16("Chord State"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, std::nullopt, PF::FLAG::FLAG_HIDDEN,                    U_CHORD,  0, 999999, 0, 0 },

	{ STRUM_SPEED, STR16("Strum Speed"), STR16("ms"), PF::VALUE::Real, PF::SCALE::Exponential, std::nullopt, PF::FLAG::FLAG_AUTOMATE,                   U_STRUM,  1, 1000,  26, 1},
	{ STRUM_DECAY, STR16("Strum Decay"), STR16("%"), PF::VALUE::Real, PF::SCALE::Linear, std::nullopt, PF::FLAG::FLAG_AUTOMATE,                         U_STRUM, 85,  100,  94, 1},
	{ STRUM_LENGTH, STR16("Strum Length"), STR16("beat"), PF::VALUE::Real, PF::SCALE::Linear, BEAT_LENGTH, PF::FLAG::FLAG_AUTOMATE,						U_STRUM,  0,    1,   4, 2},

	{ BRUSH_DECAY, STR16("Brush Decay"), STR16("%"), PF::VALUE::Real, PF::SCALE::Linear, std::nullopt, PF::FLAG::FLAG_AUTOMATE,							U_BRUSH, 85,  100,  98, 1},
	{ BRUSH_DOWN_TIME, STR16("Brush Down Time"), STR16("ms"), PF::VALUE::Real, PF::SCALE::Exponential, std::nullopt, PF::FLAG::FLAG_AUTOMATE,           U_BRUSH,  5,  300,  10, 1},
	{ BRUSH_UP_TIME, STR16("Brush Up Time"), STR16("ms"), PF::VALUE::Real, PF::SCALE::Exponential, std::nullopt, PF::FLAG::FLAG_AUTOMATE,               U_BRUSH,  5,  300,  80, 1},

	{ ARP_LENGTH, STR16("Arpeggio Length"), STR16("beat"), PF::VALUE::Real, PF::SCALE::Linear, BEAT_LENGTH, PF::FLAG::FLAG_AUTOMATE,					U_ARP, 0, 1, 2, 2},

	{ MUTE_CHANNEL , STR16("Mute Channel"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, MIDI_CHANNEL, PF::FLAG::FLAG_HIDDEN,							U_MUTE, 0, 1,   0, 0 },
	{ MUTE_NOTE_1, STR16("Mute Note 1"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,									U_MUTE, 0, 1, 103, 0 },
	{ MUTE_NOTE_2, STR16("Mute Note 2"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,									U_MUTE, 0, 1, 102, 0 },

	{ NOISE_CHANNEL, STR16("Fret Noise Channel"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, MIDI_CHANNEL, PF::FLAG::FLAG_HIDDEN,					U_NOIZE, 0,   1,   0, 0 },
	{ NOISE_NOTE_NEAR, STR16("Fret Noise Note Near"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,						U_NOIZE, 0,   1, 120, 0 },
	{ NOISE_NOTE_FAR, STR16("Fret Noise Note Far"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,							U_NOIZE, 0,   1, 123, 0 },
	{ NOISE_DIST_NEAR, STR16("Fret Noise Distance Near"), STR16("fret"), PF::VALUE::Int, PF::SCALE::Linear, FRET_DISTANCE, PF::FLAG::FLAG_HIDDEN,		U_NOIZE, 0,   1,   0, 0 },
	{ NOISE_DIST_FAR, STR16("Fret Noise Distance Far"), STR16("fret"), PF::VALUE::Int, PF::SCALE::Linear, FRET_DISTANCE, PF::FLAG::FLAG_HIDDEN,			U_NOIZE, 0,   1,   2, 0 },
	{ NOISE_VELOCITY, STR16("Fret Noise Velocity"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, MIDI_DATA, PF::FLAG::FLAG_AUTOMATE,					U_NOIZE, 0, 127, 100, 0 },

	{ STRINGS_UP_HIGH, STR16("Strings Up High"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, STRUM_STRINGS_RANGE, PF::FLAG::FLAG_AUTOMATE,			U_STRINGS, 0, 1, 3 - 1, 0 },
	{ STRINGS_UP_LOW, STR16("Strings Up Low"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, STRUM_STRINGS_RANGE, PF::FLAG::FLAG_AUTOMATE,				U_STRINGS, 0, 1, 2 - 1, 0 },
	{ STRINGS_DOWN_HIGH, STR16("Strings Down High"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, STRUM_STRINGS_RANGE, PF::FLAG::FLAG_AUTOMATE,		U_STRINGS, 0, 1, 3 - 1, 0 },
	{ STRINGS_DOWN_LOW, STR16("Strings Down Low"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, STRUM_STRINGS_RANGE, PF::FLAG::FLAG_AUTOMATE,			U_STRINGS, 0, 1, 1 - 1, 0 },
	{ STRINGS_DOWN, STR16("Strings Brush Down"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, STRUM_STRINGS_RANGE, PF::FLAG::FLAG_AUTOMATE,			U_STRINGS, 0, 1, MAX_STRINGS - 1, 0 },
	{ STRINGS_UP, STR16("Strings Brush Up"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, STRUM_STRINGS_RANGE, PF::FLAG::FLAG_AUTOMATE,				U_STRINGS, 0, 1, MAX_STRINGS - 1, 0 },

	{ STR1_OFFSET, STR16("String 1 Offset"), STR16("fret"), PF::VALUE::Int, PF::SCALE::Linear, STRING_OFFSET, PF::FLAG::FLAG_AUTOMATE,					U_OFFSET, 0, 1, StringSet::CENTER_OFFSET, 0 },
	{ STR2_OFFSET, STR16("String 2 Offset"), STR16("fret"), PF::VALUE::Int, PF::SCALE::Linear, STRING_OFFSET, PF::FLAG::FLAG_AUTOMATE,					U_OFFSET, 0, 1, StringSet::CENTER_OFFSET, 0 },
	{ STR3_OFFSET, STR16("String 3 Offset"), STR16("fret"), PF::VALUE::Int, PF::SCALE::Linear, STRING_OFFSET, PF::FLAG::FLAG_AUTOMATE,					U_OFFSET, 0, 1, StringSet::CENTER_OFFSET, 0 },
	{ STR4_OFFSET, STR16("String 4 Offset"), STR16("fret"), PF::VALUE::Int, PF::SCALE::Linear, STRING_OFFSET, PF::FLAG::FLAG_AUTOMATE,					U_OFFSET, 0, 1, StringSet::CENTER_OFFSET, 0 },
	{ STR5_OFFSET, STR16("String 5 Offset"), STR16("fret"), PF::VALUE::Int, PF::SCALE::Linear, STRING_OFFSET, PF::FLAG::FLAG_AUTOMATE,					U_OFFSET, 0, 1, StringSet::CENTER_OFFSET, 0 },
	{ STR6_OFFSET, STR16("String 6 Offset"), STR16("fret"), PF::VALUE::Int, PF::SCALE::Linear, STRING_OFFSET, PF::FLAG::FLAG_AUTOMATE,					U_OFFSET, 0, 1, StringSet::CENTER_OFFSET, 0 },
	{ STR7_OFFSET, STR16("String 7 Offset"), STR16("fret"), PF::VALUE::Int, PF::SCALE::Linear, STRING_OFFSET, PF::FLAG::FLAG_AUTOMATE,					U_OFFSET, 0, 1, StringSet::CENTER_OFFSET, 0 },

	{ ALL_NOTES_OFF , STR16("All Notes Off"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,								U_TRIGGER, 0, 1, 53, 0 },
	{ BRUSH_DOWN, STR16("Brush Down"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,										U_TRIGGER, 0, 1, 63, 0 },
	{ BRUSH_UP, STR16("Brush Up"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,											U_TRIGGER, 0, 1, 61, 0 },
	{ UP_HIGH, STR16("Up High"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,											U_TRIGGER, 0, 1, 64, 0 },
	{ UP, STR16("Up"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,														U_TRIGGER, 0, 1, 62, 0 },
	{ UP_LOW, STR16("Up Low"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,												U_TRIGGER, 0, 1, 60, 0 },
	{ DOWN_HIGH, STR16("Down High"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,										U_TRIGGER, 0, 1, 59, 0 },
	{ DOWN, STR16("Down"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,													U_TRIGGER, 0, 1, 57, 0 },
	{ DOWN_LOW, STR16("Down Low"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,											U_TRIGGER, 0, 1, 55, 0 },
	{ MUTE_1, STR16("Mute 1"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,												U_TRIGGER, 0, 1, 58, 0 },
	{ MUTE_2, STR16("Mute 2"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,												U_TRIGGER, 0, 1, 56, 0 },
	{ ARPEGGIO_1, STR16("String 1"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,										U_TRIGGER, 0, 1, 52, 0 },
	{ ARPEGGIO_2, STR16("String 2"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,										U_TRIGGER, 0, 1, 50, 0 },
	{ ARPEGGIO_3, STR16("String 3"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,										U_TRIGGER, 0, 1, 48, 0 },
	{ ARPEGGIO_4, STR16("String 4"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,										U_TRIGGER, 0, 1, 47, 0 },
	{ ARPEGGIO_5, STR16("String 5"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,										U_TRIGGER, 0, 1, 45, 0 },
	{ ARPEGGIO_6, STR16("String 6"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,										U_TRIGGER, 0, 1, 43, 0 },
		
	{ OPEN1 , STR16("Open 1"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,												U_ARTIC, 0, 1, 24, 0 },
	{ OPEN2, STR16("Open 2"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,												U_ARTIC, 0, 1,  0, 0 },
	{ HAM_LEG, STR16("Hammer/Legato"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,										U_ARTIC, 0, 1, 25, 0 },
	{ MUTE, STR16("Mute"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,													U_ARTIC, 0, 1, 26, 0 },
	{ DEAD, STR16("Dead"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,													U_ARTIC, 0, 1, 27, 0 },
	{ HARMONICS, STR16("Harmonics"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,										U_ARTIC, 0, 1, 28, 0 },
	{ SLIDE, STR16("Slide"), STR16(""), PF::VALUE::Int, PF::SCALE::Linear, PITCH, PF::FLAG::FLAG_HIDDEN,												U_ARTIC, 0, 1, 33, 0 },	
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
	constexpr size_t PARAM_TRIGGER_COUNT = 17;
	static_assert(PARAM_TRIGGER_COUNT <= paramTable.size(), "Trigger param count mismatch");

	inline bool getTriggerParams(std::array<const PF::ParamDef*, PARAM_TRIGGER_COUNT>& outResult, size_t& outCount)
	{
		return getParamsByUnit(outResult, outCount, U_TRIGGER);
	}

	// アーティキュレーション系
	constexpr size_t PARAM_ARTICULATION_COUNT = 7;
	static_assert(PARAM_ARTICULATION_COUNT <= paramTable.size(), "Articulation param count mismatch");

	inline bool getArticulationParams(std::array<const PF::ParamDef*, PARAM_ARTICULATION_COUNT>& outResult, size_t& outCount)
	{
		return getParamsByUnit(outResult, outCount, U_ARTIC);
	}
}