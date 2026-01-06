//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <vstgui/lib/controls/icontrollistener.h>
#include <vstgui/lib/crect.h>
#include <vstgui/lib/vstguibase.h>

#include <vstgui/lib/controls/cparamdisplay.h>

namespace MinMax
{
    class CChordListner
        : public VSTGUI::CParamDisplay
    {
    public:
        using onValueChanged = std::function<void(CChordListner*, int)>;

        CChordListner(VSTGUI::IControlListener* listener, int32_t tag, onValueChanged vc)
            : CParamDisplay(VSTGUI::CRect(0, 0, 0, 0))
            , vc(vc)
        {
            setVisible(false);
            setListener(listener);
            setTag(tag);
        }

        void valueChanged() override
        {
            if (vc) vc(this, (int)getValue());
        }

        CLASS_METHODS(CChordListner, CParamDisplay)

    protected:
        onValueChanged vc;
    };
}