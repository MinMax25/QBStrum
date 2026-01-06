//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <base/source/fstreamer.h>
#include <pluginterfaces/base/fplatform.h>
#include <pluginterfaces/base/fstrdefs.h>
#include <pluginterfaces/base/ftypes.h>
#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/base/ibstream.h>
#include <pluginterfaces/gui/iplugview.h>
#include <pluginterfaces/vst/ivstattributes.h>
#include <pluginterfaces/vst/ivstcomponent.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>
#include <pluginterfaces/vst/ivstmessage.h>
#include <pluginterfaces/vst/ivstmidicontrollers.h>
#include <pluginterfaces/vst/ivstunits.h>
#include <pluginterfaces/vst/vsttypes.h>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <string.h>

#include "myparameters.h"
#include "myvst3editor.h"
#include "parameterframework.h"
#include "plugcontroller.h"
#include "plugdefine.h"

namespace MinMax
{
	Steinberg::tresult PLUGIN_API MyVSTController::initialize(FUnknown* context)
	{
		Steinberg::tresult result = EditControllerEx1::initialize(context);

		if (result != Steinberg::kResultOk)
		{
			return result;
		}

		// ユニット登録
		addUnit(new Steinberg::Vst::Unit(STR16("Chord"), UNIT::CHORD));
		addUnit(new Steinberg::Vst::Unit(STR16("Strum"), UNIT::STRUM));
		addUnit(new Steinberg::Vst::Unit(STR16("Trigger"), UNIT::TRIGGER));
		addUnit(new Steinberg::Vst::Unit(STR16("Articulation"), UNIT::ARTICULATION));

		// パラメータ登録
		auto& helper = PF::ParamHelper::get();

		for (auto& def : paramTable)
		{
			auto param = helper.createParameter(def);
			if (param)
			{
				parameters.addParameter(param.release());
			}
		}

		return result;
	}

	Steinberg::tresult PLUGIN_API MyVSTController::terminate()
	{
		return EditControllerEx1::terminate();
	}

	Steinberg::tresult PLUGIN_API MyVSTController::setComponentState(Steinberg::IBStream* state)
	{
		if (!state) return Steinberg::kInvalidArgument;

		for (const auto& def : paramTable)
		{
			double plain = 0.0;
			if (state->read(&plain, sizeof(double), nullptr) != Steinberg::kResultOk) return Steinberg::kResultFalse;

			// 正規化値に変換
			Steinberg::Vst::ParamValue normalized = plainParamToNormalized(def.tag, plain);

			// EditControllerに値をセット
			setParamNormalized(def.tag, normalized);
		}

		return Steinberg::kResultOk;
	}

	Steinberg::tresult PLUGIN_API MyVSTController::setState(Steinberg::IBStream* state)
	{
		return Steinberg::kResultTrue;
	}

	Steinberg::tresult PLUGIN_API MyVSTController::getState(Steinberg::IBStream* state)
	{
		return Steinberg::kResultTrue;
	}

	Steinberg::IPlugView* PLUGIN_API MyVSTController::createView(Steinberg::FIDString name)
	{
		if (Steinberg::FIDStringsEqual(name, Steinberg::Vst::ViewType::kEditor))
		{
			view = new MyVST3Editor(this, "view", "plugeditor.uidesc");
			return view;
		}
		return nullptr;
	}

	Steinberg::tresult PLUGIN_API MyVSTController::setParamNormalized(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value)
	{
		Steinberg::tresult result = EditControllerEx1::setParamNormalized(tag, value);
		return result;
	}

	Steinberg::tresult PLUGIN_API MyVSTController::getMidiControllerAssignment(Steinberg::int32 busIndex, Steinberg::int16 channel, Steinberg::Vst::CtrlNumber midiControllerNumber, Steinberg::Vst::ParamID& value)
	{
		switch (midiControllerNumber)
		{
		case Steinberg::Vst::kCtrlNRPNSelectLSB:
			value = static_cast<Steinberg::Vst::CtrlNumber>(PARAM::CHORD_LSB);
			return Steinberg::kResultTrue;

		case Steinberg::Vst::kCtrlNRPNSelectMSB:
			value = static_cast<Steinberg::Vst::CtrlNumber>(PARAM::CHORD_MSB);
			return Steinberg::kResultTrue;
		}

		return Steinberg::kResultFalse;
	}

	Steinberg::tresult PLUGIN_API MyVSTController::getUnitByBus(Steinberg::Vst::MediaType valueType, Steinberg::Vst::BusDirection dir, Steinberg::int32 busIndex, Steinberg::int32 channel, Steinberg::Vst::UnitID& unitId)
	{
		if (valueType == Steinberg::Vst::kEvent && dir == Steinberg::Vst::kInput)
		{
			if (busIndex == 0 && channel == 0)
			{
				unitId = Steinberg::Vst::kRootUnitId;
				return Steinberg::kResultTrue;
			}
		}
		return Steinberg::kResultFalse;
	}

	Steinberg::tresult PLUGIN_API MyVSTController::notify(Steinberg::Vst::IMessage* message)
	{
		if (!message) return Steinberg::kInvalidArgument;

		const char* value = message->getMessageID();

		if (strcmp(value, MSG_CHORD_CHANGED) != 0) return Steinberg::kResultFalse;
		Steinberg::Vst::IAttributeList* attr = message->getAttributes();
		if (!attr) return Steinberg::kResultFalse;

		const void* data = nullptr;
		Steinberg::uint32 size = 0;

		if (attr->getBinary(MSG_CHORD_VALUE, data, size) != Steinberg::kResultTrue) return Steinberg::kResultFalse;

		const int* chord = reinterpret_cast<const int*>(data);

		beginEdit(static_cast<int>(PARAM::CHORD_NUM));
		Steinberg::Vst::ParamValue norm = plainParamToNormalized(PARAM::CHORD_NUM, *chord);
		setParamNormalized(static_cast<int>(PARAM::CHORD_NUM), norm);
		performEdit(static_cast<int>(PARAM::CHORD_NUM), norm);
		endEdit(static_cast<int>(PARAM::CHORD_NUM));

		return Steinberg::kResultOk;
	}
}
