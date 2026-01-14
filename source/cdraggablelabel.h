//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <cmath>
#include <functional>
#include <vstgui/lib/cbuttonstate.h>
#include <vstgui/lib/controls/ctextlabel.h>
#include <vstgui/lib/cpoint.h>
#include <vstgui/lib/crect.h>

namespace MinMax
{
    class CDraggableLabel 
        : public VSTGUI::CTextLabel
    {
    public:
        CDraggableLabel(
            const VSTGUI::CRect& size,
            std::function<void(CDraggableLabel*)> dragCb = nullptr
        )
            : CTextLabel(size), dragCallback(dragCb)
        {
        }

        // 選択中コード番号を保持
        void setChordNumber(int n) { chordNumber = n; }
        int getChordNumber() const { return chordNumber; }

        // ドラッグ開始コールバック
        std::function<void(CDraggableLabel*)> dragCallback;

    protected:
        VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override
        {
            dragStart = where;
            dragging = false;
            return VSTGUI::kMouseEventHandled;
        }

        VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override
        {
            if ((buttons & VSTGUI::kLButton) && distance(dragStart, where) > 3)
            {
                if (!dragging) 
                {
                    dragging = true;
                    if (dragCallback)
                    {
                        dragCallback(this);
                    }
                }
                return VSTGUI::kMouseEventHandled;
            }
            return VSTGUI::kMouseEventNotHandled;
        }

        VSTGUI::CMouseEventResult onMouseUp(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override
        {
            dragging = false;
            return VSTGUI::kMouseEventHandled;
        }

        static float distance(const VSTGUI::CPoint& a, const VSTGUI::CPoint& b)
        {
            float dx = a.x - b.x;
            float dy = a.y - b.y;
            return sqrtf(dx * dx + dy * dy);
        }

    private:
        VSTGUI::CPoint dragStart;
        int chordNumber = -1;
        bool dragging = false;
    };
}