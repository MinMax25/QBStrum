#pragma once

#include <filesystem>
#include <public.sdk/source/vst/vstguieditor.h>
#include <vstgui/vstgui.h>


namespace MinMax
{
	class CChordNumber
		: public VSTGUI::CParamDisplay
	{
		class CChordNumber(const VSTGUI::CRect size, VSTGUI::IControlListener* listener, int32_t tag)
			: CParamDisplay(size)
		{

		}
	};
}
