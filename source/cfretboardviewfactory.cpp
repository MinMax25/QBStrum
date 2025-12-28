//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------

#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>
#include <vstgui/vstgui_uidescription.h>
#include <vstgui/lib/controls/ioptionmenulistener.h>
#include <string>

#include "cfretboard.h"
#include "chordmap.h"
#include "cnoteedit.h"
#include "cnotelabel.h"

#include "debug_log.h"

namespace MinMax
{
    class ChordSelectorListner
        : public OptionMenuListenerAdapter
    {
    public:
        ChordSelectorListner(CFretBoard* fretborad)
            : fretBoard(fretborad)
        {
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
            return finish;
        }

    private:
        CFretBoard* fretBoard = nullptr;
    };

    class CFretBoardView
        : public CViewContainer
    {
    public:
        CFretBoardView(const UIAttributes& attributes, const IUIDescription* description, const CRect& size)
            : CViewContainer(size)
        {
            editor = dynamic_cast<VST3Editor*>(description->getController());
            editor->addRef();

            setBackgroundColor(kGreyCColor);

            int yOffSet = 22;
            for (int i = 0; i < 6; i++)
            {
                CNoteEdit* noteEdit = new CNoteEdit(CRect(10, 1 + yOffSet, 49, 15 + yOffSet), editor, 999);
                noteEdit->setBackColor(kWhiteCColor);
                noteEdit->setFontColor(kBlackCColor);
                noteEdit->setFont(kNormalFontSmaller);
                addView(noteEdit);

                CNoteLabel* noteLabel = new CNoteLabel(CRect(50, 1 + yOffSet, 89, 15 + yOffSet));
                noteLabel->setFont(kNormalFontSmaller);
                addView(noteLabel);

                yOffSet += 24;
            }

            CFretBoard* fretBoard = new CFretBoard(CRect(0, 20, 1120, size.getHeight() + 20));
            fretBoard->numStrings = STRING_COUNT;
            addView(fretBoard);

             CTextLabel* labHierMenu = new CTextLabel(CRect(300, 1, 399, 18));
            labHierMenu->setFont(kNormalFontSmall);
            labHierMenu->setText("Select Choard");
            addView(labHierMenu);

            // 階層化メニューを右に追加
            COptionMenu* hierMenu = new COptionMenu(CRect(400, 1, 439, 18), editor, 999);
            menuListener = std::make_unique<ChordSelectorListner>(fretBoard);

            hierMenu->setFontColor(kWhiteCColor);
            hierMenu->setBackColor(kGreyCColor);
            hierMenu->registerOptionMenuListener(menuListener.get());

            // ChordMap から階層データを追加
            const auto& chordMap = ChordMap::Instance();
            auto rootNames = chordMap.getRootNames();

            for (int r = 0; r < (int)rootNames.size(); ++r)
            {
                COptionMenu* typeMenuSub = new COptionMenu(CRect(0, 0, 150, 20), editor, 999);

                auto chordNames = chordMap.getChordNames(r);
                for (int t = 0; t < (int)chordNames.size(); ++t)
                {
                    COptionMenu* posMenuSub = new COptionMenu(CRect(0, 0, 150, 20), editor, 999);

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

            CTextLabel* labChord = new CTextLabel(CRect(440, 1, 539, 18));
            labChord->setFont(kNormalFontSmall);
            labChord->setText("C M7 (1)");
            addView(labChord);
        }

        CFretBoardView(const CFretBoardView& other)
            : CViewContainer(other)
            , editor(other.editor)
        {
            if (editor) editor->addRef();
            menuListener.reset();
            hierMenu = nullptr;
        }

        ~CFretBoardView()
        {
            if (hierMenu && menuListener) hierMenu->unregisterOptionMenuListener(menuListener.get());
            menuListener.reset();
            if (editor) editor->release();
        }

        CLASS_METHODS(CFretBoardView, CViewContainer)

    private:

        VST3Editor* editor{};
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