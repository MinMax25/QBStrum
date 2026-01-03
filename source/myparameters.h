#pragma once

#include <array>
#include <cassert>
#include <pluginterfaces/base/fstrdefs.h>
#include "parameterframework.h"	
#include "chordmap.h"

namespace MinMax
{
	using namespace Steinberg;
	using namespace Steinberg::Vst;
	using namespace ParameterFramework;

	using ParamHelper = ParameterFramework::ParamHelper;

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
		BEAT_LENGTH,
		ARTICULATION_RANGE,
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

		// CHORD
		CHORD_LSB = 1101,	// DAW受信用
		CHORD_MSB,			// DAW受信用
		CHORD_NUM,

		// STRUM
		MUTE_CHANNEL = 1201,
		MUTE_NOTE_1,
		MUTE_NOTE_2,
		STRUM_SPEED = 1211,
		STRUM_DECAY,
		STRUM_LENGTH,
		BRUSH_TIME = 1221,
		ARP_LENGTH = 1231,
		FNOISE_CHANNEL = 1241,
		FNOISE_NOTE_NEAR,
		FNOISE_NOTE_FAR,
		FNOISE_DIST_NEAR,
		FNOISE_DIST_FAR,
		FNOISE_VELOCITY,
		STRINGS_UP_HIGH = 1251,
		STRINGS_DOWN_HIGH,
		STRINGS_DOWN_LOW,

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

