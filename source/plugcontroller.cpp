//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------

#include <public.sdk/source/vst/utility/stringconvert.h>
#include <pluginterfaces/vst/ivstmidicontrollers.h>
#include <pluginterfaces/vst/ivstcomponent.h>
#include <base/source/fstreamer.h>

#include "plugdefine.h"
#include "plugcontroller.h"
#include "ChordMap.h"

namespace MinMax
{
	using namespace Steinberg;

	using ParamDefRepository = ParameterFramework::ParamDefRepository;

	tresult PLUGIN_API MyVSTController::initialize(FUnknown* context)
	{
		tresult result = EditControllerEx1::initialize(context);

		if (result != kResultOk)
		{
			return result;
		}

		// ユニット登録
		for (auto& item : UnitDef)
		{
			addUnit(new Unit(String(item.name), item.value));
		}

		// パラメータ登録
		paramDefs = &ParamDefRepository::Instance().getDefs();

		for (auto& param : paramDefs->getAllParams())
		{
			auto p = param->createVstParameter();
			if (p != nullptr)
			{
				parameters.addParameter(p);
			}
		}

		return result;
	}

	tresult PLUGIN_API MyVSTController::terminate()
	{
		return EditControllerEx1::terminate();
	}

	tresult PLUGIN_API MyVSTController::setComponentState(IBStream* state)
	{
		if (!state) return kResultFalse;
		IBStreamer streamer(state, kLittleEndian);

		for (auto& paramDef : paramDefs->getAllParams())
		{
			if (!paramDef->readCtrl(streamer, *this))
			{
				return kResultFalse;
			}
		}

		return kResultTrue;
	}

	tresult PLUGIN_API MyVSTController::setState(IBStream* state)
	{
		if (!state) return kResultFalse;
		IBStreamer streamer(state, kLittleEndian);

		bool Bypass;
		if (streamer.readBool(Bypass) == false) return kResultFalse;
		beginEdit(static_cast<int>(PARAM_TAG::BYPASS));
		performEdit(static_cast<int>(PARAM_TAG::BYPASS), Bypass ? 1 : 0);
		endEdit(static_cast<int>(PARAM_TAG::BYPASS));

		return kResultTrue;
	}

	tresult PLUGIN_API MyVSTController::getState(IBStream* state)
	{
		if (!state) return kResultFalse;
		IBStreamer streamer(state, kLittleEndian);

		bool bypass = getParamNormalized(static_cast<int>(PARAM_TAG::BYPASS)) > 0.5;
		streamer.writeBool(bypass);

		return kResultTrue;
	}

	IPlugView* PLUGIN_API MyVSTController::createView(FIDString name)
	{
		if (FIDStringsEqual(name, Vst::ViewType::kEditor))
		{
			auto* view = new VSTGUI::VST3Editor(this, "view", "plugeditor.uidesc");
			return view;
		}
		return nullptr;
	}

	tresult PLUGIN_API MyVSTController::setParamNormalized(ParamID tag, ParamValue value)
	{
		tresult result = EditControllerEx1::setParamNormalized(tag, value);
		return result;
	}

	tresult PLUGIN_API MyVSTController::getMidiControllerAssignment(int32 busIndex, int16 channel, CtrlNumber midiControllerNumber, ParamID& value)
	{
		switch (midiControllerNumber)
		{
		case kCtrlNRPNSelectLSB:
			value = static_cast<CtrlNumber>(PARAM_CHORD::ROOT);
			return kResultTrue;

		case kCtrlDataEntryMSB:
			value = static_cast<CtrlNumber>(PARAM_CHORD::TYPE);
			return kResultTrue;

		case kCtrlDataEntryLSB:
			value = static_cast<CtrlNumber>(PARAM_CHORD::FRET_POSITION);
			return kResultTrue;

		case 20:
			value = static_cast<CtrlNumber>(PARAM_STRUM::SPEED);
			return kResultTrue;

		case 21:
			value = static_cast<CtrlNumber>(PARAM_STRUM::DECAY);
			return kResultTrue;

		case 22:
			value = static_cast<CtrlNumber>(PARAM_STRUM::STRUM_LENGTH);
			return kResultTrue;

		case kCtrlReleaseTime:
			value = static_cast<CtrlNumber>(PARAM_STRUM::BRUSH_TIME);
			return kResultTrue;

		case 23:
			value = static_cast<CtrlNumber>(PARAM_STRUM::ARP_LENGTH);
			return kResultTrue;

		case 24:
			value = static_cast<CtrlNumber>(PARAM_STRUM::STRINGS_UP_HIGH);
			return kResultTrue;

		case 25:
			value = static_cast<CtrlNumber>(PARAM_STRUM::STRINGS_DOWN_HIGH);
			return kResultTrue;

		case 26:
			value = static_cast<CtrlNumber>(PARAM_STRUM::STRINGS_DOWN_LOW);
			return kResultTrue;

		case kCtrlSoundVariation:
			value = static_cast<CtrlNumber>(PARAM_TAG::SELECTED_ARTICULATION);
			return kResultTrue;

		case 27:
			value = static_cast<CtrlNumber>(PARAM_TAG::TRANSPOSE);
			return kResultTrue;
		}

		return kResultFalse;
	}

	tresult PLUGIN_API MyVSTController::getUnitByBus(MediaType type, BusDirection dir, int32 busIndex, int32 channel, UnitID& unitId)
	{
		if (type == kEvent && dir == kInput)
		{
			if (busIndex == 0 && channel == 0)
			{
				unitId = kRootUnitId;
				return kResultTrue;
			}
		}
		return kResultFalse;
	}

	tresult PLUGIN_API MyVSTController::notify(IMessage* message)
	{
		if (!message) return kInvalidArgument;

		const char* value = message->getMessageID();

		if (strcmp(value, MSG_CHORD_CHANGED) != 0) return kResultFalse;
		IAttributeList* attr = message->getAttributes();
		if (!attr) return kResultFalse;

		const void* data = nullptr;
		uint32 size = 0;

		if (attr->getBinary(MSG_CHORD_VALUE, data, size) != kResultTrue) return kResultFalse;

		const CChord* chord = reinterpret_cast<const CChord*>(data);

		ParamValue rootNorm = (ParamValue)chord->root / (ChordMap::Instance().getChordRootCount() - 1);
		beginEdit(static_cast<int>(PARAM_CHORD::ROOT));
		setParamNormalized(static_cast<int>(PARAM_CHORD::ROOT), rootNorm);
		performEdit(static_cast<int>(PARAM_CHORD::ROOT), rootNorm);
		endEdit(static_cast<int>(PARAM_CHORD::ROOT));

		ParamValue typeNorm = (ParamValue)chord->type / (ChordMap::Instance().getChordTypeCount() - 1);
		beginEdit(static_cast<int>(PARAM_CHORD::TYPE));
		setParamNormalized(static_cast<int>(PARAM_CHORD::TYPE), typeNorm);
		performEdit(static_cast<int>(PARAM_CHORD::TYPE), typeNorm);
		endEdit(static_cast<int>(PARAM_CHORD::TYPE));

		ParamValue posNorm = (ParamValue)chord->position / (ChordMap::Instance().getFretPosCount() - 1);
		beginEdit(static_cast<int>(PARAM_CHORD::FRET_POSITION));
		setParamNormalized(static_cast<int>(PARAM_CHORD::FRET_POSITION), posNorm);
		performEdit(static_cast<int>(PARAM_CHORD::FRET_POSITION), posNorm);
		endEdit(static_cast<int>(PARAM_CHORD::FRET_POSITION));

		return kResultOk;
	}
}
