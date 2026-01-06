//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>
#include <vstgui/vstgui_uidescription.h>

#include "cnotelabel.h"

namespace MinMax
{
    class CNoteLabelFactory
        : public VSTGUI::ViewCreatorAdapter
    {
    public:

        CNoteLabelFactory() { VSTGUI::UIViewFactory::registerViewCreator(*this); }

        VSTGUI::IdStringPtr getViewName() const override { return "UI:Note Label"; }

        VSTGUI::IdStringPtr getBaseViewName() const override { return VSTGUI::UIViewCreator::kCTextLabel; }

        VSTGUI::CView* create(const VSTGUI::UIAttributes& attributes, const VSTGUI::IUIDescription* description) const override
        {
            return new CNoteLabel(VSTGUI::CRect(VSTGUI::CPoint(0, 0), VSTGUI::CPoint(40, 15)));
        }
    };

    CNoteLabelFactory __gCNoteLabelFactory;
}