#pragma once

#include <base/source/fstring.h>
#include <filesystem>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <public.sdk/source/vst/vstguieditor.h>
#include <vstgui/lib/vstguibase.h>
#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/uidescription/iuidescription.h>
#include <vstgui/vstgui.h>
#include <vstgui/vstgui_uidescription.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>

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
        CFretBoardView(const VSTGUI::UIAttributes& attr, const VSTGUI::IUIDescription* desc, const VSTGUI::CRect& size)
            : CViewContainer(size)
        {
            editor = dynamic_cast<VSTGUI::VST3Editor*>(desc->getController());
            assert(editor && "CFretBoardView requires VST3Editor");

            setBackgroundColor(VSTGUI::kTransparentCColor);

            presetPath = ChordMap::Instance().getPresetPath().u8string();

            initFretBoard();
            initChordSelecter();
            initMenuButtons();
        }

        ~CFretBoardView() override = default;

        CLASS_METHODS(CFretBoardView, CViewContainer)

    protected:
        VSTGUI::CColor NORMAL_TEXT_COLOR = VSTGUI::kWhiteCColor;
        VSTGUI::CColor EDIT_TEXT_COLOR = VSTGUI::kCyanCColor;

        bool canEdit = false;

        VSTGUI::VST3Editor* editor = nullptr;

        std::string presetPath;

        CFretBoard* fretBoard = nullptr;

        CChordSelecter* chordSelecter = nullptr;

        CMenuButton* fileButton = nullptr;

        CMenuButton* editButton = nullptr;

        void initFretBoard()
        {
            fretBoard = new CFretBoard(getViewSize());
            fretBoard->setPressedFrets(getVoicing(0));
            addView(fretBoard);
        }

        void initChordSelecter()
        {
            chordSelecter = new CChordSelecter({ 365,1,515,19 }, [this](CChordSelecter*, int v) { onChordNumberChanged(v); });
            addView(chordSelecter);
        }

        void initMenuButtons()
        {
            fileButton = new CMenuButton({ 1,1,60,18 }, "Preset", [this](CMenuButton* b) { popupMenu(b, MenuType::File); });
            addView(fileButton);

            editButton = new CMenuButton({ 62,1,122,18 }, "Edit", [this](CMenuButton* b) { popupMenu(b, MenuType::Edit); });
            addView(editButton);
        }

        void onChordNumberChanged(int value)
        {
            fretBoard->setPressedFrets(getVoicing(value));
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
                editButton->setTextColor(canEdit ? EDIT_TEXT_COLOR : NORMAL_TEXT_COLOR);

                if (canEdit)
                {
                    addMenuCommand(menu, "Commit Changes", [this](VSTGUI::CCommandMenuItem*) { commitEdits(); });
                    addMenuCommand(menu, "Cancel Changes", [this](VSTGUI::CCommandMenuItem*) { cancelEdits(); });
                }
                else
                {
                    addMenuCommand(menu, "Enter Edit Mode", [this](VSTGUI::CCommandMenuItem*) { enterEditMode(); });
                }
            }

            VSTGUI::CPoint p(anchor->getViewSize().left, anchor->getViewSize().bottom);
            anchor->localToFrame(p);
            menu->popup(getFrame(), p);
            menu->forget();
        }

        VSTGUI::COptionMenu* createOpenPresetMenu()
        {
            auto* menu = new VSTGUI::COptionMenu();
            
            std::vector<std::string> files;
            if (Files::getPresetFiles(files) != Steinberg::kResultTrue || files.empty())
            {
                auto* item = new VSTGUI::CCommandMenuItem(VSTGUI::CCommandMenuItem::Desc("No Presets"));
                item->setEnabled(false);
                menu->addEntry(item);
                return menu;
            }

            for (const auto& f : files)
            {
                auto name = std::filesystem::path(f).filename().u8string();
                auto* item = new VSTGUI::CCommandMenuItem(VSTGUI::CCommandMenuItem::Desc(name.c_str()));

                item->setActions(
                    [this, f](VSTGUI::CCommandMenuItem*)
                    {
                        try
                        {
                            ChordMap::Instance().initFromPreset(f); presetPath = f; fretBoard->valueChanged();
                        }
                        catch (...)
                        {
                            showError("Failed to load preset.");
                        }
                    }
                );
                menu->addEntry(item);
            }
            return menu;
        }
  
        void enterEditMode()
        {
            canEdit = true;
            editButton->setTextColor(EDIT_TEXT_COLOR);
            fretBoard->beginEdit();
            chordSelecter->beginEdit();
        }

        void commitEdits()
        {
            auto pressed = fretBoard->getPressedFrets();
            int chordNum = chordSelecter->getCurrentChordNumber();
            ChordMap::Instance().setVoicing(chordNum, pressed);

            exitEditMode(false);
        }

        void cancelEdits()
        {
            if (!canEdit) return;
            exitEditMode(true);
        }

        void exitEditMode(bool isCancel)
        {
            canEdit = false;
            editButton->setTextColor(NORMAL_TEXT_COLOR); 
            fretBoard->endEdit(isCancel);
            chordSelecter->endEdit();
        }

        StringSet getVoicing(int value) const 
        {
            return ChordMap::Instance().getChordVoicing(value);
        }

        void saveChordMap()
        {
            if (!fileButton) return;
            showDialog(
                fileButton,
                VSTGUI::CNewFileSelector::kSelectSaveFile,
                [this](const std::string& path)
                {
                    try { ChordMap::Instance().saveToFile(path); presetPath = path; }
                    catch (...) { showError("Failed to save preset."); }
                }
            );
        }

        void showDialog(VSTGUI::CControl* p, VSTGUI::CNewFileSelector::Style style, std::function<void(const std::string&)> fileSelected)
        {
            Files::createPresetDirectory();

            auto* selector = VSTGUI::CNewFileSelector::create(p->getFrame(), style);
            if (!selector) return;

            std::string defaultName = presetPath.empty() ? "NewPreset.json" : std::filesystem::path(presetPath).filename().u8string();
            if (!presetPath.empty())
                selector->setInitialDirectory(std::filesystem::path(presetPath).parent_path().string().c_str());

            selector->setTitle(Files::TITLE.c_str());
            selector->setDefaultSaveName(defaultName.c_str());
            selector->addFileExtension(VSTGUI::CFileExtension(Files::FILTER.c_str(), Files::FILE_EXT.c_str()));

            p->getFrame()->setFocusView(nullptr);

            if (selector->runModal() && selector->getNumSelectedFiles() == 1) 
            {
                presetPath = selector->getSelectedFile(0);
                fileSelected(presetPath);
            }
            selector->forget();
        }

        void showError(const std::string& msg)
        {
            //VSTGUI::CAlertBox::show("Error", msg.c_str(), getFrame()); // UI 内表示に変更
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
        CFretBoardViewFactory() 
        { 
            VSTGUI::UIViewFactory::registerViewCreator(*this);
        }

        VSTGUI::IdStringPtr getViewName() const override 
        { 
            return "UI:FretBoard View"; 
        }

        VSTGUI::IdStringPtr getBaseViewName() const override 
        {
            return VSTGUI::UIViewCreator::kCViewContainer; 
        }

        VSTGUI::CView* create(const VSTGUI::UIAttributes& attr, const VSTGUI::IUIDescription* desc) const override
        {
            return new CFretBoardView(attr, desc, { 0,0,795,170 });
        }
    };

    CFretBoardViewFactory __gCFretBoardViewFactory;
}
