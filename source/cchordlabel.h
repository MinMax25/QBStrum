#pragma once

#include "chordmap.h"
#include <vstgui/lib/controls/ctextlabel.h>
#include <vstgui/lib/cdrawcontext.h>

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
            size_t index = std::stoi(getText().getString());

            const auto& map = ChordMap::Instance();

            UTF8String name =
                (index >= 0 && index < map.getCount())
                ? map.getByIndex(index).displayName.c_str()
                : "---";

            drawBack(context);
            drawPlatformText(context, name);
        }

        CLASS_METHODS(CChordLabel, CTextLabel)

    private:
    };
}
