//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <pluginterfaces/vst/vsttypes.h>

namespace MinMax
{
	// ノートメッセージ値
	struct CNoteMsg
	{
		Steinberg::Vst::ParamID tag;
		bool isOn;
		int velocity;
	};

	// フレットボードクリックによるノートプレビューメッセージ値
	struct CFretNoteMsg
	{
		int stringIndex;
		int fret;
		bool isOn;
		int velocity;
	};
}
