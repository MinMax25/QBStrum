#pragma once

#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/lib/vstguibase.h>
#include <pluginterfaces/gui/iplugview.h>
#include <pluginterfaces/base/ustring.h>

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