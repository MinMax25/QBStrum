#pragma once

#include <base/source/fstring.h>
#include <pluginterfaces/vst/ivstevents.h>
#include <array>

namespace MinMax
{
	using namespace Steinberg;
	using namespace Steinberg::Vst;

	using String = Steinberg::String;
	using Event = Steinberg::Vst::Event;

	inline constexpr int NOTE_COUNT = 128;

	inline constexpr int STRING_COUNT = 6;

	inline constexpr int STRUM_SPEED_MAX = 1000;
	inline constexpr int STRUM_SPEED_MIN = 1;

	inline constexpr int STRUM_DECAY_MAX = 100;
	inline constexpr int STRUM_DECAY_MIN = 85;

	inline constexpr int NOTE_LENGTH_MAX = 8;
	inline constexpr int NOTE_LENGTH_MIN = 1;

	inline constexpr int FRET_DISTANCE_MAX = 5;
	inline constexpr int FRET_DISTANCE_MIN = 1;

	inline constexpr int BRUSH_TIME_MAX = 300;
	inline constexpr int BRUSH_TIME_MIN = 30;

	inline constexpr int MAX_SCHEDULED_NOTES = 128;

	inline constexpr int SPECIAL_NOTES = -1;

	// メッセージ識別子
	inline constexpr auto MSG_SOUND_CHECK = "SoundCheck";
	inline constexpr auto MSG_CHORD_CHANGED = "ChordChanged";
	inline constexpr auto MSG_CHORD_VALUE = "ChordValue";

	// ノートメッセージ値
	struct CNoteMsg
	{
		ParamID value;
		bool OnOff;
		int velocity;
	};

	// パラメータ定義アイテム
	struct ParamItem
	{
		ParamID value;
		const char* name;
	};

	// ユニット定義
	inline constexpr int PARAM_UNIT_COUNT = 4;

	enum class PARAM_UNIT : ParamID
	{
		CHORD = 1,
		STRUM,
		TRIGGER,
		ARTICULATION,
	};

	inline constexpr std::array<ParamItem, PARAM_UNIT_COUNT> UnitDef =
	{
		{
			{ static_cast<ParamID>(PARAM_UNIT::CHORD), "Chord"   },
			{ static_cast<ParamID>(PARAM_UNIT::STRUM), "Strum"   },
			{ static_cast<ParamID>(PARAM_UNIT::ARTICULATION), "Articulation" },
			{ static_cast<ParamID>(PARAM_UNIT::TRIGGER), "Trigger" },
		}
	};

	// 一般パラメータ定義
	enum class PARAM_TAG : ParamID
	{
		BYPASS = 1,
		CHANNEL_SEPALATE = 1001,
		TRANSPOSE,
		SELECTED_ARTICULATION,
	};

	// コード指定パラメータ定義
	enum class PARAM_CHORD : ParamID
	{
		ROOT = 1101,
		TYPE,
		FRET_POSITION,
	};

	// ストラムパラメータ定義
	enum class PARAM_STRUM : ParamID
	{
		MUTE_CHANNEL = 1201,
		MUTE_NOTE_1,
		MUTE_NOTE_2,
		SPEED,
		DECAY,
		STRUM_LENGTH,
		BRUSH_TIME,
		ARP_LENGTH,
		FNOISE_CHANNEL,
		FNOISE_NOTE_NEAR,
		FNOISE_NOTE_FAR,
		FNOISE_DIST_NEAR,
		FNOISE_DIST_FAR,
		STRINGS_UP_HIGH,
		STRINGS_DOWN_HIGH,
		STRINGS_DOWN_LOW,
	};

	// ストラムトリガー定義
	inline constexpr int PARAM_TRIGGER_COUNT = 16;

	enum class PARAM_TRIGGER : ParamID
	{
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
	};

