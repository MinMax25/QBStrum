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

namespace MinMax
{
    using namespace VSTGUI;
    class ChordSelectorListner
        : public OptionMenuListenerAdapter
    {
    public:
        ChordSelectorListner(VST3Editor* editor)
            : editor(editor)
        {
        }

        bool onOptionMenuSetPopupResult(COptionMenu* menu, COptionMenu* selectedMenu, int32_t selectedIndex) override
        {
            if (!menu || !selectedMenu) return false;
            return false;
        }

    private:
        VST3Editor* editor = nullptr;
    };

    class CChordSelecter
        : public CViewContainer
    {
    public:

        CChordSelecter(const UIAttributes& attributes, const IUIDescription* description, const CRect& size)
            : CViewContainer(size)
        {
            editor = static_cast<VST3Editor*>(description->getController());
            if (editor) editor->addRef();


            CColor ccc;
            description->getColor(u8"Dark", ccc);
            setBackgroundColor(ccc);

            // äKëwâªÉÅÉjÉÖÅ[
            hierMenu = new COptionMenu(CRect(1, 1, 39, 18), nullptr, -1);
            menuListener = std::make_unique<ChordSelectorListner>(editor);

            hierMenu->setFontColor(kWhiteCColor);
            hierMenu->setBackColor(kGreyCColor);
            hierMenu->registerOptionMenuListener(menuListener.get());

            // ChordMap Ç©ÇÁäKëwÉfÅ[É^Çí«â¡
            const auto& chordMap = ChordMap::Instance();

            CMenuItem* rmenu = nullptr;
            CMenuItem* tmenu = nullptr;
            CMenuItem* pmenu = nullptr;

            int r = -1;
            int t = -1;
            int p = -1;

            for (int i = 0; i < chordMap.getFlatCount(); i++)
            {
                auto& f = chordMap.getByIndex(i);

                if (r != f.root)
                {
                }
                else if (t != f.type)
                {
                }
                else if (p != f.type)
                {
                }
            }
 /*           auto rootNames = chordMap.getRootNames();

            for (int r = 0; r < (int)rootNames.size(); ++r)
            {
                COptionMenu* typeMenuSub = new COptionMenu(CRect(0, 0, 150, 20), nullptr, -1);

                auto chordNames = chordMap.getChordNames(r);
                for (int t = 0; t < (int)chordNames.size(); ++t)
                {
                    COptionMenu* posMenuSub = new COptionMenu(CRect(0, 0, 150, 20), nullptr, -1);

                    auto posNames = chordMap.getFretPosNames(r, t);
                    for (auto& posName : posNames)
                    {
                        posMenuSub->addEntry(posName.c_str());
                    }

                    typeMenuSub->addEntry(posMenuSub, chordNames[t].c_str());
                }

                hierMenu->addEntry(typeMenuSub, rootNames[r].c_str());
            }*/

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

        //CLASS_METHODS(CChordSelecter, CViewContainer)

    protected:

        COptionMenu* createChordOptionMenu(int32_t tag)
        {
            CRect size(0, 0, 180, 20);
            auto* menu = new COptionMenu(size, nullptr, -1);
            menu->setTag(tag);

            const auto& chordMap = ChordMap::Instance();

            std::map<
                std::string,
                std::map<std::string, std::vector<int>>
            > hierarchy;

            for (int i = 0; i < chordMap.getFlatCount(); ++i)
            {
                const auto& e = chordMap.getByIndex(i);
                hierarchy[e.rootName][e.typeName].push_back(i);
            }

            for (auto& [rootName, typeMap] : hierarchy)
            {
                auto rootItem = makeOwned<CMenuItem>(rootName.c_str());
                CMenuItemList rootList;

                for (auto& [typeName, indices] : typeMap)
                {
                    auto typeItem = makeOwned<CMenuItem>(typeName.c_str());
                    CMenuItemList typeList;

                    for (int flatIndex : indices)
                    {
                        const auto& e = chordMap.getByIndex(flatIndex);

                        typeList.push_back(
                            makeOwned<CMenuItem>(
                                e.posName.c_str(),
                                flatIndex
                            )
                        );
                    }

                    // Åö ÉTÉuÉÅÉjÉÖÅ[ÇÕ CMenuItem Ç…ÇæÇØ setSubmenu
                    typeItem->setSubmenu(std::move(typeList));
                    rootList.push_back(std::move(typeItem));
                }

                // Åö Ç±Ç±Ç‡ CMenuItem
                rootItem->setSubmenu(std::move(rootList));

                // Åö OptionMenu Ç…ìnÇ∑ÇÃÇÕ CMenuItem ÇæÇØ
                menu->addEntry(std::move(rootItem));
            }

            return menu;
        }

        VST3Editor* editor{};
        COptionMenu* hierMenu = nullptr;
        std::unique_ptr<ChordSelectorListner> menuListener;
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