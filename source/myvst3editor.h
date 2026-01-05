//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <public.sdk/source/vst/vsteditcontroller.h>
#include <vstgui/lib/vstguibase.h>
#include <vstgui/plugin-bindings/vst3editor.h>

namespace MinMax
{
	class MyVST3Editor
		: public VSTGUI::VST3Editor
	{
	public:
		MyVST3Editor(Steinberg::Vst::EditController* controller, VSTGUI::UTF8StringPtr _viewName, VSTGUI::UTF8StringPtr _xmlFile)
			: VSTGUI::VST3Editor(controller, _viewName, _xmlFile)
		{
		}
	};
}