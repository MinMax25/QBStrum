#pragma once

#include "chordmap.h"
#include <vstgui/lib/controls/ctextlabel.h>
#include <vstgui/lib/cdrawcontext.h>
#include <string>

namespace MinMax
{
    using namespace VSTGUI;

    class CChordLabel : public CTextLabel
    {
    public:
        CChordLabel(const CRect& size)
            : CTextLabel(size)
        {
        }

        void draw(CDrawContext* context) override
        {
            auto str = getText().getString();
            if (str.empty()) return;

            int index = std::stoi(str);

            const auto& map = ChordMap::Instance();

            UTF8String name =
                (index >= 0 && index < map.getFlatCount())
                ? map.getByIndex(index).displayName.c_str()
                : "---";

            drawBack(context);
            drawPlatformText(context, name);
        }

        CLASS_METHODS(CChordLabel, CTextLabel)

    private:
    };
}
