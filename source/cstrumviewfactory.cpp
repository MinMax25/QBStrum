//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <array>
#include <base/source/fstring.h>
#include <cstdint>
#include <functional>
#include <pluginterfaces/base/smartpointer.h>
#include <pluginterfaces/vst/vsttypes.h>
#include <vstgui/lib/cbuttonstate.h>
#include <vstgui/lib/ccolor.h>
#include <vstgui/lib/cdrawdefs.h>
#include <vstgui/lib/cfont.h>
#include <vstgui/lib/controls/cbuttons.h>
#include <vstgui/lib/controls/ccontrol.h>
#include <vstgui/lib/controls/ctextlabel.h>
#include <vstgui/lib/controls/icontrollistener.h>
#include <vstgui/lib/cpoint.h>
#include <vstgui/lib/crect.h>
#include <vstgui/lib/crowcolumnview.h>
#include <vstgui/lib/cview.h>
#include <vstgui/lib/cviewcontainer.h>
#include <vstgui/lib/vstguibase.h>
#include <vstgui/lib/vstguifwd.h>
#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>
#include <vstgui/uidescription/iuidescription.h>
#include <vstgui/uidescription/iviewcreator.h>
#include <vstgui/uidescription/uiattributes.h>
#include <vstgui/uidescription/uiviewfactory.h>

#include "cnoteedit.h"
#include "cnotelabel.h"
#include "cnotemsg.h"
#include "myparameters.h"
#include "parameterframework.h"
#include "plugdefine.h"

namespace MinMax
{
    // ノート確認ボタン
    class CNoteButton
        : public  VSTGUI::CTextButton
    {
    public:

        CNoteButton(
            std::function<void(CControl*, VSTGUI::CPoint, bool)> _func,
            const  VSTGUI::CRect& size, VSTGUI::IControlListener* listener = nullptr, int32_t tag = -1, VSTGUI::UTF8StringPtr title = nullptr, Style = kKickStyle)
            : CTextButton(size, listener, tag, title, style)
        {
            onNoteButtonClicked = _func;
        }

        ~CNoteButton()
        {
            onNoteButtonClicked = nullptr;
        }

        void valueChanged() override
        {
            // Kill Event
        }

        VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override
        {
            if (onNoteButtonClicked) onNoteButtonClicked(this, where, true);
            return CTextButton::onMouseDown(where, buttons);
        }

        VSTGUI::CMouseEventResult onMouseUp(VSTGUI::CPoint& where, const  VSTGUI::CButtonState& buttons) override
        {
            if (onNoteButtonClicked) onNoteButtonClicked(this, where, false);
            return CTextButton::onMouseUp(where, buttons);
        }

        CLASS_METHODS(CNoteButton, CTextButton)

    protected:

        std::function<void(CControl*, VSTGUI::CPoint, bool)> onNoteButtonClicked;
    };