	inline constexpr std::array<ParamItem, PARAM_TRIGGER_COUNT> TriggerDef =
	{
		{
			{ static_cast<ParamID>(PARAM_TRIGGER::ALL_NOTES_OFF), "All Notes Off" },
			{ static_cast<ParamID>(PARAM_TRIGGER::BRUSH_UP), "Brush Up" },
			{ static_cast<ParamID>(PARAM_TRIGGER::BRUSH_DOWN), "Brush Down" },
			{ static_cast<ParamID>(PARAM_TRIGGER::UP_HIGH), "Up High" },
			{ static_cast<ParamID>(PARAM_TRIGGER::UP), "Up" },
			{ static_cast<ParamID>(PARAM_TRIGGER::DOWN_HIGH), "Down High" },
			{ static_cast<ParamID>(PARAM_TRIGGER::DOWN), "Down" },
			{ static_cast<ParamID>(PARAM_TRIGGER::DOWN_LOW), "Down Low" },
			{ static_cast<ParamID>(PARAM_TRIGGER::MUTE_1), "Mute 1" },
			{ static_cast<ParamID>(PARAM_TRIGGER::MUTE_2), "Mute 2" },
			{ static_cast<ParamID>(PARAM_TRIGGER::ARPEGGIO_1), "String 1" },
			{ static_cast<ParamID>(PARAM_TRIGGER::ARPEGGIO_2), "String 2" },
			{ static_cast<ParamID>(PARAM_TRIGGER::ARPEGGIO_3), "String 3" },
			{ static_cast<ParamID>(PARAM_TRIGGER::ARPEGGIO_4), "String 4" },
			{ static_cast<ParamID>(PARAM_TRIGGER::ARPEGGIO_5), "String 5" },
			{ static_cast<ParamID>(PARAM_TRIGGER::ARPEGGIO_6), "String 6" }
		}
	};

	// アーティキュレーションパラメータ定義
	inline constexpr int PARAM_ARTICULATION_COUNT = 7;

	enum class PARAM_ARTICULATION : ParamID
	{
		OPEN1 = 1401,
		OPEN2,
		HAM_LEG,
		MUTE,
		DEAD,
		HARMONICS,
		SLIDE,
	};

	inline constexpr std::array<ParamItem, PARAM_ARTICULATION_COUNT> ArticulationDef =
	{
		{
			{ static_cast<ParamID>(PARAM_ARTICULATION::OPEN1), "Open 1" },
			{ static_cast<ParamID>(PARAM_ARTICULATION::OPEN2), "Open 2" },
			{ static_cast<ParamID>(PARAM_ARTICULATION::HAM_LEG), "Hammer/Legato" },
			{ static_cast<ParamID>(PARAM_ARTICULATION::MUTE), "Mute" },
			{ static_cast<ParamID>(PARAM_ARTICULATION::DEAD), "Dead" },
			{ static_cast<ParamID>(PARAM_ARTICULATION::HARMONICS), "Harmonics" },
			{ static_cast<ParamID>(PARAM_ARTICULATION::SLIDE), "Slide" },
		}
	};

	// 音程名変換辞書
	inline const std::array<char*, NOTE_COUNT> NoteNames =
	{
		"C-2","C#-2","D-2","D#-2","E-2","F-2","F#-2","G-2","G#-2","A-2","A#-2","B-2",
		"C-1","C#-1","D-1","D#-1","E-1","F-1","F#-1","G-1","G#-1","A-1","A#-1","B-1",
		"C0","C#0","D0","D#0","E0","F0","F#0","G0","G#0","A0","A#0","B0",
		"C1","C#1","D1","D#1","E1","F1","F#1","G1","G#1","A1","A#1","B1",
		"C2","C#2","D2","D#2","E2","F2","F#2","G2","G#2","A2","A#2","B2",
		"C3","C#3","D3","D#3","E3","F3","F#3","G3","G#3","A3","A#3","B3",
		"C4","C#4","D4","D#4","E4","F4","F#4","G4","G#4","A4","A#4","B4",
		"C5","C#5","D5","D#5","E5","F5","F#5","G5","G#5","A5","A#5","B5",
		"C6","C#6","D6","D#6","E6","F6","F#6","G6","G#6","A6","A#6","B6",
		"C7","C#7","D7","D#7","E7","F7","F#7","G7","G#7","A7","A#7","B7",
		"C8","C#8","D8","D#8","E8","F8","F#8","G8",
	};
}
