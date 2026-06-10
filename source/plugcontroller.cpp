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
	using namespace Steinberg;
	using namespace Steinberg::Vst;

	tresult PLUGIN_API MyVSTController::initialize(FUnknown* context)
	{
		tresult result = EditControllerEx1::initialize(context);

		if (result != kResultOk)
		{
			return result;
		}

		// ユニット登録
		addUnit(new Unit(STR16("General"), U_GENERAL));
		addUnit(new Unit(STR16("Chord"), U_CHORD));
		addUnit(new Unit(STR16("Strum"), U_STRUM));
		addUnit(new Unit(STR16("Brush"), U_BRUSH));
		addUnit(new Unit(STR16("Arpeggio"), U_ARP));
		addUnit(new Unit(STR16("Strings"), U_STRINGS));
		addUnit(new Unit(STR16("Mute"), U_MUTE));
		addUnit(new Unit(STR16("Fret Noize"), U_NOIZE));
		addUnit(new Unit(STR16("String Offset"), U_OFFSET));
		addUnit(new Unit(STR16("Trigger"), U_TRIGGER));
		addUnit(new Unit(STR16("Articulation"), U_ARTIC));

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

	tresult PLUGIN_API MyVSTController::terminate()
	{
		return EditControllerEx1::terminate();
	}

	tresult PLUGIN_API MyVSTController::setComponentState(IBStream* state)
	{
		if (!state) return kInvalidArgument;
		StateIO io(state);

		for (const auto& def : paramTable)
		{
			double plain = 0.0;

			if (!io.readDouble(plain)) return kResultFalse;
			ParamValue normalized = plainParamToNormalized(def.tag, plain);
			setParamNormalized(def.tag, normalized);
			if (def.tag == CHORD_NUM)
			{
				auto c = ChordMap::instance().getChordVoicing((int)plain);
				ChordInfo.flatIndex = c.flatIndex;
				for (int i = 0; i < STRING_COUNT; i++)
				{
					ChordInfo.data[i] = c.data[i];
					ChordInfo.setOffset(i, c.getOffset(i));
				}
			}
		}

		return kResultOk;
	}

	tresult PLUGIN_API MyVSTController::setState(IBStream* state)
	{
		return kResultTrue;
	}

	tresult PLUGIN_API MyVSTController::getState(IBStream* state)
	{
		return kResultTrue;
	}

	IPlugView* PLUGIN_API MyVSTController::createView(FIDString name)
	{
		if (FIDStringsEqual(name, ViewType::kEditor))
		{
			view = new MyVST3Editor(this, "view", "plugeditor.uidesc");
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
			value = static_cast<CtrlNumber>(CHORD_LSB);
			return kResultTrue;

		case kCtrlNRPNSelectMSB:
			value = static_cast<CtrlNumber>(CHORD_MSB);
			return kResultTrue;
		}

		return kResultFalse;
	}

	tresult PLUGIN_API MyVSTController::getUnitByBus(MediaType valueType, BusDirection dir, int32 busIndex, int32 channel, UnitID& unitId)
	{
		if (valueType == kEvent && dir == kInput)
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
		const void* msgData;
		uint32 msgSize;

		const auto attr = message->getAttributes();
		if (attr == nullptr) return kResultFalse;

		if (!(attr->getBinary(MSG_CHORD_CHANGED, msgData, msgSize) == kResultTrue && msgSize == sizeof(StringSet)))
		{
			return kResultFalse;
		}

		const auto set = reinterpret_cast<const StringSet*>(msgData);

		ChordInfo.state = set->state;
		ChordInfo.flatIndex = set->flatIndex;

		for (int i = 0; i < STRING_COUNT; i++)
		{
			ChordInfo.data[i] = set->data[i];
			ChordInfo.setOffset(i, set->getOffset(i));
		}

		{
			beginEdit(static_cast<int>(CHORD_NUM));
			ParamValue norm = plainParamToNormalized(CHORD_NUM, set->flatIndex);
			setParamNormalized(static_cast<int>(CHORD_NUM), norm);
			performEdit(static_cast<int>(CHORD_NUM), norm);
			endEdit(static_cast<int>(CHORD_NUM));
		}

		{
			beginEdit(static_cast<int>(CHORD_STATE_REVISION));
			ParamValue norm = plainParamToNormalized(CHORD_STATE_REVISION, set->state);
			setParamNormalized(static_cast<int>(CHORD_STATE_REVISION), norm);
			performEdit(static_cast<int>(CHORD_STATE_REVISION), norm);
			endEdit(static_cast<int>(CHORD_STATE_REVISION));
		}

		return kResultOk;
	}
}
