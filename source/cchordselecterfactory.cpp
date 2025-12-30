//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------

#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/vstgui_uidescription.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>
#include <memory>

#include "myparameters.h"
#include "cchordlabel.h"
#include <map>

#include "debug_log.h"
#include <utility>
#include <vstgui/lib/vstguibase.h>

namespace MinMax
{
    using namespace VSTGUI;

    class CChordSelecter
        : public CViewContainer
    {
    private:

        COptionMenu* createChordOptionMenu()
        {
            auto& chordMap = ChordMap::Instance();

            // トップレベル OptionMenu
            auto* rootMenu = new COptionMenu(CRect(0, 0, 0, 0), nullptr, -1);

            for (int r = 0; r < chordMap.getRootCount(); ++r)
            {
                // ルート項目
                auto* rootItem = new CMenuItem(chordMap.getRootName(r).c_str());

                // タイプ用サブメニュー
                auto* typeMenu = new COptionMenu(CRect(0, 0, 0, 0), nullptr, -1);

                for (int t = 0; t < chordMap.getTypeCount(r); ++t)
                {
                    auto* typeItem = new CMenuItem(chordMap.getTypeName(r, t).c_str());

                    // ボイシング用サブメニュー
                    auto* voicingMenu = new COptionMenu(CRect(0, 0, 0, 0), nullptr, -1);

                    for (int v = 0; v < chordMap.getVoicingCount(r, t); ++v)
                    {
                        int flatIndex = chordMap.getFlatIndex(r, t, v);

                        auto* voicingItem = new CMenuItem(
                            chordMap.getVoicingName(r, t, v).c_str(),
                            flatIndex
                        );

                        voicingMenu->addEntry(voicingItem);
                    }

                    typeItem->setSubmenu(voicingMenu);
                    typeMenu->addEntry(typeItem);
                }

                rootItem->setSubmenu(typeMenu);
                rootMenu->addEntry(rootItem);
            }

            return rootMenu;
        }

    public:

        CChordSelecter(const UIAttributes& attributes, const IUIDescription* description, const CRect& size)
            : CViewContainer(size)
        {
            editor = static_cast<VST3Editor*>(description->getController());
            if (editor) editor->addRef();

            CColor backGroundColor;
            description->getColor(u8"Dark", backGroundColor);
            setBackgroundColor(backGroundColor);

            // 階層化メニュー
            auto* hierMenu = createChordOptionMenu();
            hierMenu->setViewSize(CRect(1, 1, 40, 18));
            hierMenu->setBackColor(kGreyCColor);

            addView(hierMenu);

            //
            CChordLabel* chordLabel = new CChordLabel(CRect(41, 1, 180, 18));
            chordLabel->setFont(kNormalFontSmall);
            chordLabel->setListener(description->getController());
            chordLabel->setTag(PARAM::CHORD_NUM);
            addView(chordLabel);
        }

        ~CChordSelecter()
        {
            if (editor) editor->release();
        }

        CLASS_METHODS(CChordSelecter, CViewContainer)

    protected:

        VST3Editor* editor{};
    };

    class CChordSelecterFactory
        : public ViewCreatorAdapter
    {
    public:

        CChordSelecterFactory() { UIViewFactory::registerViewCreator(*this); }

        IdStringPtr getViewName() const override { return "UI:Chord Selecter View"; }

        IdStringPtr getBaseViewName() const override { return UIViewCreator::kCViewContainer; }

        CView* create(const UIAttributes& attributes, const IUIDescription* description) const override
        {
            return new CChordSelecter(attributes, description, CRect(CPoint(0, 0), CPoint(182, 20)));
        }
    };

    CChordSelecterFactory __gCCChordSelecterFactory;
}