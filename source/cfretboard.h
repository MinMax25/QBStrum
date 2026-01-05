//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <base/source/fstring.h>
#include <public.sdk/source/vst/vstguieditor.h>
#include <vector>
#include <vstgui/lib/vstguibase.h>
#include <vstgui/vstgui.h>

#include "myparameters.h"

namespace MinMax
{

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
        CFretBoard(const VSTGUI::CRect& size)
            : CControl(size)
        {
            using namespace VSTGUI;

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

        // 現在の押弦情報
        StringSet getPressedFrets() const
        {
            return pressedFrets;
        }

        // 押弦情報を上書きして再描画
        void setPressedFrets(const StringSet& newFrets)
        {
            pressedFrets = newFrets;
            invalid(); // 再描画
        }

        CLASS_METHODS(CFretBoard, CControl)
    };
}