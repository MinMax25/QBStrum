//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------

#include <filesystem>
#include <vector>
#include <string>
#include <base/source/fstring.h>
#include <public.sdk/source/vst/vstguieditor.h>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>
#include <vstgui/uidescription/iuidescription.h>
#include <vstgui/lib/vstguibase.h>
#include <vstgui/vstgui_uidescription.h>
#include <vstgui/vstgui.h>
#include <rapidjson/document.h>

#include "myparameters.h"
#include "files.h"
#include "cfretboard.h"
#include "cmenubutton.h"

namespace MinMax
{
    // メニュー要素追加
    template<typename F>
    inline void addCommand(VSTGUI::COptionMenu* menu, const VSTGUI::UTF8String& title, F&& cb)
    {
        auto* item = new VSTGUI::CCommandMenuItem(VSTGUI::CCommandMenuItem::Desc(title));
        item->setActions(std::forward<F>(cb));
        menu->addEntry(item);
    }

    // フレットボードビュー
    class CFretBoardView
        : public VSTGUI::CViewContainer
    {
    private:
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
                    , editor(editor_), paramID(paramID_)
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
            CChordSelecter(const VSTGUI::CRect& size, VSTGUI::VST3Editor* editor_, ParamID tag)
                : CViewContainer(size)
                , editor(editor_)
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

            void setEditing(bool state)
            {
                chordMenu->setVisible(!state); // 編集時はメニュー非表示
            }

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
        };

    public:
        CFretBoardView(const VSTGUI::UIAttributes& attributes, const VSTGUI::IUIDescription* description, const VSTGUI::CRect& size)
            : CViewContainer(size)
        {
            editor = dynamic_cast<VSTGUI::VST3Editor*>(description->getController());
            assert(editor && "CFretBoardView requires VST3Editor");

            setBackgroundColor(VSTGUI::kTransparentCColor);

            // --- FretBoard ---
            fretBoard = new CFretBoard(getViewSize(), editor, PARAM::CHORD_NUM);
            addView(fretBoard);

            // --- File Menu ---
            fileButton = new CMenuButton(VSTGUI::CRect(1, 1, 60, 18), editor, -1, "Preset", [this](CMenuButton* b) { popupFileMenu(getFrame(), b); });
            addView(fileButton);

            // --- Edit Menu ---
            editButton = new CMenuButton(VSTGUI::CRect(62, 1, 122, 18), editor, -1, "Edit", [this](CMenuButton* b) { popupEditMenu(getFrame(), b); });
            addView(editButton);

            // --- Chord Selecter ---
            chordSelecter = new CChordSelecter(VSTGUI::CRect(365, 1, 515, 19), editor, PARAM::CHORD_NUM);
            addView(chordSelecter);
        }

        ~CFretBoardView() override { }

        CLASS_METHODS(CFretBoardView, CViewContainer)

    protected:
        VSTGUI::VST3Editor* editor = nullptr;

        CFretBoard* fretBoard = nullptr;

        CChordSelecter* chordSelecter = nullptr;

        CMenuButton* fileButton = nullptr;

        CMenuButton* editButton = nullptr;

        std::string presetFileName;

        bool isEditing = false;

        StringSet originalPressedFrets;

        static void popupFileMenu(VSTGUI::CFrame* frame, const VSTGUI::CView* anchor)
        {
            auto* menu = new VSTGUI::COptionMenu();

            auto* openMenu = createOpenPresetMenu();
            auto* openItem = new VSTGUI::CMenuItem("Open", openMenu);
            menu->addEntry(openItem);
            openMenu->forget();

            addCommand(menu, "Save", [](VSTGUI::CCommandMenuItem*) { /* ... */ });

            VSTGUI::CRect r = anchor->getViewSize();
            VSTGUI::CPoint p(r.left, r.bottom);
            anchor->localToFrame(p);

            menu->popup(frame, p);
            menu->forget();
        }

