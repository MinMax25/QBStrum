//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------

#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/vstgui_uidescription.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>

#include "myparameters.h"
#include "cchordlabel.h"

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
 /*           hierMenu = new COptionMenu(CRect(1, 1, 39, 18), nullptr, -1);
            menuListener = std::make_unique<ChordSelectorListner>(editor);

            hierMenu->setFontColor(kWhiteCColor);
            hierMenu->setBackColor(kGreyCColor);
            hierMenu->registerOptionMenuListener(menuListener.get());*/

            //// ChordMap Ç©ÇÁäKëwÉfÅ[É^Çí«â¡
            //const auto& chordMap = ChordMap::Instance();
            //auto rootNames = chordMap.getRootNames();

            //for (int r = 0; r < (int)rootNames.size(); ++r)
            //{
            //    COptionMenu* typeMenuSub = new COptionMenu(CRect(0, 0, 150, 20), nullptr, -1);

            //    auto chordNames = chordMap.getChordNames(r);
            //    for (int t = 0; t < (int)chordNames.size(); ++t)
            //    {
            //        COptionMenu* posMenuSub = new COptionMenu(CRect(0, 0, 150, 20), nullptr, -1);

            //        auto posNames = chordMap.getFretPosNames(r, t);
            //        for (auto& posName : posNames)
            //        {
            //            posMenuSub->addEntry(posName.c_str());
            //        }

            //        typeMenuSub->addEntry(posMenuSub, chordNames[t].c_str());
            //    }

            //    hierMenu->addEntry(typeMenuSub, rootNames[r].c_str());
            //}

            //addView(hierMenu);

            //
            CChordLabel* chordLabel = new CChordLabel(CRect(41, 1, 180, 18));
            chordLabel->setFont(kNormalFontSmall);
            chordLabel->setListener(description->getController());
            chordLabel->setTag(PARAM::CHORD_NUMBER);
            addView(chordLabel);
        }

        ~CChordSelecter()
        {
            if (editor) editor->release();
        }

        //CLASS_METHODS(CChordSelecter, CViewContainer)

    protected:

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