//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------

#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/vstgui_uidescription.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>

#include "plugdefine.h"
#include "cnotelabel.h"
#include "cnoteedit.h"
#include "myparameters.h"

namespace MinMax
{
    // ノート確認ボタン
    class CNoteButton
        : public CTextButton
    {
    public:

        CNoteButton(
            std::function<void(CControl*, CPoint, bool)> _func,
            const CRect& size, IControlListener* listener = nullptr, int32_t tag = -1, UTF8StringPtr title = nullptr, Style = kKickStyle)
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

        CMouseEventResult onMouseDown(CPoint& where, const CButtonState& buttons) override
        {
            if (onNoteButtonClicked) onNoteButtonClicked(this, where, true);
            return CTextButton::onMouseDown(where, buttons);
        }

        CMouseEventResult onMouseUp(CPoint& where, const CButtonState& buttons) override
        {
            if (onNoteButtonClicked) onNoteButtonClicked(this, where, false);
            return CTextButton::onMouseUp(where, buttons);
        }

        CLASS_METHODS(CNoteButton, CTextButton)

    protected:

        std::function<void(CControl*, CPoint, bool)> onNoteButtonClicked;
    };

    class CStrumView
        : public CViewContainer
    {
    public:

        CStrumView(const UIAttributes& attributes, const IUIDescription* description, const CRect& size)
            : CViewContainer(size)
        {
            editor = static_cast<VST3Editor*>(description->getController());
            editor->addRef();

            setBackgroundColor(kGreyCColor);

			// ストラムパラメータグリッド
            CRowColumnView* grdStrum = new CRowColumnView(CRect(CPoint(10, 5), CPoint(410, 15 * PARAM_TRIGGER_COUNT)));
            grdStrum->setAutosizeFlags(kAutosizeAll);
            grdStrum->setBackgroundColor(kTransparentCColor);
            grdStrum->setStyle(CRowColumnView::kRowStyle);
            addView(grdStrum);

            std::array<const ParamDef*, MinMax::PARAM_TRIGGER_COUNT> triggerParams;
            size_t triggerCount = 0;
            getTriggerParams(triggerParams, triggerCount);

            for (size_t i = 0; i < triggerCount; ++i)
            {
                const ParamDef* def = triggerParams[i];
                
                // ストラムパラメータ行
                CRowColumnView* rcvONote = new CRowColumnView(CRect(CPoint(0, 0), CPoint(400, 15)));
                rcvONote->setAutosizeFlags(kAutosizeNone);
                rcvONote->setBackgroundColor(kTransparentCColor);
                rcvONote->setStyle(CRowColumnView::kColumnStyle);

				// ストラムパターン名ラベル
                CTextLabel* labStrumTitle = new CTextLabel(CRect(CPoint(0, 0), CPoint(80, 15)));
                labStrumTitle->setBackColor(kTransparentCColor);
                labStrumTitle->setFrameColor(kTransparentCColor);
                labStrumTitle->setFontColor(kWhiteCColor);
                labStrumTitle->setHoriAlign(CHoriTxtAlign::kLeftText);
                labStrumTitle->setFont(kNormalFontSmall);
                labStrumTitle->setText(String(def->name).text8());
                rcvONote->addView(labStrumTitle);

				// ピッチ・ノート入力
                CNoteEdit* txtONote = new CNoteEdit(CRect(CPoint(0, 0), CPoint(25, 15)), description->getController(), -1);
                txtONote->setTag(def->tag);
                txtONote->setBackColor(kWhiteCColor);
                txtONote->setFontColor(kBlackCColor);
                txtONote->setFont(kNormalFontSmall);
                rcvONote->addView(txtONote);

				// ノート名表示ラベル
                CNoteLabel* labONote = new CNoteLabel(CRect(CPoint(0, 0), CPoint(30, 15)));
                labONote->setListener(description->getController());
                labONote->setFont(kNormalFontSmall);
                labONote->setTag(def->tag);
                rcvONote->addView(labONote);

				// ノート送信ボタン
                CNoteButton* btnONote = new CNoteButton([this](CControl* p, CPoint where, bool onoff) { onNoteButtonClicked(p, where, onoff); }, CRect(CPoint(0, 0), CPoint(64, 15)));
                btnONote->setTag(def->tag);
                btnONote->setWantsFocus(false);
                btnONote->setStyle(CTextButton::kKickStyle);
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

        VST3Editor* editor{};

        void onNoteButtonClicked(CControl* pControl, CPoint where, bool onoff)
        {
            if (auto message = Steinberg::owned(editor->getController()->allocateMessage()))
            {
                CNoteMsg note;

                ParamID tag = pControl->getTag();

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
        : public ViewCreatorAdapter
    {
    public:

        CStrumViewFactory() { UIViewFactory::registerViewCreator(*this); }

        IdStringPtr getViewName() const override { return "UI:Strum View"; }

        IdStringPtr getBaseViewName() const override { return UIViewCreator::kCViewContainer; }

        CView* create(const UIAttributes& attributes, const IUIDescription* description) const override
        {
            return new CStrumView(attributes, description, CRect(CPoint(0, 0), CPoint(305, 5 * 2 + 15 * PARAM_TRIGGER_COUNT)));
        }
    };

    CStrumViewFactory __gCStrumViewFactory;
}