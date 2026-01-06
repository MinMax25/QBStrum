//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>
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