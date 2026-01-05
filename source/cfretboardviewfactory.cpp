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

    // プリセットファイル操作
    const struct Files 
    {
        /// 定数
        inline static const char* STR_USERPROFILE = "USERPROFILE";
        inline static const char* PRESET_ROOT = "Documents/VST3 Presets/MinMax/QBStrum";

        inline static const std::string TITLE = "QBStrum";
        inline static const std::string FILTER = "Chord Preset(.json)";
        inline static const std::string FILE_EXT = "json";

        // プリセットパスを取得する
        inline static std::filesystem::path getPresetPath()
        {
            const char* home = getenv(STR_USERPROFILE);
            if (!home)
                return std::filesystem::current_path();

            return std::filesystem::path(home).append(PRESET_ROOT).make_preferred();
        }

        // プリセットディレクトリを作成する
        inline static void createPresetDirectory()
        {
            auto p = getPresetPath();
            if (!std::filesystem::exists(p))
            {
                std::filesystem::create_directories(p);
            }
        }

        // プリセットファイルのパス一覧を取得する 
        inline static tresult getPresetFiles(std::vector<std::string>& file_names)
        {
            std::filesystem::directory_iterator iter(getPresetPath()), end;
            std::error_code err;

            for (; iter != end && !err; iter.increment(err))
            {
                const std::filesystem::directory_entry entry = *iter;
                if (std::filesystem::path(entry.path().string()).extension() != ".json") continue;
                file_names.push_back(entry.path().string());
            }

            return kResultTrue;
        }
    };

    //
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

        // ギターフレット表示コントロール
        class CFretBoard
            : public VSTGUI::CControl
        {
        private:
            // 設定
            const int lastFret = 19;                            // 最大フレット数
            const int firstFret = -1;                           // -1フレット（ナット外側）
            const int numFrets = (lastFret - firstFret + 1);
            const double outerMargin = 10.0;                    // 上部余白

            VSTGUI::CColor bg;
            VSTGUI::CColor stringColor;
            VSTGUI::CColor fretColor;
            VSTGUI::CColor nutColor;
            VSTGUI::CColor markerColor;
            VSTGUI::CColor fretNumberColor;
            VSTGUI::CColor pressedColor;

            VSTGUI::CRect frameSize;
            VSTGUI::CRect boardSize;

            double usableHeight;
            double stringSpacing;
            double fretSpacing;

            // 押さえているフレット
            StringSet pressedFrets;

            //
            bool editing = false;

            bool isMarkerFret(int f)
            {
                return (
                    f == 2 ||
                    f == 4 ||
                    f == 6 ||
                    f == 8 ||
                    f == 11 ||
                    f == 14 ||
                    f == 16 ||
                    f == 18
                    );
            }

        public:
            CFretBoard(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, ParamID tag)
                : CControl(size)
            {
                using namespace VSTGUI;

                setListener(listener);
                setTag(tag);

                // 初期値設定
                frameSize = size;
                boardSize = VSTGUI::CRect(frameSize.left + 65, frameSize.top + 20, frameSize.right, frameSize.bottom - 15);
                usableHeight = boardSize.getHeight() - outerMargin * 2;
                stringSpacing = usableHeight / (STRING_COUNT - 1);
                fretSpacing = boardSize.getWidth() / numFrets;

                bg = CColor(60, 40, 20, 255);
                stringColor = CColor(230, 230, 230, 255);
                fretColor = CColor(180, 180, 180, 255);
                nutColor = CColor(255, 255, 255, 255);
                markerColor = CColor(200, 200, 200, 255);
                fretNumberColor = CColor(220, 220, 220, 255);
                pressedColor = CColor(255, 140, 0, 255);
            }

            void draw(VSTGUI::CDrawContext* pContext) override
            {
                using namespace VSTGUI;

                // 背景
                pContext->setFillColor(bg);
                pContext->drawRect(boardSize, kDrawFilled);

                // ------------------------
                // 弦の描画（上下10px以内に収める）
                // ------------------------
                pContext->setFrameColor(stringColor);
                pContext->setLineWidth(2.0);
                for (int i = 0; i < STRING_COUNT; ++i)
                {
                    double y = boardSize.top + outerMargin + stringSpacing * i;
                    pContext->drawLine(CPoint(boardSize.left, y), CPoint(boardSize.right, y));
                }

                // ------------------------
                // フレット（–1 → ）
                // ------------------------
                for (int fret = firstFret; fret <= lastFret; ++fret)
                {
                    double x = boardSize.left + fretSpacing * (fret - firstFret);

                    if (fret == 0)
                    {
                        // ナット
                        pContext->setFrameColor(nutColor);
                        pContext->setLineWidth(5.0);
                        pContext->drawLine(CPoint(x - 3, boardSize.top), CPoint(x - 3, boardSize.bottom));
                        continue;
                    }

                    // 通常フレット
                    pContext->setFrameColor(fretColor);
                    pContext->setLineWidth(2.0);

                    pContext->drawLine(CPoint(x, boardSize.top), CPoint(x, boardSize.bottom));
                }

                pContext->setFillColor(markerColor);

                for (int fret = 1; fret <= lastFret; ++fret)
                {
                    if (!isMarkerFret(fret)) continue;

                    double x = boardSize.left + fretSpacing * (fret - firstFret + 0.5);

                    if (fret == 11 || fret == 22)
                    {
                        double y1 = boardSize.top + boardSize.getHeight() * 0.33;
                        double y2 = boardSize.top + boardSize.getHeight() * 0.66;

                        pContext->drawEllipse(CRect(x - 6, y1 - 6, x + 6, y1 + 6), kDrawFilled);
                        pContext->drawEllipse(CRect(x - 6, y2 - 6, x + 6, y2 + 6), kDrawFilled);
                    }
                    else
                    {
                        double y = boardSize.top + boardSize.getHeight() * 0.5;
                        pContext->drawEllipse(CRect(x - 6, y - 6, x + 6, y + 6), kDrawFilled);
                    }
                }

                // ------------------------
                // フレット番号（0〜  ）
                // ------------------------
                pContext->setFontColor(fretNumberColor);

                // フォント（VSTGUI既定）
                auto font = kNormalFontVerySmall; // or kNormalFontSmall
                pContext->setFont(font, 10);

                // 表示Y位置（指板の下）
                double numberY = frameSize.bottom - 8; // 下端から少し上

                for (int fret = 0; fret <= lastFret; ++fret)
                {
                    // フレット中央の X 座標
                    double x =
                        boardSize.left
                        + fretSpacing * fret //(fret - firstFret)
                        + fretSpacing * 0.5;

                    // 文字列化
                    Steinberg::String text;
                    text.printf("%d", fret);

                    // 文字の矩形（中央揃え）
                    CRect textRect(x - 10, numberY - 5, x + 10, numberY + 2);

                    pContext->drawString(text, textRect, kCenterText, true);
                }

                // ------------------------
                // 押さえているフレットのマーカー表示（🔶）
                // ------------------------
                {
                    pContext->setFillColor(pressedColor);
                    pContext->setFrameColor(pressedColor);

                    for (unsigned int stringIndex = 0; stringIndex < pressedFrets.size; ++stringIndex)
                    {
                        int fret = pressedFrets.data[stringIndex];

                        double y = boardSize.top + outerMargin + stringSpacing * stringIndex;

                        // --- (1) ミュート (-1) は X を描画 ---
                        if (fret == -1)
                        {
                            // ナットの上に配置（画面内）
                            double x = boardSize.left + fretSpacing * 0.5;  // ナットの少し右に置く
                            const double s = 6.0;

                            pContext->setFrameColor(CColor(255, 0, 0, 255)); // 赤色など
                            pContext->drawLine(CPoint(x - s, y - s), CPoint(x + s, y + s));
                            pContext->drawLine(CPoint(x - s, y + s), CPoint(x + s, y - s));
                            continue;
                        }

                        // --- (2) 開放弦 (0) は何も描かない ---
                        if (fret == 0)
                        {
                            continue;
                        }

                        // --- (3) 通常の押弦は (fret-1) を内部計算に使う ---
                        int internalFret = fret - 1;

                        if (internalFret < firstFret || internalFret > lastFret)
                        {
                            continue;
                        }

                        double x =
                            boardSize.left
                            + fretSpacing * (internalFret - firstFret)
                            + fretSpacing * 0.5;

                        const double s = 8.0;

                        std::vector<CPoint> pts;

                        pts.emplace_back(x, y - s);
                        pts.emplace_back(x + s, y);
                        pts.emplace_back(x, y + s);
                        pts.emplace_back(x - s, y);

                        pContext->drawPolygon(pts, VSTGUI::kDrawFilled);
                    }
                }
            }

            void valueChanged() override
            {
                auto value = getValue();
                pressedFrets = ChordMap::Instance().getVoicing((int)value);
                invalid();
            }

            VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState&) override
            {
                if (!editing) return VSTGUI::kMouseEventNotHandled;

                int stringIndex =
                    int((where.y - (boardSize.top + outerMargin)) / stringSpacing + 0.5);

                if (stringIndex < 0 || stringIndex >= STRING_COUNT)
                    return VSTGUI::kMouseEventNotHandled;

                int fret = int((where.x - boardSize.left) / fretSpacing) - 1;

                if (fret < firstFret || fret > lastFret - 1)
                    return VSTGUI::kMouseEventNotHandled;

                int& current = pressedFrets.data[stringIndex];

                // ---- ナット左 ----
                if (fret < 0)
                {
                    current = (current == -1) ? 0 : -1;
                }
                else
                {
                    int newValue = fret + 1;

                    if (current == newValue)
                    {
                        // 押弦 → 開放 → ミュート → 押弦
                        if (current > 0)
                            current = 0;
                        else if (current == 0)
                            current = -1;
                        else
                            current = newValue;
                    }
                    else
                    {
                        // 別フレットをクリック
                        current = newValue;
                    }
                }

                invalid();
                return VSTGUI::kMouseEventHandled;
            }

            void setEditing(bool state)
            {
                editing = state;
                invalid(); // 再描画
            }

            // 現在の押弦情報をコピーして返す
            StringSet getPressedFrets() const
            {
                return pressedFrets; // コピーされる
            }

            // 押弦情報を上書きして再描画
            void setPressedFrets(const StringSet& newFrets)
            {
                pressedFrets = newFrets;
                invalid(); // 再描画
            }

            CLASS_METHODS(CFretBoard, CControl)
        };

        // メニュー表示
        class MenuButton 
            : public VSTGUI::CTextButton
        {
        public:
            using Callback = std::function<void(MenuButton*)>;

            MenuButton(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag, const VSTGUI::UTF8String& title, Callback cb)
                : VSTGUI::CTextButton(size, listener, tag, title)
                , callback(cb)
            {
                setFrameColor(VSTGUI::kTransparentCColor);
                setGradient(nullptr);
                setTextColor(VSTGUI::kWhiteCColor);
                setFont(VSTGUI::kNormalFontSmall);
            }

            VSTGUI::CMouseEventResult onMouseEntered(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons ) override
            {
                mouseInside = true;
                invalid();
                return VSTGUI::kMouseEventHandled;
            }

            VSTGUI::CMouseEventResult onMouseExited(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override
            {
                mouseInside = false;
                invalid();
                return VSTGUI::kMouseEventHandled;
            }

            void draw(VSTGUI::CDrawContext* ctx) override
            {
                // hover 背景（Windows 風）
                if (mouseInside)
                {
                    ctx->setFillColor(VSTGUI::kGreyCColor);
                    ctx->drawRect(getViewSize(), VSTGUI::kDrawFilled);
                }

                // テキスト描画（親クラス任せ）
                VSTGUI::CTextButton::draw(ctx);
            }

            void valueChanged() override
            {
                if (getValue()) return;
                if (callback) callback(this);
            }

        private:
            bool mouseInside{ false };
            Callback callback;
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
            fileButton = new MenuButton(VSTGUI::CRect(1, 1, 60, 18), editor, -1, "Preset", [this](MenuButton* b) { popupFileMenu(getFrame(), b); });
            addView(fileButton);

            // --- Edit Menu ---
            editButton = new MenuButton(VSTGUI::CRect(62, 1, 122, 18), editor, -1, "Edit", [this](MenuButton* b) { popupEditMenu(getFrame(), b); });
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

        MenuButton* fileButton = nullptr;

        MenuButton* editButton = nullptr;

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