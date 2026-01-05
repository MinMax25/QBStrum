#pragma once

#include <base/source/fstring.h>
#include <filesystem>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <public.sdk/source/vst/vstguieditor.h>
#include <vstgui/lib/vstguibase.h>
#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/uidescription/iuidescription.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>
#include <vstgui/vstgui.h>
#include <vstgui/vstgui_uidescription.h>

#include "cchordselecter.h"
#include "cfretboard.h"
#include "cmenubutton.h"
#include "files.h"
#include "myparameters.h"

namespace MinMax
{
    class CFretBoardView : public VSTGUI::CViewContainer
    {
    public:
        CFretBoardView(const VSTGUI::UIAttributes& attributes,
            const VSTGUI::IUIDescription* description,
            const VSTGUI::CRect& size)
            : CViewContainer(size)
        {
            editor = dynamic_cast<VSTGUI::VST3Editor*>(description->getController());
            assert(editor && "CFretBoardView requires VST3Editor");

            setBackgroundColor(VSTGUI::kTransparentCColor);

            presetFileName = ChordMap::Instance().getPresetPath().u8string();

            initFretBoard();
            initChordSelecter();
            initMenuButtons();
        }

        ~CFretBoardView() override = default;

        CLASS_METHODS(CFretBoardView, CViewContainer)

    protected:
        VSTGUI::VST3Editor* editor = nullptr;
        std::string presetFileName;
        StringSet originalPressedFrets;

        CFretBoard* fretBoard = nullptr;
        CChordSelecter* chordSelecter = nullptr;
        CMenuButton* fileButton = nullptr;
        CMenuButton* editButton = nullptr;

        // ---- UI 初期化 ----
        void initFretBoard()
        {
            fretBoard = new CFretBoard(getViewSize());
            fretBoard->setPressedFrets(getVoicing(0));
            addView(fretBoard);
        }

        void initChordSelecter()
        {
            constexpr VSTGUI::CRect rect(365, 1, 515, 19);
            chordSelecter = new CChordSelecter(rect, [this](CChordSelecter* /*p*/, int value)
                {
                    fretBoard->setPressedFrets(getVoicing(value));
                });
            addView(chordSelecter);
        }

        void initMenuButtons()
        {
            constexpr VSTGUI::CRect fileRect(1, 1, 60, 18);
            constexpr VSTGUI::CRect editRect(62, 1, 122, 18);

            fileButton = new CMenuButton(fileRect, "Preset", [this](CMenuButton* b)
                {
                    popupMenu(b, MenuType::File);
                });
            addView(fileButton);

            editButton = new CMenuButton(editRect, "Edit", [this](CMenuButton* b)
                {
                    popupMenu(b, MenuType::Edit);
                });
            addView(editButton);
        }

        enum class MenuType { File, Edit };

        void popupMenu(VSTGUI::CView* anchor, MenuType type)
        {
            auto* menu = new VSTGUI::COptionMenu();

            if (type == MenuType::File)
            {
                auto* openMenu = createOpenPresetMenu();
                menu->addEntry(new VSTGUI::CMenuItem("Open", openMenu));
                openMenu->forget();

                addMenuCommand(menu, "Save", [this](VSTGUI::CCommandMenuItem*) { saveChordMap(); });
            }
            else
            {
                addMenuCommand(menu, "Edit", [](VSTGUI::CCommandMenuItem*) { /* 編集処理 */ });
                addMenuCommand(menu, "Commit Changes", [](VSTGUI::CCommandMenuItem*) { /* 確定処理 */ });
                addMenuCommand(menu, "Cancel Changes", [](VSTGUI::CCommandMenuItem*) { /* キャンセル処理 */ });
            }

            VSTGUI::CRect r = anchor->getViewSize();
            VSTGUI::CPoint p(r.left, r.bottom);
            anchor->localToFrame(p);

            menu->popup(getFrame(), p);
            menu->forget();
        }

        VSTGUI::COptionMenu* createOpenPresetMenu()
        {
            auto* menu = new VSTGUI::COptionMenu();

            std::vector<std::string> files;
            if (Files::getPresetFiles(files) != kResultTrue || files.empty())
            {
                auto* item = new VSTGUI::CCommandMenuItem(
                    VSTGUI::CCommandMenuItem::Desc("No Presets")
                );
                item->setEnabled(false);
                menu->addEntry(item);
                return menu;
            }

            for (const auto& fullpath : files)
            {
                auto filename = std::filesystem::path(fullpath).filename().u8string();
                auto* item = new VSTGUI::CCommandMenuItem(
                    VSTGUI::CCommandMenuItem::Desc(filename.c_str())
                );

                item->setActions([this, fullpath](VSTGUI::CCommandMenuItem*)
                    {
                        try
                        {
                            ChordMap::Instance().initFromPreset(fullpath);
                            presetFileName = fullpath;
                            fretBoard->valueChanged();
                        }
                        catch (...)
                        {
                            showError("Failed to load preset.");
                        }
                    });

                menu->addEntry(item);
            }

            return menu;
        }

        StringSet getVoicing(int value) const
        {
            return ChordMap::Instance().getChordVoicing(value);
        }

        void saveChordMap()
        {
            if (!fileButton) return;

            showDialog(fileButton, VSTGUI::CNewFileSelector::kSelectSaveFile,
                [this](const std::string& path)
                {
                    try
                    {
                        ChordMap::Instance().saveToFile(path);
                        presetFileName = path;
                    }
                    catch (...)
                    {
                        showError("Failed to save preset.");
                    }
                });
        }

        void showDialog(VSTGUI::CControl* pControl,
            VSTGUI::CNewFileSelector::Style style,
            std::function<void(const std::string& path)> fileSelected)
        {
            Files::createPresetDirectory();

            auto* selector = VSTGUI::CNewFileSelector::create(pControl->getFrame(), style);
            if (!selector) return;

            std::string defaultName = presetFileName.empty() ? "NewPreset.json" :
                std::filesystem::path(presetFileName).filename().u8string();

            if (!presetFileName.empty())
            {
                selector->setInitialDirectory(std::filesystem::path(presetFileName).parent_path().string().c_str());
            }

            selector->setTitle(Files::TITLE.c_str());
            selector->setDefaultSaveName(defaultName.c_str());
            selector->addFileExtension(VSTGUI::CFileExtension(Files::FILTER.c_str(), Files::FILE_EXT.c_str()));

            pControl->getFrame()->setFocusView(nullptr);

            if (selector->runModal() && selector->getNumSelectedFiles() == 1)
            {
                presetFileName = selector->getSelectedFile(0);
                fileSelected(presetFileName);
            }

            selector->forget();
        }

        void showError(const std::string& msg)
        {
            // TODO: 実際のUIエラー表示に置き換え
            fprintf(stderr, "Error: %s\n", msg.c_str());
        }

        template<typename F>
        static void addMenuCommand(VSTGUI::COptionMenu* menu, const VSTGUI::UTF8String& title, F&& cb)
        {
            auto* item = new VSTGUI::CCommandMenuItem(VSTGUI::CCommandMenuItem::Desc(title));
            item->setActions(std::forward<F>(cb));
            menu->addEntry(item);
        }
    };

    class CFretBoardViewFactory : public VSTGUI::ViewCreatorAdapter
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
