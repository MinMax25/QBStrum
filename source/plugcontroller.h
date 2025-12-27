//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------

#pragma once

#include <vstgui/plugin-bindings/vst3editor.h>
#include "ParameterFramework.h"

namespace MinMax
{
	class MyVSTController 
		: public Steinberg::Vst::EditControllerEx1
		, public IMidiMapping
	{
		using ExParamContainer = ParameterFramework::ExParamContainer;

	public:
		MyVSTController() = default;

		~MyVSTController() SMTG_OVERRIDE = default;

		static Steinberg::FUnknown* createInstance(void* /*context*/)
		{
			return (Steinberg::Vst::IEditController*)new MyVSTController;
		}

		Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
		Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;
		Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) SMTG_OVERRIDE;
		Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) SMTG_OVERRIDE;
		Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;
		Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;
		Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value) SMTG_OVERRIDE;

		// -- 追加部分 --

		tresult PLUGIN_API getMidiControllerAssignment(int32 busIndex, int16 channel, CtrlNumber midiControllerNumber, ParamID& value) SMTG_OVERRIDE;

		tresult PLUGIN_API getUnitByBus(MediaType type, BusDirection dir, int32 busIndex, int32 channel, UnitID& unitId) SMTG_OVERRIDE;
		
		tresult PLUGIN_API notify(IMessage* message) SMTG_OVERRIDE;

		// --------------

		DEFINE_INTERFACES
			DEF_INTERFACE(IMidiMapping)
			END_DEFINE_INTERFACES(EditControllerEx1)
			DELEGATE_REFCOUNT(EditControllerEx1)

	protected:

		const ExParamContainer* paramDefs = nullptr;
	};
}