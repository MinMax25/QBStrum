//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------

#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>
#include <vstgui/uidescription/iuidescription.h>
#include <vstgui/lib/vstguibase.h>
#include <vstgui/vstgui_uidescription.h>

#include "myparameters.h"
#include "cfretboard.h"

namespace MinMax
{
    class CFretBoardView
        : public VSTGUI::CViewContainer
    {
    public:
        CFretBoardView(const VSTGUI::UIAttributes& attributes, const VSTGUI::IUIDescription* description, const VSTGUI::CRect& size)
            : CViewContainer(size)
        {
            editor = dynamic_cast<VSTGUI::VST3Editor*>(description->getController());
            if (editor) editor->addRef();

            setBackgroundColor(VSTGUI::kGreyCColor);

            CFretBoard* fretBoard = new CFretBoard(VSTGUI::CRect(0, 20, 1120, size.getHeight() + 20), editor, PARAM::CHORD_NUM);
            addView(fretBoard);
        }

        ~CFretBoardView()
        {
            if (editor) editor->release();
        }

        CLASS_METHODS(CFretBoardView, CViewContainer)

    private:

        VSTGUI::VST3Editor* editor = nullptr;;
    };

    class CFretBoardViewFactory
        : public VSTGUI::ViewCreatorAdapter
    {
    public:

        CFretBoardViewFactory() { VSTGUI::UIViewFactory::registerViewCreator(*this); }

        VSTGUI::IdStringPtr getViewName() const override { return "UI:FretBoard View"; }

        VSTGUI::IdStringPtr getBaseViewName() const override { return VSTGUI::UIViewCreator::kCViewContainer; }

        VSTGUI::CView* create(const VSTGUI::UIAttributes& attributes, const VSTGUI::IUIDescription* description) const override
        {
            return new CFretBoardView(attributes, description, VSTGUI::CRect(VSTGUI::CPoint(0, 0), VSTGUI::CPoint(660, 160)));
        }
    };

    CFretBoardViewFactory __gCFretBoardViewFactory;
}