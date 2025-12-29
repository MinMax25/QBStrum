//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------

#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>
#include <vstgui/lib/controls/ioptionmenulistener.h>
#include <vstgui/lib/controls/ctextlabel.h>
#include <vstgui/vstgui_uidescription.h>
#include <string>

#include "chordmap.h"
#include "cfretboard.h"
#include "cchordlabel.h"

namespace MinMax
{
    using namespace Steinberg;
    using namespace Steinberg::Vst;

    class ChordSelectorListner
        : public OptionMenuListenerAdapter
    {
    public:
        ChordSelectorListner(CFretBoard* fretborad, VST3Editor* editor)
            : fretBoard(fretborad)
            , editor(editor)
        {
            if (fretBoard)
            {
                CChord chord{};
                fretBoard->setValue(chord);
            }
        }

        bool onOptionMenuSetPopupResult(COptionMenu* menu, COptionMenu* selectedMenu, int32_t selectedIndex) override
        {
            if (!menu || !selectedMenu) return false;

            CChord chord;

            bool finish = false;
            int chordNumber = 0; // 先頭からの位置インデックス

            for (int r = 0; r < menu->getNbEntries(); ++r)
            {
                COptionMenu* typeMenu = menu->getSubMenu(r);
                if (!typeMenu) continue;

                for (int t = 0; t < typeMenu->getNbEntries(); ++t)
                {
                    COptionMenu* posMenu = typeMenu->getSubMenu(t);
                    if (!posMenu) continue;

                    if (posMenu == selectedMenu)
                    {
                        chord.root = r;
                        chord.type = t;
                        chord.position = selectedIndex;

                        fretBoard->setValue(chord);

                        chordNumber += selectedIndex;

                        finish = true;
                    }
                    if (finish) break;
                    chordNumber += posMenu->getNbEntries();
                }
                if (finish) break;
            }
 
            if (finish)
            {
                EditController* controller = dynamic_cast<EditController*>(editor->getController());
                if (!controller) return false;
                controller->beginEdit(1104);
                ParamValue norm = controller->plainParamToNormalized(1104, chordNumber);
                controller->setParamNormalized(1104, norm);
                controller->performEdit(1104, norm);
                controller->endEdit(1104);
            }

            return finish;
        }

    private:
        CFretBoard* fretBoard = nullptr;
        VST3Editor* editor = nullptr;
    };

    class CFretBoardView
        : public CViewContainer
    {
    public:
        CFretBoardView(const UIAttributes& attributes, const IUIDescription* description, const CRect& size)
            : CViewContainer(size)
        {
            editor = dynamic_cast<VST3Editor*>(description->getController());
            //if (editor) editor->addRef();

            setBackgroundColor(kGreyCColor);

            CFretBoard* fretBoard = new CFretBoard(CRect(0, 20, 1120, size.getHeight() + 20));
            fretBoard->numStrings = STRING_COUNT;
            addView(fretBoard);

            // 階層化メニュー
            hierMenu = new COptionMenu(CRect(400, 1, 429, 18), nullptr, -1);
            menuListener = std::make_unique<ChordSelectorListner>(fretBoard, editor);

            hierMenu->setFontColor(kWhiteCColor);
            hierMenu->setBackColor(kGreyCColor);
            hierMenu->registerOptionMenuListener(menuListener.get());

            // ChordMap から階層データを追加
            const auto& chordMap = ChordMap::Instance();
            auto rootNames = chordMap.getRootNames();

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
            }

            addView(hierMenu);

            CChordLabel* labChord = new CChordLabel(CRect(430, 1, 549, 18));
            labChord->setFont(kNormalFontSmall);
            labChord->setListener(editor);
            labChord->setTag(1104);
            addView(labChord);
        }

        CFretBoardView(const CFretBoardView&) = delete;

        ~CFretBoardView()
        {
            menuListener.reset();
        }

        bool removed(CView* parent) override
        {
            if (hierMenu && menuListener)
            {
                hierMenu->unregisterOptionMenuListener(menuListener.get());
            }
            return CViewContainer::removed(parent);
        }

        //CLASS_METHODS(CFretBoardView, CViewContainer)

    private:

        VST3Editor* editor = nullptr;;
        std::unique_ptr<ChordSelectorListner> menuListener;
        COptionMenu* hierMenu = nullptr;
    };

    class CFretBoardViewFactory
        : public ViewCreatorAdapter
    {
    public:

        CFretBoardViewFactory() { UIViewFactory::registerViewCreator(*this); }

        IdStringPtr getViewName() const override { return "UI:FretBoard View"; }

        IdStringPtr getBaseViewName() const override { return UIViewCreator::kCViewContainer; }

        CView* create(const UIAttributes& attributes, const IUIDescription* description) const override
        {
            return new CFretBoardView(attributes, description, CRect(CPoint(0, 0), CPoint(660, 160)));
        }
    };

    CFretBoardViewFactory __gCFretBoardViewFactory;
}