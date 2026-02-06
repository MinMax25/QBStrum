//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------

#include <vstgui/lib/cpoint.h>
#include <vstgui/lib/crect.h>
#include <vstgui/lib/cview.h>
#include <vstgui/lib/vstguibase.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>
#include <vstgui/uidescription/iuidescription.h>
#include <vstgui/uidescription/iviewcreator.h>
#include <vstgui/uidescription/uiattributes.h>
#include <vstgui/vstgui_uidescription.h>

#include "cnoteedit.h"

namespace MinMax
{
    class CNoteEditFactory
        : public  VSTGUI::ViewCreatorAdapter
    {
    public:

        CNoteEditFactory() { VSTGUI::UIViewFactory::registerViewCreator(*this); }

        VSTGUI::IdStringPtr getViewName() const override { return "UI:Note Edit"; }

        VSTGUI::IdStringPtr getBaseViewName() const override { return  VSTGUI::UIViewCreator::kCTextEdit; }

        VSTGUI::CView* create(const  VSTGUI::UIAttributes& attributes, const  VSTGUI::IUIDescription* description) const override
        {
            return new CNoteEdit(VSTGUI::CRect(VSTGUI::CPoint(0, 0), VSTGUI::CPoint(40, 15)), description->getController(), -1);
        }
    };

    CNoteEditFactory __gCNoteEditFactory;
}