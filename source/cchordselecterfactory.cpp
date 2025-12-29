//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------

#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/vstgui_uidescription.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>

#include "plugdefine.h"
#include "myparameters.h"

namespace MinMax
{
    using namespace VSTGUI;

    class CChordSelecter
        : public CViewContainer
    {
    public:

        CChordSelecter(const UIAttributes& attributes, const IUIDescription* description, const CRect& size)
            : CViewContainer(size)
        {
            editor = static_cast<VST3Editor*>(description->getController());
            editor->addRef();
        }

        ~CChordSelecter()
        {
            editor->release();
        }

        CLASS_METHODS(CChordSelecter, CViewContainer)

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

    class CChordSelecterFactory
        : public ViewCreatorAdapter
    {
    public:

        CChordSelecterFactory() { UIViewFactory::registerViewCreator(*this); }

        IdStringPtr getViewName() const override { return "UI:Chord Selecter View"; }

        IdStringPtr getBaseViewName() const override { return UIViewCreator::kCViewContainer; }

        CView* create(const UIAttributes& attributes, const IUIDescription* description) const override
        {
            return new CChordSelecter(attributes, description, CRect(CPoint(0, 0), CPoint(200, 15)));
        }
    };

    CChordSelecterFactory __gCCChordSelecterFactory;
}