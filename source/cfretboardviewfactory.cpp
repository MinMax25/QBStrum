//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------

#include <cassert>
#include <filesystem>
#include <functional>
#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/vst/vsttypes.h>
#include <string>
#include <vector>
#include <vstgui/lib/ccolor.h>
#include <vstgui/lib/cfileselector.h>
#include <vstgui/lib/cfont.h>
#include <vstgui/lib/controls/ccontrol.h>
#include <vstgui/lib/controls/coptionmenu.h>
#include <vstgui/lib/controls/ctextlabel.h>
#include <vstgui/lib/cpoint.h>
#include <vstgui/lib/crect.h>
#include <vstgui/lib/cstring.h>
#include <vstgui/lib/cview.h>
#include <vstgui/lib/cviewcontainer.h>
#include <vstgui/lib/vstguibase.h>
#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>
#include <vstgui/uidescription/iuidescription.h>
#include <vstgui/uidescription/iviewcreator.h>
#include <vstgui/uidescription/uiattributes.h>
#include <vstgui/uidescription/uiviewfactory.h>
#include <base/source/fobject.h>

#include "cchordlistener.h"
#include "cchordselecter.h"
#include "cfretboard.h"
#include "chordmap.h"
#include "cmenubutton.h"
#include "files.h"
#include "myparameters.h"

#include "plugcontroller.h"

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

            initFretBoard();
            initMenuButtons();
            initLabelPreset();
            initChordSelecter();

            initChordListener();
        }

        ~CFretBoardView() override
        {
            if (chordListner)
                chordListner->forget();
        }

        CLASS_METHODS(CFretBoardView, CViewContainer)

    protected:

        enum class MenuType 
        { 
            File, 
            Edit
        };

        bool canEdit = false;

        VSTGUI::VST3Editor* editor = nullptr;

        CFretBoard* fretBoard = nullptr;

        CChordSelecter* chordSelecter = nullptr;

        CChordListner* chordListner = nullptr;

        CMenuButton* fileButton = nullptr;

        CMenuButton* editButton = nullptr;

        VSTGUI::CTextLabel* labelPreset = nullptr;

        void initFretBoard()
        {
            fretBoard = new CFretBoard(getViewSize());
            fretBoard->setPressedFrets(StringSet());
            addView(fretBoard);
        }

        void initMenuButtons()
        {
            fileButton = new CMenuButton({ 101, 1, 160, 18 }, "Preset", [this](CMenuButton* b) { popupMenu(b, MenuType::File); });
            addView(fileButton);

            editButton = new CMenuButton({ 161, 1, 220, 18 }, "Edit", [this](CMenuButton* b) { popupMenu(b, MenuType::Edit); });
            addView(editButton);
        }

        void initLabelPreset()
        {
            labelPreset = new VSTGUI::CTextLabel({ 221, 1, 400, 18 });
            labelPreset->setBackColor(VSTGUI::kGreyCColor);
            labelPreset->setFont(VSTGUI::kNormalFontSmall);
            labelPreset->setText(ChordMap::instance().getPresetName());
            addView(labelPreset);
        }

        void initChordSelecter()
        {
            chordSelecter = new CChordSelecter({ 651,1,800,19 }, [this](CChordSelecter*, int v) { onSelectedChordChanged(v); });
            addView(chordSelecter);
        }

        void initChordListener()
        {
            if (!editor) return;
            chordListner =
                new CChordListner(
                    editor, PARAM::CHORD_STATE_REVISION,
                    [this](CChordListner*, int v)
                    {
                        onParameterChordChanged(v);
                    }
                );
        }

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
                auto name = std::filesystem::path(f).stem().u8string();
                auto* item = new VSTGUI::CCommandMenuItem(VSTGUI::CCommandMenuItem::Desc(name.c_str()));

                item->setActions(
                    [this, f](VSTGUI::CCommandMenuItem*)
                    {
                        try
                        {
                            auto& cm = ChordMap::instance();
                            std::filesystem::path p(f);
                            cm.loadChordPreset(p);
                            auto cn = chordSelecter->getCurrentChordNumber();
                            auto& pf = cm.getChordVoicing(cn);
                            fretBoard->setPressedFrets(pf);
                            labelPreset->setText(cm.getPresetName());
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

        void onParameterChordChanged(int value)
        {
            if (!editor) return;
            if (canEdit) return;
            auto* c = Steinberg::FCast<MyVSTController>(editor->getController());
            fretBoard->setPressedFrets(c->ChordInfo);
            chordSelecter->setChordNumber(c->ChordInfo.flatIndex);
        }

        void onSelectedChordChanged(int value)
        {
            if (!editor) return;
            if (canEdit) return;

            auto* c = editor->getController();

            for (int i = 0; i < MAX_STRINGS; i++)
            {
                Steinberg::Vst::ParamID tag = PARAM::STR1_OFFSET + i;
                c->beginEdit(tag);
                Steinberg::Vst::ParamValue norm = c->plainParamToNormalized(tag, StringSet::CENTER_OFFSET);
                c->setParamNormalized(tag, norm);
                c->performEdit(tag, norm);
                c->endEdit(tag);
            }

            {
                c->beginEdit(PARAM::CHORD_NUM);
                Steinberg::Vst::ParamValue norm = c->plainParamToNormalized(PARAM::CHORD_NUM, value);
                c->setParamNormalized(PARAM::CHORD_NUM, norm);
                c->performEdit(PARAM::CHORD_NUM, norm);
                c->endEdit(PARAM::CHORD_NUM);
            }
        }

        void saveChordMap()
        {
            if (!fileButton) return;

            showDialog(
                fileButton,
                VSTGUI::CNewFileSelector::kSelectSaveFile,
                [this](const std::filesystem::path path)
                {
                    try
                    {
                        ChordMap::instance().saveChordPreset(path);
                        labelPreset->setText(ChordMap::instance().getPresetName());
                    }
                    catch (...)
                    {
                        showError("Failed to save preset.");
                    }
                }
            );
        }

        void enterEditMode()
        {
            canEdit = true;
            fretBoard->beginEdit();
            chordSelecter->beginEdit();
        }

        void commitEdits()
        {
            auto currentSet = fretBoard->getPressedFrets();
            int chordNum = chordSelecter->getCurrentChordNumber();
            ChordMap::instance().setVoicing(chordNum, currentSet);

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
            fretBoard->endEdit(isCancel);
            chordSelecter->endEdit();
        }

        void showDialog(VSTGUI::CControl* p, VSTGUI::CNewFileSelector::Style style, std::function<void(std::filesystem::path path)> fileSelected)
        {
            Files::createPluginDirectory();

            auto* selector = VSTGUI::CNewFileSelector::create(p->getFrame(), style);
            if (!selector) return;

            selector->setTitle(Files::TITLE);
            selector->setInitialDirectory(Files::getPresetPath().u8string());
            selector->setDefaultSaveName(ChordMap::instance().getPresetPath().filename().u8string());
            selector->addFileExtension(VSTGUI::CFileExtension(Files::FILTER, Files::FILE_EXT));

            p->getFrame()->setFocusView(nullptr);
            if (selector->runModal() && (int)selector->getNumSelectedFiles() == 1)
            {
                auto p = ChordMap::convertUtf8ToUtf16(selector->getSelectedFile(0));
                auto path = std::filesystem::path(p);
                fileSelected(path);
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
