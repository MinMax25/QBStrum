//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <base/source/fstring.h>
#include <filesystem>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <public.sdk/source/vst/vstguieditor.h>
#include <rapidjson/document.h>
#include <string>
#include <vector>
#include <vstgui/lib/vstguibase.h>
#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>
#include <vstgui/uidescription/iuidescription.h>
#include <vstgui/vstgui.h>
#include <vstgui/vstgui_uidescription.h>

#include "cchordselecter.h"
#include "cfretboard.h"
#include "cmenubutton.h"
#include "files.h"
#include "myparameters.h"

namespace MinMax
{
    // フレットボードビュー
    class CFretBoardView
        : public VSTGUI::CViewContainer
    {
    private:

    public:
        CFretBoardView(const VSTGUI::UIAttributes& attributes, const VSTGUI::IUIDescription* description, const VSTGUI::CRect& size)
            : CViewContainer(size)
        {
            editor = dynamic_cast<VSTGUI::VST3Editor*>(description->getController());
            assert(editor && "CFretBoardView requires VST3Editor");

            setBackgroundColor(VSTGUI::kTransparentCColor);

            presetFileName = ChordMap::Instance().getPresetPath().u8string();

            // --- FretBoard ---
            fretBoard = new CFretBoard(getViewSize());
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

        std::string presetFileName;

        StringSet originalPressedFrets;

        CFretBoard* fretBoard = nullptr;

        CChordSelecter* chordSelecter = nullptr;

        CMenuButton* fileButton = nullptr;
        void popupFileMenu(VSTGUI::CFrame* frame, const VSTGUI::CView* anchor)
        {
            auto* menu = new VSTGUI::COptionMenu();

            auto* openMenu = createOpenPresetMenu();
            auto* openItem = new VSTGUI::CMenuItem("Open", openMenu);

            menu->addEntry(openItem);
            openMenu->forget();

            addCommand(menu, "Save", [this](VSTGUI::CCommandMenuItem*) { saveChordMap(); });

            VSTGUI::CRect r = anchor->getViewSize();
            VSTGUI::CPoint p(r.left, r.bottom);
            anchor->localToFrame(p);

            menu->popup(frame, p);
            menu->forget();
        }

        CMenuButton* editButton = nullptr;
        void popupEditMenu(VSTGUI::CFrame* frame, const VSTGUI::CView* anchor)
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

        VSTGUI::COptionMenu* createOpenPresetMenu()
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

                auto* item = new VSTGUI::CCommandMenuItem(title);

                item->setActions(
                    [this, fullpath](VSTGUI::CCommandMenuItem*)
                    {
                        ChordMap::Instance().initFromPreset(fullpath);
                        presetFileName = fullpath;
                        fretBoard->valueChanged();
                    }
                );
                menu->addEntry(item);
            }

            return menu;
        }

        // コードボイシングの保存
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

        // コードプリセットの保存
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

        // ファイルダイアログ表示
        void showDialog(VSTGUI::CControl* pControl, VSTGUI::CNewFileSelector::Style style, std::function<void(const std::string& path)> fileSelected)
        {
            Files::createPresetDirectory();

            auto* selector = VSTGUI::CNewFileSelector::create(pControl->getFrame(), style);
            if (selector == nullptr) return;

            std::string defaultName = presetFileName.empty() ? "NewPreset.json" : std::filesystem::path(presetFileName).filename().u8string();

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

        // メニュー要素追加
        template<typename F>
        static void addCommand(VSTGUI::COptionMenu* menu, const VSTGUI::UTF8String& title, F&& cb)
        {
            auto* item = new VSTGUI::CCommandMenuItem(VSTGUI::CCommandMenuItem::Desc(title));
            item->setActions(std::forward<F>(cb));
            menu->addEntry(item);
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