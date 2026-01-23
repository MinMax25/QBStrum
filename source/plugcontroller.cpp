//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------

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

#include "chordmap.h"
#include "myparameters.h"
#include "myvst3editor.h"
#include "parameterhelper.h"
#include "plugcontroller.h"
#include "plugdefine.h"
#include "stateio.h"

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
		addUnit(new Steinberg::Vst::Unit(STR16("General"), U_GENERAL));
		addUnit(new Steinberg::Vst::Unit(STR16("Chord"), U_CHORD));
		addUnit(new Steinberg::Vst::Unit(STR16("Strum"), U_STRUM));
		addUnit(new Steinberg::Vst::Unit(STR16("Brush"), U_BRUSH));
		addUnit(new Steinberg::Vst::Unit(STR16("Arpeggio"), U_ARP));
		addUnit(new Steinberg::Vst::Unit(STR16("Strings"), U_STRINGS));
		addUnit(new Steinberg::Vst::Unit(STR16("Mute"), U_MUTE));
		addUnit(new Steinberg::Vst::Unit(STR16("Fret Noize"), U_NOIZE));
		addUnit(new Steinberg::Vst::Unit(STR16("String Offset"), U_OFFSET));
		addUnit(new Steinberg::Vst::Unit(STR16("Trigger"), U_TRIGGER));
		addUnit(new Steinberg::Vst::Unit(STR16("Articulation"), U_ARTIC));

		// パラメータ登録
		for (auto& def : paramTable)
		{
			auto param = PF::ParamHelper::instance().createParameter(def);
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
		StateIO io(state);

		for (const auto& def : paramTable)
		{
			double plain = 0.0;

			if (!io.readDouble(plain)) return Steinberg::kResultFalse;
			Steinberg::Vst::ParamValue normalized = plainParamToNormalized(def.tag, plain);
			setParamNormalized(def.tag, normalized);
			if (def.tag == CHORD_NUM)
			{
				auto c = ChordMap::instance().getChordVoicing((int)plain);
				ChordInfo.size = c.size;
				ChordInfo.flatIndex = c.flatIndex;
				for (int i = 0; i < MAX_STRINGS; i++)
				{
					ChordInfo.data[i] = c.data[i];
					ChordInfo.setOffset(i, c.getOffset(i));
				}
			}
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
			value = static_cast<Steinberg::Vst::CtrlNumber>(CHORD_LSB);
			return Steinberg::kResultTrue;

		case Steinberg::Vst::kCtrlNRPNSelectMSB:
			value = static_cast<Steinberg::Vst::CtrlNumber>(CHORD_MSB);
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
		const void* msgData;
		Steinberg::uint32 msgSize;

		const auto attr = message->getAttributes();
		if (attr == nullptr) return Steinberg::kResultFalse;

		if (!(attr->getBinary(MSG_CHORD_CHANGED, msgData, msgSize) == Steinberg::kResultTrue && msgSize == sizeof(StringSet)))
		{
			return Steinberg::kResultFalse;
		}

		const auto set = reinterpret_cast<const StringSet*>(msgData);

		ChordInfo.state = set->state;
		ChordInfo.flatIndex = set->flatIndex;
		ChordInfo.size = set->size;

		for (int i = 0; i < (int)set->size; i++)
		{
			ChordInfo.data[i] = set->data[i];
			ChordInfo.setOffset(i, set->getOffset(i));
		}

		{
			beginEdit(static_cast<int>(CHORD_NUM));
			Steinberg::Vst::ParamValue norm = plainParamToNormalized(CHORD_NUM, set->flatIndex);
			setParamNormalized(static_cast<int>(CHORD_NUM), norm);
			performEdit(static_cast<int>(CHORD_NUM), norm);
			endEdit(static_cast<int>(CHORD_NUM));
		}

		{
			beginEdit(static_cast<int>(CHORD_STATE_REVISION));
			Steinberg::Vst::ParamValue norm = plainParamToNormalized(CHORD_STATE_REVISION, set->state);
			setParamNormalized(static_cast<int>(CHORD_STATE_REVISION), norm);
			performEdit(static_cast<int>(CHORD_STATE_REVISION), norm);
			endEdit(static_cast<int>(CHORD_STATE_REVISION));
		}

		return Steinberg::kResultOk;
	}
}