    class CStrumView
        : public  VSTGUI::CViewContainer
    {
    public:

        CStrumView(const  VSTGUI::UIAttributes& attributes, const  VSTGUI::IUIDescription* description, const  VSTGUI::CRect& size)
            : CViewContainer(size)
        {
            editor = static_cast<VSTGUI::VST3Editor*>(description->getController());
            editor->addRef();

            setBackgroundColor(VSTGUI::kGreyCColor);

			// ストラムパラメータグリッド
            VSTGUI::CRowColumnView* grdStrum = new  VSTGUI::CRowColumnView(VSTGUI::CRect(VSTGUI::CPoint(10, 5), VSTGUI::CPoint(410, 15 * PARAM_TRIGGER_COUNT)));
            grdStrum->setAutosizeFlags(VSTGUI::kAutosizeAll);
            grdStrum->setBackgroundColor(VSTGUI::kTransparentCColor);
            grdStrum->setStyle(VSTGUI::CRowColumnView::kRowStyle);
            addView(grdStrum);

            std::array<const PF::ParamDef*, MinMax::PARAM_TRIGGER_COUNT> triggerParams;
            size_t triggerCount = 0;
            getTriggerParams(triggerParams, triggerCount);

            for (size_t i = 0; i < triggerCount; ++i)
            {
                const PF::ParamDef* def = triggerParams[i];
                
                // ストラムパラメータ行
                VSTGUI::CRowColumnView* rcvONote = new  VSTGUI::CRowColumnView(VSTGUI::CRect(VSTGUI::CPoint(0, 0), VSTGUI::CPoint(400, 15)));
                rcvONote->setAutosizeFlags(VSTGUI::kAutosizeNone);
                rcvONote->setBackgroundColor(VSTGUI::kTransparentCColor);
                rcvONote->setStyle(VSTGUI::CRowColumnView::kColumnStyle);

				// ストラムパターン名ラベル
                VSTGUI::CTextLabel* labStrumTitle = new  VSTGUI::CTextLabel(VSTGUI::CRect(VSTGUI::CPoint(0, 0), VSTGUI::CPoint(80, 15)));
                labStrumTitle->setBackColor(VSTGUI::kTransparentCColor);
                labStrumTitle->setFrameColor(VSTGUI::kTransparentCColor);
                labStrumTitle->setFontColor(VSTGUI::kWhiteCColor);
                labStrumTitle->setHoriAlign(VSTGUI::CHoriTxtAlign::kLeftText);
                labStrumTitle->setFont(VSTGUI::kNormalFontSmall);
                labStrumTitle->setText(Steinberg::String(def->name).text8());
                rcvONote->addView(labStrumTitle);

				// ピッチ・ノート入力
                CNoteEdit* txtONote = new CNoteEdit(VSTGUI::CRect(VSTGUI::CPoint(0, 0), VSTGUI::CPoint(25, 15)), description->getController(), -1);
                txtONote->setTag(def->tag);
                txtONote->setBackColor(VSTGUI::kWhiteCColor);
                txtONote->setFontColor(VSTGUI::kBlackCColor);
                txtONote->setFont(VSTGUI::kNormalFontSmall);
                rcvONote->addView(txtONote);

				// ノート名表示ラベル
                CNoteLabel* labONote = new CNoteLabel(VSTGUI::CRect(VSTGUI::CPoint(0, 0), VSTGUI::CPoint(30, 15)));
                labONote->setListener(description->getController());
                labONote->setFont(VSTGUI::kNormalFontSmall);
                labONote->setTag(def->tag);
                rcvONote->addView(labONote);

				// ノート送信ボタン
                CNoteButton* btnONote = new CNoteButton([this](VSTGUI::CControl* p, VSTGUI::CPoint where, bool onoff) { onNoteButtonClicked(p, where, onoff); }, VSTGUI::CRect(VSTGUI::CPoint(0, 0), VSTGUI::CPoint(64, 15)));
                btnONote->setTag(def->tag);
                btnONote->setWantsFocus(false);
                btnONote->setStyle(VSTGUI::CTextButton::kKickStyle);
                btnONote->setRoundRadius(2);
                rcvONote->addView(btnONote);

                grdStrum->addView(rcvONote);
            }
        }

        ~CStrumView()
        {
            editor->release();
        }

        CLASS_METHODS(CStrumView, CViewContainer)

    protected:

        VSTGUI::VST3Editor* editor{};

        void onNoteButtonClicked(VSTGUI::CControl* pControl, VSTGUI::CPoint where, bool onoff)
        {
            if (auto message = Steinberg::owned(editor->getController()->allocateMessage()))
            {
                CNoteMsg note;

                Steinberg::Vst::ParamID tag = pControl->getTag();

                note.tag = tag;
                note.isOn = onoff;

				int vel = (223 - where.x) * 2;
                if (vel > 127) vel = 127;
                if (vel < 1) vel = 1;
                note.velocity = vel;

                // プラグインプロセッサにノートメッセージを送信
                message->setMessageID(MSG_SOUND_CHECK);
                if (auto attr = message->getAttributes())
                {
                    attr->setBinary(MSG_SOUND_CHECK, &note, sizeof(CNoteMsg));
                }
                if (editor->getController() == nullptr) return;
                editor->getController()->getPeer()->notify(message);
            }
        }
    };

    class CStrumViewFactory
        : public  VSTGUI::ViewCreatorAdapter
    {
    public:

        CStrumViewFactory() { VSTGUI::UIViewFactory::registerViewCreator(*this); }

        VSTGUI::IdStringPtr getViewName() const override { return "UI:Strum View"; }

        VSTGUI::IdStringPtr getBaseViewName() const override { return  VSTGUI::UIViewCreator::kCViewContainer; }

        VSTGUI::CView* create(const  VSTGUI::UIAttributes& attributes, const  VSTGUI::IUIDescription* description) const override
        {
            return new CStrumView(attributes, description, VSTGUI::CRect(VSTGUI::CPoint(0, 0), VSTGUI::CPoint(305, 5 * 2 + 15 * PARAM_TRIGGER_COUNT)));
        }
    };

    CStrumViewFactory __gCStrumViewFactory;
}