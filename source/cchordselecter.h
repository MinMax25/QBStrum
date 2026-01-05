//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <pluginterfaces/vst/vsttypes.h>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <string>
#include <vstgui/lib/ccolor.h>
#include <vstgui/lib/cdrawcontext.h>
#include <vstgui/lib/cfont.h>
#include <vstgui/lib/controls/coptionmenu.h>
#include <vstgui/lib/controls/ctextlabel.h>
#include <vstgui/lib/crect.h>
#include <vstgui/lib/cstring.h>
#include <vstgui/lib/cviewcontainer.h>
#include <vstgui/lib/vstguibase.h>
#include <vstgui/plugin-bindings/vst3editor.h>

#include "chordmap.h"
#include "myparameters.h"

namespace MinMax
{
    // コード選択
    class CChordSelecter
        : public VSTGUI::CViewContainer
    {
    private:
        class ChordOptionMenu
            : public VSTGUI::COptionMenu
        {
        public:
            using SelectedChordChanged = std::function<void(ChordOptionMenu*, int)>;

            ChordOptionMenu(const VSTGUI::CRect& size, SelectedChordChanged cb)
                : COptionMenu(size, nullptr, -1)
                , selectedChordChanged(cb)
            {
            }

            void valueChanged() override
            {
                int32_t idx = -1;
                if (auto* menu = getLastItemMenu(idx))
                {
                    if (auto* item = menu->getEntry(idx))
                    {
                        int value = item->getTag();
                        selectedChordChanged(this, value);
                    }
                }
            }

        protected:
            SelectedChordChanged selectedChordChanged;
        };

    public:
        using SelectedChordChanged = std::function<void(CChordSelecter*, int)>;

        CChordSelecter(const VSTGUI::CRect& size, SelectedChordChanged cb)
            : CViewContainer(size)
            , selectedChordChanged(cb)
        {
            setBackgroundColor(VSTGUI::kTransparentCColor);

            // 階層化コードメニュー
            chordMenu = createChordOptionMenu(VSTGUI::CRect(1, 1, 17, 17), PARAM::CHORD_NUM);
            chordMenu->setBackColor(VSTGUI::kGreyCColor);
            addView(chordMenu);

            // コード名表示ラベル
            chordLabel = new VSTGUI::CTextLabel(VSTGUI::CRect(18, 1, 148, 17));
            chordLabel->setBackColor(VSTGUI::kWhiteCColor);
            chordLabel->setFontColor(VSTGUI::kBlackCColor);
            chordLabel->setFont(VSTGUI::kNormalFontSmaller);
            setChordText(0);
            addView(chordLabel);
        }

        ~CChordSelecter() {}

        CLASS_METHODS(CChordSelecter, CViewContainer)

    protected:
        ChordOptionMenu* chordMenu{};

        VSTGUI::CTextLabel* chordLabel = nullptr;

        SelectedChordChanged selectedChordChanged;

        ChordOptionMenu* createChordOptionMenu(const VSTGUI::CRect& size, int32_t tag)
        {
            auto& chordMap = ChordMap::Instance();

            // トップレベル OptionMenu
            auto* rootMenu = new ChordOptionMenu(size, [this](ChordOptionMenu* p, int value) { onSelectedChordChanged(value); });

            for (int r = 0; r < chordMap.getRootCount(); ++r)
            {
                // ルート項目
                auto* rootItem = new VSTGUI::CMenuItem(chordMap.getRootName(r).c_str());

                // タイプ用サブメニュー
                auto* typeMenu = new VSTGUI::COptionMenu(VSTGUI::CRect(0, 0, 0, 0), nullptr, -1);

                for (int t = 0; t < chordMap.getTypeCount(r); ++t)
                {
                    auto* typeItem = new VSTGUI::CMenuItem(chordMap.getTypeName(r, t).c_str());

                    // ボイシング用サブメニュー
                    auto* voicingMenu = new VSTGUI::COptionMenu(VSTGUI::CRect(0, 0, 0, 0), nullptr, -1);

                    for (int v = 0; v < chordMap.getVoicingCount(r, t); ++v)
                    {
                        int flatIndex = chordMap.getFlatIndex(r, t, v);

                        auto* voicingItem = new VSTGUI::CMenuItem(chordMap.getVoicingName(r, t, v).c_str(), flatIndex);

                        voicingMenu->addEntry(voicingItem);
                    }

                    typeItem->setSubmenu(voicingMenu);
                    voicingMenu->forget();

                    typeMenu->addEntry(typeItem);
                }

                rootItem->setSubmenu(typeMenu);
                typeMenu->forget();

                rootMenu->addEntry(rootItem);
            }

            return rootMenu;
        }

        void onSelectedChordChanged(int value)
        {
            setChordText(value);
            if (selectedChordChanged) selectedChordChanged(this, value);
        }

        void setChordText(int value)
        {
            const auto& map = ChordMap::Instance();

            VSTGUI::UTF8String name =
                (value >= 0 && value < map.getFlatCount())
                ? map.getByIndex(value).displayName.c_str()
                : "# undefined #";

            if (chordLabel) chordLabel->setText(name);
        }
    };
}