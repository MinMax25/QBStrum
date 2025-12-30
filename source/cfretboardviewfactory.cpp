//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------

#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>
#include <vstgui/lib/controls/ioptionmenulistener.h>
#include <vstgui/lib/controls/ctextlabel.h>
#include <vstgui/vstgui_uidescription.h>
#include <string>

#include "chordmap.h"
#include "cfretboard.h"
#include "cchordlabel.h"
#include <vstgui/lib/vstguibase.h>

namespace MinMax
{
    using namespace Steinberg;
    using namespace Steinberg::Vst;

    class CFretBoardView
        : public CViewContainer
    {
    public:
        CFretBoardView(const UIAttributes& attributes, const IUIDescription* description, const CRect& size)
            : CViewContainer(size)
        {
            editor = dynamic_cast<VST3Editor*>(description->getController());
            if (editor) editor->addRef();

            setBackgroundColor(kGreyCColor);

            CFretBoard* fretBoard = new CFretBoard(CRect(0, 20, 1120, size.getHeight() + 20));
            fretBoard->numStrings = STRING_COUNT;
            addView(fretBoard);
        }

        ~CFretBoardView()
        {
            if (editor) editor->release();
        }

        CLASS_METHODS(CFretBoardView, CViewContainer)

    private:

        VST3Editor* editor = nullptr;;
    };

    class CFretBoardViewFactory
        : public ViewCreatorAdapter
    {
    public:

        CFretBoardViewFactory() { UIViewFactory::registerViewCreator(*this); }

        IdStringPtr getViewName() const override { return "UI:FretBoard View"; }

        IdStringPtr getBaseViewName() const override { return UIViewCreator::kCViewContainer; }

        CView* create(const UIAttributes& attributes, const IUIDescription* description) const override
        {
            return new CFretBoardView(attributes, description, CRect(CPoint(0, 0), CPoint(660, 160)));
        }
    };

    CFretBoardViewFactory __gCFretBoardViewFactory;
}