#pragma once

#include <vstgui/vstgui.h>
#include <public.sdk/source/vst/vsteditcontroller.h>

#include "chordmap.h"

namespace MinMax
{
    using namespace VSTGUI;

    class CFretBoard
        : public CControl
    {
    public:
        CFretBoard(const CRect& size, EditControllerEx1* controller = nullptr);

        void draw(CDrawContext* pContext) override;

        void setValue(ParamID tag, int idx);
        // 弦数
        int numStrings = 6;

        // 押さえているフレット
        std::vector<int> pressedFrets;

    private:

        CChord chord{};

        CLASS_METHODS(CFretBoard, CControl)
    };

}
