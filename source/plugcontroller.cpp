//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------

#include <public.sdk/source/vst/utility/stringconvert.h>
#include <pluginterfaces/vst/ivstmidicontrollers.h>
#include <pluginterfaces/vst/ivstcomponent.h>
#include <base/source/fstreamer.h>
#include <pluginterfaces/base/ibstream.h>
#include <vstgui/plugin-bindings/vst3editor.h>

#include "plugdefine.h"
#include "plugcontroller.h"
#include "chordmap.h"
#include "parameterframework.h"
#include "myparameters.h"

#if DEBUG
#include "debug_log.h"
#endif

namespace MinMax
{
	using namespace Steinberg;

	tresult PLUGIN_API MyVSTController::initialize(FUnknown* context)
	{
		tresult result = EditControllerEx1::initialize(context);

		if (result != kResultOk)
		{
			return result;
		}

		// ユニット登録
		addUnit(new Unit(STR16("Chord"), UNIT::CHORD));
		addUnit(new Unit(STR16("Strum"), UNIT::STRUM));
		addUnit(new Unit(STR16("Trigger"), UNIT::TRIGGER));
		addUnit(new Unit(STR16("Articulation"), UNIT::ARTICULATION));

		// パラメータ登録
		auto& helper = ParameterFramework::ParamHelper::get();

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

	tresult PLUGIN_API MyVSTController::terminate()
	{
		return EditControllerEx1::terminate();
	}

	tresult PLUGIN_API MyVSTController::setComponentState(IBStream* state)
	{
		if (!state) return kInvalidArgument;

		for (const auto& def : paramTable)
		{
			double plain = 0.0;
			if (state->read(&plain, sizeof(double), nullptr) != kResultOk) return kResultFalse;

			// 正規化値に変換
			ParamValue normalized = plainParamToNormalized(def.tag, plain);

			// EditControllerに値をセット
			setParamNormalized(def.tag, normalized);
		}

		return kResultOk;
	}

	tresult PLUGIN_API MyVSTController::setState(IBStream* state)
	{
		if (!state) return kResultFalse;
		IBStreamer streamer(state, kLittleEndian);

		bool Bypass;
		if (streamer.readBool(Bypass) == false) return kResultFalse;
		beginEdit(static_cast<int>(PARAM::BYPASS));
		performEdit(static_cast<int>(PARAM::BYPASS), Bypass ? 1 : 0);
		endEdit(static_cast<int>(PARAM::BYPASS));

		return kResultTrue;
	}

	tresult PLUGIN_API MyVSTController::getState(IBStream* state)
	{
		if (!state) return kResultFalse;
		IBStreamer streamer(state, kLittleEndian);

		bool bypass = getParamNormalized(static_cast<int>(PARAM::BYPASS)) > 0.5;
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
			value = static_cast<CtrlNumber>(PARAM::CHORD_ROOT);
			return kResultTrue;

		case kCtrlDataEntryMSB:
			value = static_cast<CtrlNumber>(PARAM::CHORD_TYPE);
			return kResultTrue;

		case kCtrlDataEntryLSB:
		case 14:
			value = static_cast<CtrlNumber>(PARAM::FRET_POSITION);
			return kResultTrue;

		case 20:
			value = static_cast<CtrlNumber>(PARAM::SPEED);
			return kResultTrue;

		case 21:
			value = static_cast<CtrlNumber>(PARAM::DECAY);
			return kResultTrue;

		case 22:
			value = static_cast<CtrlNumber>(PARAM::STRUM_LENGTH);
			return kResultTrue;

		case kCtrlReleaseTime:
			value = static_cast<CtrlNumber>(PARAM::BRUSH_TIME);
			return kResultTrue;

		case 23:
			value = static_cast<CtrlNumber>(PARAM::ARP_LENGTH);
			return kResultTrue;

		case 24:
			value = static_cast<CtrlNumber>(PARAM::STRINGS_UP_HIGH);
			return kResultTrue;

		case 25:
			value = static_cast<CtrlNumber>(PARAM::STRINGS_DOWN_HIGH);
			return kResultTrue;

		case 26:
			value = static_cast<CtrlNumber>(PARAM::STRINGS_DOWN_LOW);
			return kResultTrue;

		case kCtrlSoundVariation:
			value = static_cast<CtrlNumber>(PARAM::SELECTED_ARTICULATION);
			return kResultTrue;

		case 27:
			value = static_cast<CtrlNumber>(PARAM::TRANSPOSE);
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
		beginEdit(static_cast<int>(PARAM::CHORD_ROOT));
		setParamNormalized(static_cast<int>(PARAM::CHORD_ROOT), rootNorm);
		performEdit(static_cast<int>(PARAM::CHORD_ROOT), rootNorm);
		endEdit(static_cast<int>(PARAM::CHORD_ROOT));

		ParamValue typeNorm = (ParamValue)chord->type / (ChordMap::Instance().getChordTypeCount() - 1);
		beginEdit(static_cast<int>(PARAM::CHORD_TYPE));
		setParamNormalized(static_cast<int>(PARAM::CHORD_TYPE), typeNorm);
		performEdit(static_cast<int>(PARAM::CHORD_TYPE), typeNorm);
		endEdit(static_cast<int>(PARAM::CHORD_TYPE));

		ParamValue posNorm = (ParamValue)chord->position / (ChordMap::Instance().getFretPosCount() - 1);
		beginEdit(static_cast<int>(PARAM::FRET_POSITION));
		setParamNormalized(static_cast<int>(PARAM::FRET_POSITION), posNorm);
		performEdit(static_cast<int>(PARAM::FRET_POSITION), posNorm);
		endEdit(static_cast<int>(PARAM::FRET_POSITION));

		return kResultOk;
	}
}