        static VSTGUI::COptionMenu* createOpenPresetMenu()
        {
            auto* menu = new VSTGUI::COptionMenu();

            std::vector<std::string> files;
            if (Files::getPresetFiles(files) != kResultTrue || files.empty())
            {
                auto* item = new VSTGUI::CCommandMenuItem(VSTGUI::CCommandMenuItem::Desc("No Presets"));
                item->setEnabled(false);
                menu->addEntry(item);
                return menu;
            }

            int32_t tagBase = 10000;

            for (const auto& fullpath : files)
            {
                std::filesystem::path p(fullpath);

                std::string utf8 = p.filename().u8string();
                VSTGUI::UTF8String title(utf8.c_str());

                auto* item = new VSTGUI::CMenuItem(title, tagBase++);
                menu->addEntry(item);
            }

            return menu;
        }

        static void popupEditMenu(VSTGUI::CFrame* frame, const VSTGUI::CView* anchor)
        {
            auto* menu = new VSTGUI::COptionMenu();

            addCommand(menu, "Edit", [](VSTGUI::CCommandMenuItem*) { /* ... */ });
            addCommand(menu, "Commit Changes", [](VSTGUI::CCommandMenuItem*) { /* ... */ });
            addCommand(menu, "Cancel Changes", [](VSTGUI::CCommandMenuItem*) { /* ... */ });

            VSTGUI::CRect r = anchor->getViewSize();
            VSTGUI::CPoint p(r.left, r.bottom);
            anchor->localToFrame(p);

            menu->popup(frame, p);
            menu->forget();
        }

        void saveChordMap()
        {
            if (!fileButton) return;

            showDialog(
                fileButton, 
                VSTGUI::CNewFileSelector::kSelectSaveFile,
                [this](const std::string& path)
                {
                    ChordMap::Instance().saveToFile(path);
                }
            );
        }

        void saveEdits()
        {
            // 現在の押弦情報を取得
            auto pressed = fretBoard->getPressedFrets();

            // ChordMap の現在選択コード番号に対して更新
            auto* controller = editor->getController();
            ParamValue norm = controller->getParamNormalized(PARAM::CHORD_NUM);
            int chordNum = static_cast<int>(controller->normalizedParamToPlain(PARAM::CHORD_NUM, norm));
            ChordMap::Instance().setVoicing(chordNum, pressed);

            // 編集終了後、元の状態として保持しておく
            originalPressedFrets = pressed;
        }

        // ファイルダイアログ表示
        void showDialog(VSTGUI::CControl* pControl, VSTGUI::CNewFileSelector::Style style, std::function<void(const std::string& path)> fileSelected)
        {
            Files::createPresetDirectory();

            auto* selector = VSTGUI::CNewFileSelector::create(pControl->getFrame(), style);
            if (selector == nullptr) return;

            std::string defaultName = presetFileName.empty() ? "NewPreset.json" : std::filesystem::path(presetFileName).filename().string();

            selector->setTitle(Files::TITLE.c_str());

            if (!presetFileName.empty())
            {
                std::filesystem::path p(presetFileName);
                selector->setInitialDirectory(p.parent_path().string().c_str());
            }

            selector->setDefaultSaveName(defaultName.c_str());

            selector->addFileExtension(VSTGUI::CFileExtension(Files::FILTER.c_str(), Files::FILE_EXT.c_str()));

            pControl->getFrame()->setFocusView(nullptr);

            if (selector->runModal() && (int)selector->getNumSelectedFiles() == 1)
            {
                presetFileName = selector->getSelectedFile(0);
                fileSelected(presetFileName);
            }

            selector->forget();

            return;
        }
    };

    class CFretBoardViewFactory
        : public VSTGUI::ViewCreatorAdapter
    {
    public:

        CFretBoardViewFactory() { VSTGUI::UIViewFactory::registerViewCreator(*this); }

        VSTGUI::IdStringPtr getViewName() const override { return "UI:FretBoard View"; }

        VSTGUI::IdStringPtr getBaseViewName() const override { return VSTGUI::UIViewCreator::kCViewContainer; }

        VSTGUI::CView* create(const VSTGUI::UIAttributes& attributes, const VSTGUI::IUIDescription* description) const override
        {
            return new CFretBoardView(attributes, description, VSTGUI::CRect(VSTGUI::CPoint(0, 0), VSTGUI::CPoint(795, 170)));
        }
    };

    CFretBoardViewFactory __gCFretBoardViewFactory;
}