//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <string>
#include <vstgui/lib/ccolor.h>
#include <vstgui/lib/cfont.h>
#include <vstgui/lib/controls/coptionmenu.h>
#include <vstgui/lib/controls/ctextlabel.h>
#include <vstgui/lib/crect.h>
#include <vstgui/lib/cstring.h>
#include <vstgui/lib/cviewcontainer.h>
#include <vstgui/lib/vstguibase.h>
#include <functional> 

#include "chordmap.h"
#include "myparameters.h"
#include "cdraggablelabel.h"
#include "midiwriter.h"

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
			chordMenu->setFontColor(VSTGUI::kTransparentCColor);
            chordMenu->setBackColor(VSTGUI::kGreyCColor);
			chordMenu->setWantsFocus(false);
            addView(chordMenu);

            // コード名表示ラベル
            chordLabel =
                new CDraggableLabel(
                    VSTGUI::CRect(18, 1, 148, 17),
                    [this](CDraggableLabel* lbl)
                    {
                        return dropVoicingMidiData(lbl);
                    }
                );
            chordLabel->setBackColor(VSTGUI::kWhiteCColor);
            chordLabel->setFontColor(VSTGUI::kBlackCColor);
            chordLabel->setFont(VSTGUI::kNormalFontSmaller);
            addView(chordLabel);
        }

        ~CChordSelecter() {}
        
        void beginEdit()
        {
            canEdit_ = true;
            chordMenu->setVisible(false);
            chordLabel->setFontColor(VSTGUI::kRedCColor);
            chordLabel->setBackColor(VSTGUI::kBlackCColor);
        }

        void endEdit()
        {
            canEdit_ = false;
            chordMenu->setVisible(true);
            chordLabel->setFontColor(VSTGUI::kBlackCColor);
            chordLabel->setBackColor(VSTGUI::kWhiteCColor);
        }

        int getCurrentChordNumber()
        {
            return currentChordNumber;
        }

        void setChordNumber(int value)
        {
            currentChordNumber = value;
            setChordText(value);
        }

        CLASS_METHODS(CChordSelecter, CViewContainer)

    protected:
        bool canEdit_ = true;

        ChordOptionMenu* chordMenu{};

        int currentChordNumber = 0;

        CDraggableLabel* chordLabel = nullptr;

        SelectedChordChanged selectedChordChanged;

        ChordOptionMenu* createChordOptionMenu(const VSTGUI::CRect& size, int32_t tag)
        {
            auto& chordMap = ChordMap::instance();

            // トップレベル OptionMenu
            auto* rootMenu =
                new ChordOptionMenu(
                    size,
                    [this](ChordOptionMenu* p, int value) 
                    {
                        onSelectedChordChanged(value);
                    }
                );

            int flatIndex = 0;
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
                        auto* voicingItem = new VSTGUI::CMenuItem(std::to_string(v + 1), flatIndex++);

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
            currentChordNumber = value;
            if (selectedChordChanged) selectedChordChanged(this, value);
        }

        void setChordText(int flatIndex)
        {
            const auto& map = ChordMap::instance();

            VSTGUI::UTF8String name =
                (flatIndex >= 0 && flatIndex < map.getFlatCount())
                ? map.getChordInfoByIndex(flatIndex).displayName.c_str()
                : "# undefined #";

            if (auto* lbl = dynamic_cast<CDraggableLabel*>(chordLabel))
            {
                lbl->setText(name);
            }
        }

        std::wstring dropVoicingMidiData(CDraggableLabel* pControl)
        {
            // 1. 一時フォルダを取得
            auto tempPath = MinMax::Files::getTempPath();

            // 2. ファイル名ベース
            std::wstring baseName = L"voicing";
            std::wstring filename;
            int counter = 0;

            // 3. 一時ファイル名衝突回避ループ
            while (true)
            {
                if (counter == 0)
                {
                    filename = baseName + L".mid";
                }
                else
                {
                    filename = baseName + L"_" + std::to_wstring(counter) + L".mid";
                }

                auto fullpath = tempPath / std::filesystem::path(filename.begin(), filename.end());
                if (!std::filesystem::exists(fullpath)) break;

                ++counter;
            }

            auto fullpath = tempPath / std::filesystem::path(filename.begin(), filename.end());

            // 4. コードボイシング取得
            auto v = ChordMap::instance().getChordVoicing(currentChordNumber);
            for (int i = 0; i < STRING_COUNT; i++)
            {
                if (v.data[i] >= 0)
                {
                    v.data[i] += ChordMap::instance().getTunings().data[i];
                }
            }

            // 5. MIDIデータ出力
            writeChordToMidi(v, fullpath.wstring());

            return fullpath.wstring();
        }

        bool writeChordToMidi(const StringSet& chord, const std::wstring& filepath)
        {
            // MidiWriter 初期化（480 ticks per quarter note）
            MidiWriter writer(480);

            uint32_t fullNoteLength = 480 * 4; // 全音符 = 1小節

            for (int i = 0; i < STRING_COUNT; ++i)
            {
                int note = chord.data[i];
                if (note >= 0)
                {
                    writer.addNote(note, 100, 0, fullNoteLength); // velocity 100, startTick 0
                }
            }

            // ファイル出力
            return writer.write(filepath.c_str(), chordLabel->getText());
        }
    };
}