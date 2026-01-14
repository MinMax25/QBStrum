#pragma once

#if defined(_WIN32)
#include "winfiledragdrop.h"
#endif

#include <cmath>
#include <functional>
#include <string>

#include <vstgui/lib/cbuttonstate.h>
#include <vstgui/lib/controls/ctextlabel.h>
#include <vstgui/lib/cpoint.h>
#include <vstgui/lib/crect.h>

namespace MinMax
{
    class CDraggableLabel : public VSTGUI::CTextLabel
    {
    public:
        // ドラッグ開始時に「D&D するファイルパス」を返す
        using DragPathCallback = std::function<std::wstring(CDraggableLabel*)>;

        CDraggableLabel(
            const VSTGUI::CRect& size,
            DragPathCallback cb = nullptr
        )
            : CTextLabel(size)
            , dragPathCallback(cb)
        {
        }

        void setChordNumber(int n) { chordNumber = n; }
        int  getChordNumber() const { return chordNumber; }

    protected:
        VSTGUI::CMouseEventResult onMouseDown(
            VSTGUI::CPoint& where,
            const VSTGUI::CButtonState&
        ) override
        {
            dragStart = where;
            dragging = false;
            return VSTGUI::kMouseEventHandled;
        }

        VSTGUI::CMouseEventResult onMouseMoved(
            VSTGUI::CPoint& where,
            const VSTGUI::CButtonState& buttons
        ) override
        {
            if ((buttons & VSTGUI::kLButton) &&
                !dragging &&
                distance(dragStart, where) > 3)
            {
                dragging = true;

#if defined(_WIN32)
                if (dragPathCallback)
                {
                    std::wstring path = dragPathCallback(this);
                    if (!path.empty())
                    {
                        WinDND::startFileDrag(path);
                    }
                }
#endif
                return VSTGUI::kMouseEventHandled;
            }
            return VSTGUI::kMouseEventNotHandled;
        }

        VSTGUI::CMouseEventResult onMouseUp(
            VSTGUI::CPoint&,
            const VSTGUI::CButtonState&
        ) override
        {
            dragging = false;
            return VSTGUI::kMouseEventHandled;
        }

    private:
        static float distance(const VSTGUI::CPoint& a, const VSTGUI::CPoint& b)
        {
            float dx = a.x - b.x;
            float dy = a.y - b.y;
            return std::sqrt(dx * dx + dy * dy);
        }

        VSTGUI::CPoint dragStart{};
        int  chordNumber = -1;
        bool dragging = false;

        DragPathCallback dragPathCallback;
    };
}
