//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------

#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/vstgui_uidescription.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>

#include "myparameters.h"
#include "cchordlabel.h"
#include <map>

#include <vstgui/lib/vstguibase.h>

namespace MinMax
{
    using namespace VSTGUI;

    class ChordOptionMenu : public COptionMenu
    {
    public:
        ChordOptionMenu(const CRect& size, VST3Editor* editor, int32_t tag)
            : COptionMenu(size, editor, tag)
            , editor(editor), tag(tag)
        {
        }

        void valueChanged() override
        {
            if (!lastMenu || !editor) return;

            int value = lastMenu->getCurrent()->getTag();
            editor->getController()->beginEdit(tag);
            ParamValue norm = editor->getController()->plainParamToNormalized(tag, value);
            editor->getController()->setParamNormalized(tag, norm);
            editor->getController()->performEdit(tag, norm);
            editor->getController()->endEdit(tag);
        }

    protected:
        VST3Editor* editor{};
        ParamID tag;
    };

    class CChordSelecter
        : public CViewContainer
    {
    private:

        ChordOptionMenu* createChordOptionMenu(const CRect& size, IControlListener* listener, int32_t tag)
        {
            auto& chordMap = ChordMap::Instance();

            // トップレベル OptionMenu
            auto* rootMenu = new ChordOptionMenu(size, editor, tag);

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

                        auto* voicingItem = new CMenuItem(chordMap.getVoicingName(r, t, v).c_str(), flatIndex);

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

    public:

        CChordSelecter(const UIAttributes& attributes, const IUIDescription* description, const CRect& size)
            : CViewContainer(size)
        {
            editor = static_cast<VST3Editor*>(description->getController());
            if (editor) editor->addRef();

            CColor backGroundColor;
            description->getColor(u8"Dark", backGroundColor);
            setBackgroundColor(backGroundColor);

            // 階層化コードメニュー
            chordMenu = createChordOptionMenu(CRect(1, 1, 40, 18), editor, PARAM::CHORD_NUM);
            chordMenu->setBackColor(kGreyCColor);
            addView(chordMenu);

            //
            CChordLabel* chordLabel = new CChordLabel(CRect(41, 1, 180, 18));
            chordLabel->setFont(kNormalFontSmall);
            chordLabel->setListener(editor);
            chordLabel->setTag(PARAM::CHORD_NUM);
            addView(chordLabel);
        }

        ~CChordSelecter()
        {
            if (editor) editor->release();
        }

        CLASS_METHODS(CChordSelecter, CViewContainer)

    protected:
        ChordOptionMenu* chordMenu{};
        bool updatingFromParam = false;
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