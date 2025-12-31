//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------

#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>
#include <vstgui/uidescription/iuidescription.h>
#include <vstgui/lib/vstguibase.h>
#include <vstgui/vstgui_uidescription.h>

#include "myparameters.h"

namespace MinMax
{
    class CEditModeButton
        : public VSTGUI::CTextButton
    {
    public:
        CEditModeButton(const VSTGUI::CRect& size, std::function<void(CControl*)> _func)
            : CTextButton(size)
            , func(_func)
        {
            setTag(9001);
            setTitle(u8"Edit");
        }

        void valueChanged() override
        {
            if (!getValue()) return;

            state = !state;
            setTitle(state ? u8"Save" : u8"Edit");
            func(this);
        }

        bool getState()
        {
            return state;
        }

    protected:
        std::function<void(CControl*)> func;
        bool state = false;
    };

    class CFretBoard
        : public VSTGUI::CControl
    {
        // 設定
        const int lastFret = 19;                            // 最大フレット数
        const int firstFret = -1;                           // -1フレット（ナット外側）
        const int numFrets = (lastFret - firstFret + 1);
        const double outerMargin = 10.0;                    // 上部余白

    public:
        CFretBoard(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, ParamID tag)
            : CControl(size)
        {
            using namespace VSTGUI;

            setListener(listener);
            setTag(tag);

            // 初期値設定
            frameSize = size;
            boardSize = VSTGUI::CRect(frameSize.left, frameSize.top + 20, frameSize.right, frameSize.bottom - 20);
            usableHeight = boardSize.getHeight() - outerMargin * 2.0;
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

            for (int fret = 0; fret <= 19; ++fret)
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
                CRect textRect(x - 10, numberY - 10, x + 10, numberY + 2);

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
            const VSTGUI::CRect& r = getViewSize();
            VSTGUI::CRect boardSize(r.left, r.top, r.right, r.bottom - 20);

            double outerMargin = 10.0;
            double usableHeight = boardSize.getHeight() - outerMargin * 2.0;
            double stringSpacing = usableHeight / (STRING_COUNT - 1);
            double fretSpacing = boardSize.getWidth() / (30 - (-1) + 1);

            int stringIndex =
                int((where.y - (boardSize.top + outerMargin)) / stringSpacing + 0.5);

            if (stringIndex < 0 || stringIndex >= STRING_COUNT)
                return VSTGUI::kMouseEventNotHandled;

            int fret = int((where.x - boardSize.left) / fretSpacing) - 1;

            if (fret < -1 || fret > 18)
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

    private:
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
        bool isEditing = false;

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

        CLASS_METHODS(CFretBoard, CControl)
    };

    class CFretBoardView
        : public VSTGUI::CViewContainer
    {
    public:
        CFretBoardView(const VSTGUI::UIAttributes& attributes, const VSTGUI::IUIDescription* description, const VSTGUI::CRect& size)
            : CViewContainer(size)
        {
            editor = dynamic_cast<VSTGUI::VST3Editor*>(description->getController());
            if (editor) editor->addRef();

            setBackgroundColor(VSTGUI::kGreyCColor);

            // --- FretBoard ---
            CFretBoard* fretBoard = new CFretBoard(getViewSize(), editor, PARAM::CHORD_NUM);
            addView(fretBoard);

            // --- Edit / Save Button ---
            editButton =
                new CEditModeButton(
                    VSTGUI::CRect(10, 1, 60, 18),
                    [this](VSTGUI::CControl* p)
                    {
                        valueChanged(p);
                    }
                );
            editButton->setFont(VSTGUI::kNormalFontSmall);
            addView(editButton);
        }

        ~CFretBoardView()
        {
            if (editor) editor->release();
        }

        void valueChanged(VSTGUI::CControl* pControl)
        {
            if (pControl->getTag() != 9001) return;
        }

        CLASS_METHODS(CFretBoardView, CViewContainer)

    private:

        VSTGUI::VST3Editor* editor = nullptr;;

        CEditModeButton* editButton = nullptr;
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
            return new CFretBoardView(attributes, description, VSTGUI::CRect(VSTGUI::CPoint(0, 0), VSTGUI::CPoint(730, 175)));
        }
    };

    CFretBoardViewFactory __gCFretBoardViewFactory;
}