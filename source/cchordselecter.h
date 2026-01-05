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
            ChordOptionMenu(const VSTGUI::CRect& size, VSTGUI::VST3Editor* editor_, ParamID paramID_)
                : COptionMenu(size, editor_, paramID_)
                , editor(editor_)
                , paramID(paramID_)
            {
            }

            void valueChanged() override
            {
                if (isEditing) return;

                if (!editor || !editor->getController())
                    return;

                int32_t idx = -1;
                if (auto* menu = getLastItemMenu(idx))
                {
                    if (auto* item = menu->getEntry(idx))
                    {
                        int value = item->getTag();

                        auto* controller = editor->getController();
                        controller->beginEdit(paramID);
                        ParamValue norm = controller->plainParamToNormalized(paramID, value);
                        controller->setParamNormalized(paramID, norm);
                        controller->performEdit(paramID, norm);
                        controller->endEdit(paramID);
                    }
                }
            }

            void setState(bool state)
            {
                isEditing = state;
            }

        protected:

            VSTGUI::VST3Editor* editor{};

            ParamID paramID;

            bool isEditing = false;
        };

        class CChordLabel : public VSTGUI::CTextLabel
        {
        public:
            CChordLabel(const VSTGUI::CRect& size)
                : CTextLabel(size)
            {
            }

            void draw(VSTGUI::CDrawContext* context) override
            {
                auto str = getText().getString();

                if (str.empty())
                {
                    drawBack(context);
                    return;
                }

                if (!std::isdigit(static_cast<unsigned char>(str[0])) && str[0] != '-')
                    return;

                int index = std::atoi(str.c_str());

                const auto& map = ChordMap::Instance();

                VSTGUI::UTF8String name =
                    (index >= 0 && index < map.getFlatCount())
                    ? map.getByIndex(index).displayName.c_str()
                    : "# undefined #";

                drawBack(context);
                drawPlatformText(context, name);
            }

            CLASS_METHODS(CChordLabel, CTextLabel)

        private:
        };

    public:
        using Callback = std::function<void(CChordSelecter*)>;

        CChordSelecter(const VSTGUI::CRect& size, VSTGUI::VST3Editor* editor_, ParamID tag, Callback cb)
            : CViewContainer(size)
            , editor(editor_)
            , callback(cb)
        {
            setBackgroundColor(VSTGUI::kTransparentCColor);

            // 階層化コードメニュー
            chordMenu = createChordOptionMenu(VSTGUI::CRect(1, 1, 17, 17), PARAM::CHORD_NUM);
            chordMenu->setBackColor(VSTGUI::kGreyCColor);
            addView(chordMenu);

            // コード名表示ラベル
            CChordLabel* chordLabel = new CChordLabel(VSTGUI::CRect(18, 1, 148, 17));
            chordLabel->setBackColor(VSTGUI::kWhiteCColor);
            chordLabel->setFontColor(VSTGUI::kBlackCColor);
            chordLabel->setFont(VSTGUI::kNormalFontSmaller);
            chordLabel->setListener(editor);
            chordLabel->setTag(PARAM::CHORD_NUM);
            addView(chordLabel);
        }

        ~CChordSelecter() {}

        CLASS_METHODS(CChordSelecter, CViewContainer)

    protected:
        VSTGUI::VST3Editor* editor{};

        ChordOptionMenu* chordMenu{};

        ChordOptionMenu* createChordOptionMenu(const VSTGUI::CRect& size, int32_t tag)
        {
            auto& chordMap = ChordMap::Instance();

            // トップレベル OptionMenu
            auto* rootMenu = new ChordOptionMenu(size, editor, tag);

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

        Callback callback;
    };
}