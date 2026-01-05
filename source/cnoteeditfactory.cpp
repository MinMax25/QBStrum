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
        : public ViewCreatorAdapter
    {
    public:

        CNoteEditFactory() { UIViewFactory::registerViewCreator(*this); }

        IdStringPtr getViewName() const override { return "UI:Note Edit"; }

        IdStringPtr getBaseViewName() const override { return UIViewCreator::kCTextEdit; }

        CView* create(const UIAttributes& attributes, const IUIDescription* description) const override
        {
            return new CNoteEdit(CRect(CPoint(0, 0), CPoint(40, 15)), description->getController(), -1);
        }
    };

    CNoteEditFactory __gCNoteEditFactory;
}