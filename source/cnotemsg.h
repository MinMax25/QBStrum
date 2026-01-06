#pragma once

#include <array>
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
}
