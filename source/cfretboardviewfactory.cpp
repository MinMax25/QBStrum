//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------

#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>
#include <vstgui/vstgui_uidescription.h>

#include "myparameters.h"
#include "CFretBoard.h"
#include "chordmap.h"

namespace MinMax
{
    class CFretOptionMenu
        : public COptionMenu
    {
    public:
        CFretOptionMenu(const CRect& size, IControlListener* listener, int32_t tag, CFretBoard* fretBoard)
            : COptionMenu(size, listener, tag)
            , fretBoard(fretBoard)
        {
        }

        void valueChanged() override
        {
            if (fretBoard == nullptr) return;
            COptionMenu::valueChanged();
            fretBoard->setValue(getTag(), getCurrentIndex());
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

            CFretBoard* fretBoard = new CFretBoard(CRect(0, 20, 1120, size.getHeight() + 20));
            fretBoard->numStrings = STRING_COUNT;
            addView(fretBoard);

            rootMenu = new CFretOptionMenu(CRect(1, 1, 79, 18), description->getController(), static_cast<int32_t>(PARAM::CHORD_ROOT), fretBoard);
            rootMenu->setFontColor(kWhiteCColor);
            rootMenu->setBackColor(kBlackCColor);
            rootMenu->setTag(static_cast<int32_t>(PARAM::CHORD_ROOT));
            addView(rootMenu);

            typeMenu = new CFretOptionMenu(CRect(81, 1, 159, 18), description->getController(), static_cast<int32_t>(PARAM::CHORD_TYPE), fretBoard);
            typeMenu->setFontColor(kWhiteCColor);
            typeMenu->setBackColor(kBlackCColor);
            typeMenu->setTag(static_cast<int32_t>(PARAM::CHORD_TYPE));
            addView(typeMenu);

            posMenu = new CFretOptionMenu(CRect(161, 1, 239, 18), description->getController(), static_cast<int32_t>(PARAM::FRET_POSITION), fretBoard);
            posMenu->setFontColor(kWhiteCColor);
            posMenu->setBackColor(kBlackCColor);
            posMenu->setTag(static_cast<int32_t>(PARAM::FRET_POSITION));
            addView(posMenu);
        }

        ~CFretBoardView()
        {
            if (editor)
            {
                editor->release();
            }
        }


        CFretOptionMenu* rootMenu = nullptr;
        CFretOptionMenu* typeMenu = nullptr;
        CFretOptionMenu* posMenu = nullptr;

        CLASS_METHODS(CFretBoardView, CViewContainer)

    private:

        VST3Editor* editor{};
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