#pragma once

#include <filesystem>
#include <string>
#include <public.sdk/source/vst/vstguieditor.h>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/lib/vstguibase.h>
#include <vstgui/vstgui.h>

#include "myparameters.h"

namespace MinMax
{
	class CChordNumber
		: public VSTGUI::CParamDisplay
	{
		class CChordNumber(const VSTGUI::CRect size, IControlListener* listener, int32_t tag)
			: CParamDisplay(size)
		{

		}
	};
}