		// ARTICULATION
		OPEN1 = 1401,
		OPEN2,
		HAM_LEG,
		MUTE,
		DEAD,
		HARMONICS,
		SLIDE,
	};

	// 共通レンジ定義取得
	class RangeResolver
		: public ParameterFramework::IRangeResolver
	{
	public:
		bool resolve(uint32 id, ValueRange& out) const override
		{
			switch (static_cast<RANGE>(id))
			{
			case RANGE::MIDI_DATA:           out = { 0, 127, false }; return true;
			case RANGE::PITCH:               out = { 0, 127, false }; return true;
			case RANGE::MIDI_CHANNEL:        out = { 1, 16, false }; return true;
			case RANGE::TRANSPOSE_RANGE:     out = { -12, 12, false }; return true;
			case RANGE::FRET_DISTANCE:       out = { 1, 6, false }; return true;
			case RANGE::STRUM_STRINGS_RANGE: out = { 1, 5, false }; return true;
			case RANGE::BEAT_LENGTH:         out = { 1, 8, false }; return true;
			case RANGE::ARTICULATION_RANGE:  out = { 0, 1, true }; return true;
			default: return false;
			}
		}
	};

	// オプションリスト取得
	class OptionProvider
		: public ParameterFramework::IOptionProvider
	{
	public:
		OptionList getOptionNames(int32 rangeKind) const override
		{
			switch (rangeKind)
			{
			case RANGE::ARTICULATION_RANGE:
				return { "Open 1", "Open 2", "Hammer/Legato", "Mute", "Dead", "Harmonics", "Slide" };
			default:
				return { };
			}
		}
	};

	/***
	 * Parameter設定
	*/
	
	// 全パラメータ数
	inline constexpr size_t PARAM_MAX = 48;

	// 全パラメータ定義
	inline const std::array<ParamDef, PARAM_MAX> paramTable =
	{ 
		{
			{ PARAM::BYPASS, STR16("Bypass"), STR16(""), VALUE::Bool, SCALE::Linear, std::nullopt, FLAG::SYS_BYPASS, UNIT::SYSTEM, 0, 1, 0, 0, 0 },
			{ PARAM::CHANNEL_SEPALATE, STR16("Channel Sepalate"), STR16(""), VALUE::Bool, SCALE::Linear, std::nullopt, FLAG::HIDDEN, UNIT::SYSTEM, 0, 1, 0, 0, 0 },
			{ PARAM::TRANSPOSE, STR16("Transpose"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::TRANSPOSE_RANGE, FLAG::AUTOMATE, UNIT::SYSTEM, 0, 1, 0, 0, 0 },
			{ PARAM::SELECTED_ARTICULATION, STR16("Selected Articulation"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::ARTICULATION_RANGE, FLAG::AUTOMATE, UNIT::SYSTEM, 0, 1, 0, 0, 0 },
			{ PARAM::NEED_SAMPLEBLOCK_ADUST, STR16("Need Sampleblock Adust"), STR16(""), VALUE::Bool, SCALE::Linear, std::nullopt, FLAG::HIDDEN, UNIT::SYSTEM, 0, 1, 0, 0, 0 },
			{ PARAM::CHORD_LSB, STR16("Chord LSB"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::MIDI_DATA, FLAG::HIDDEN, UNIT::CHORD, 0, 1, 0, 0, 0 },
			{ PARAM::CHORD_MSB, STR16("Chord MSB"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::MIDI_DATA, FLAG::HIDDEN, UNIT::CHORD, 0, 1, 0, 0, 0 },
			{ PARAM::CHORD_NUM, STR16("Chord Number"), STR16(""), VALUE::Int, SCALE::Linear, std::nullopt, FLAG::HIDDEN, UNIT::CHORD, 0, 1199, 0, 0, 0 },
			{ PARAM::MUTE_CHANNEL , STR16("Mute Channel"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::MIDI_CHANNEL, FLAG::HIDDEN, UNIT::STRUM, 0, 1, 1, 0, 0 },
			{ PARAM::MUTE_NOTE_1, STR16("Mute Note 1"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN, UNIT::STRUM, 0, 1, 103, 0, 0 },
			{ PARAM::MUTE_NOTE_2, STR16("Mite Note 2"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN, UNIT::STRUM, 0, 1, 102, 0, 0 },
			{ PARAM::STRUM_SPEED, STR16("Strum Speed"), STR16("ms"), VALUE::Real, SCALE::Exponential, std::nullopt, FLAG::AUTOMATE, UNIT::STRUM, 1, 1000, 26, 1, 0 },
			{ PARAM::STRUM_DECAY, STR16("Strum Decay"), STR16("%"), VALUE::Int, SCALE::Linear, std::nullopt, FLAG::AUTOMATE, UNIT::STRUM, 85, 100, 96, 1, 0 },
			{ PARAM::STRUM_LENGTH, STR16("Strum Length"), STR16("beat"), VALUE::Int, SCALE::Linear, RANGE::BEAT_LENGTH, FLAG::AUTOMATE, UNIT::STRUM, 0, 1, 4, 0, 0 },
			{ PARAM::BRUSH_TIME, STR16("Brush Time"), STR16("ms"), VALUE::Real, SCALE::Exponential, std::nullopt, FLAG::AUTOMATE, UNIT::STRUM, 20, 300, 30, 1, 0 },
			{ PARAM::ARP_LENGTH, STR16("Arpegio Length"), STR16("beat"), VALUE::Int, SCALE::Linear, RANGE::BEAT_LENGTH, FLAG::AUTOMATE, UNIT::STRUM, 0, 1, 2, 0, 0 },
			{ PARAM::FNOISE_CHANNEL, STR16("Fret Noise Channel"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::MIDI_CHANNEL, FLAG::HIDDEN, UNIT::STRUM, 0, 1, 1, 0, 0 },
			{ PARAM::FNOISE_NOTE_NEAR, STR16("Fret Noise Note Near"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN, UNIT::STRUM, 0, 1, 120, 0, 0 },
			{ PARAM::FNOISE_NOTE_FAR, STR16("Fret Noise Note Far"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN, UNIT::STRUM, 0, 1, 123, 0, 0 },
			{ PARAM::FNOISE_DIST_NEAR, STR16("Fret Noise Distance Near"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::FRET_DISTANCE, FLAG::HIDDEN, UNIT::STRUM, 0, 1, 1, 0, 0 },
			{ PARAM::FNOISE_DIST_FAR, STR16("Fret Noise Distance Far"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::FRET_DISTANCE, FLAG::HIDDEN, UNIT::STRUM, 0, 1, 3, 0, 0 },
			{ PARAM::FNOISE_VELOCITY, STR16("Fret Noise Velocity"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::MIDI_DATA, FLAG::AUTOMATE, UNIT::STRUM, 0, 127, 100, 0, 0 },
			{ PARAM::STRINGS_UP_HIGH, STR16("Strings Up High"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::STRUM_STRINGS_RANGE, FLAG::AUTOMATE, UNIT::STRUM, 0, 1, 4, 0, 0 },
			{ PARAM::STRINGS_DOWN_HIGH, STR16("Strings Down High"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::STRUM_STRINGS_RANGE, FLAG::AUTOMATE, UNIT::STRUM, 0, 1, 3, 0, 0 },
			{ PARAM::STRINGS_DOWN_LOW, STR16("Strings Down Low"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::STRUM_STRINGS_RANGE, FLAG::AUTOMATE, UNIT::STRUM, 0, 1, 1, 0, 0 },
			{ PARAM::ALL_NOTES_OFF , STR16("All Notes Off"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN, UNIT::TRIGGER, 0, 1, 77, 0, 0 },
			{ PARAM::BRUSH_UP, STR16("Brush Up"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN,            UNIT::TRIGGER, 0, 1, 76, 0, 0 },
			{ PARAM::BRUSH_DOWN, STR16("Brush Down"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN,        UNIT::TRIGGER, 0, 1, 74, 0, 0 },
			{ PARAM::UP_HIGH, STR16("Up High"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN,              UNIT::TRIGGER, 0, 1, 63, 0, 0 },
			{ PARAM::UP, STR16("Up"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN,                        UNIT::TRIGGER, 0, 1, 62, 0, 0 },
			{ PARAM::DOWN_HIGH, STR16("Down High"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN,          UNIT::TRIGGER, 0, 1, 61, 0, 0 },
			{ PARAM::DOWN, STR16("Down"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN,                    UNIT::TRIGGER, 0, 1, 60, 0, 0 },
			{ PARAM::DOWN_LOW, STR16("Down Low"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN,            UNIT::TRIGGER, 0, 1, 59, 0, 0 },
			{ PARAM::MUTE_1, STR16("Mute 1"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN,                UNIT::TRIGGER, 0, 1, 58, 0, 0 },
			{ PARAM::MUTE_2, STR16("Mute 2"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN,                UNIT::TRIGGER, 0, 1, 57, 0, 0 },
			{ PARAM::ARPEGGIO_1, STR16("String 1"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN,          UNIT::TRIGGER, 0, 1, 72, 0, 0 },
			{ PARAM::ARPEGGIO_2, STR16("String 2"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN,          UNIT::TRIGGER, 0, 1, 71, 0, 0 },
			{ PARAM::ARPEGGIO_3, STR16("String 3"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN,          UNIT::TRIGGER, 0, 1, 69, 0, 0 },
			{ PARAM::ARPEGGIO_4, STR16("String 4"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN,          UNIT::TRIGGER, 0, 1, 67, 0, 0 },
			{ PARAM::ARPEGGIO_5, STR16("String 5"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN,          UNIT::TRIGGER, 0, 1, 65, 0, 0 },
			{ PARAM::ARPEGGIO_6, STR16("String 6"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN,          UNIT::TRIGGER, 0, 1, 64, 0, 0 },
			{ PARAM::OPEN1 , STR16("Open 1"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN, UNIT::ARTICULATION, 0, 1, 24, 0, 0 },
			{ PARAM::OPEN2, STR16("Open 2"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN, UNIT::ARTICULATION, 0, 1, 0, 0, 0 },
			{ PARAM::HAM_LEG, STR16("Hammer/Legato"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN, UNIT::ARTICULATION, 0, 1, 25, 0, 0 },
			{ PARAM::MUTE, STR16("Mute"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN, UNIT::ARTICULATION, 0, 1, 26, 0, 0 },
			{ PARAM::DEAD, STR16("Dead"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN, UNIT::ARTICULATION, 0, 1, 27, 0, 0 },
			{ PARAM::HARMONICS, STR16("Harmonics"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN, UNIT::ARTICULATION, 0, 1, 28, 0, 0 },
			{ PARAM::SLIDE, STR16("Slide"), STR16(""), VALUE::Int, SCALE::Linear, RANGE::PITCH, FLAG::HIDDEN, UNIT::ARTICULATION, 0, 1, 33, 0, 0 },
		}
	};

	// ユニット指定によるパラメータ定義取り出し
	template<size_t N>
	inline bool getParamsByUnit(std::array<const ParamDef*, N>& outResult, size_t& outCount, UNIT targetUnit)
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

	inline bool getTriggerParams(std::array<const ParamDef*, PARAM_TRIGGER_COUNT>& outResult, size_t& outCount)
	{
		return getParamsByUnit(outResult, outCount, UNIT::TRIGGER);
	}

	// アーティキュレーション系
	constexpr size_t PARAM_ARTICULATION_COUNT = 7;
	static_assert(PARAM_ARTICULATION_COUNT <= paramTable.size(), "Articulation param count mismatch");

	inline bool getArticulationParams(std::array<const ParamDef*, PARAM_ARTICULATION_COUNT>& outResult, size_t& outCount)
	{
		return getParamsByUnit(outResult, outCount, UNIT::ARTICULATION);
	}

	// パラメータヘルパー初期化
	inline const void initParameters()
	{
		static RangeResolver rangeResolver;
		static OptionProvider optionProvider;

		auto& helper = ParameterFramework::ParamHelper::get();

		const char* STR_USERPROFILE = "USERPROFILE";
		const char* PRESET_ROOT = "Documents/VST3 Presets/MinMax/QBStrum/Standard 6Strings.json";
		std::filesystem::path path = std::filesystem::path(getenv(STR_USERPROFILE)).append(PRESET_ROOT).make_preferred();
		ChordMap::initFromPreset(path);

		helper.setKindResolver(&rangeResolver);
		helper.setOptionProvider(&optionProvider);
	}
}