//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <array>
#include <string>
#include <vstgui/lib/cdrawcontext.h>
#include <vstgui/lib/controls/ccontrol.h>
#include <vstgui/lib/controls/cparamdisplay.h>
#include <vstgui/lib/controls/ctextlabel.h>
#include <vstgui/lib/crect.h>
#include <vstgui/lib/cstring.h>
#include <vstgui/lib/vstguibase.h>

#include "plugdefine.h"

namespace MinMax
{
    // ノート名表示ラベル
    class CNoteLabel
        : public  VSTGUI::CTextLabel
    {
        static constexpr int NOTE_COUNT = 128;
    public:
        CNoteLabel(const  VSTGUI::CRect& size)
            : CTextLabel(size)
        {

        }

        void CNoteLabel::draw(VSTGUI::CDrawContext* pContext) override
        {
            VSTGUI::UTF8String value = "0" + getText();
            int row = std::stoi(value.getString());
            row = row < NOTE_COUNT ? row : 127;

            VSTGUI::UTF8String cellValue;
            cellValue = NoteNames[row];

            drawBack(pContext);
            drawPlatformText(pContext, cellValue);
            setDirty(false);
        }

        CLASS_METHODS(CNoteLabel, CTextLabel)
    };
}