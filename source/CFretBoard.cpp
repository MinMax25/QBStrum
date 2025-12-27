//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------

#include "plugdefine.h"
#include "cfretboard.h"
#include "chordmap.h"
#include "myparameters.h"

namespace MinMax
{
	CFretBoard::CFretBoard(const CRect& size, EditControllerEx1* controller)
		: CControl(size)
	{
    }

	void CFretBoard::draw(CDrawContext* pContext)
	{
        const CRect& r = getViewSize();

        const CRect& boardSize = CRect(r.left, r.top, r.right, r.bottom - 20);

        // 背景
        CColor bg(60, 40, 20, 255);
        pContext->setFillColor(bg);
        pContext->drawRect(boardSize, kDrawFilled);

        // 設定
        const int lastFret = 30;    // 最大フレット数
        const int firstFret = -1;   // -1フレット（ナット外側）
        const int numFrets = (lastFret - firstFret + 1);

        CColor stringColor(230, 230, 230, 255);
        CColor fretColor(180, 180, 180, 255);
        CColor nutColor(255, 255, 255, 255);
        CColor markerColor(200, 200, 200, 255);

        // ←─★ 上下余白を 10px に設定
        double outerMargin = 10.0;

        double usableHeight = boardSize.getHeight() - outerMargin * 2.0;
        double stringSpacing = usableHeight / (numStrings - 1);
        double fretSpacing = boardSize.getWidth() / numFrets;

        // ------------------------
        // 弦の描画（上下10px以内に収める）
        // ------------------------
        pContext->setFrameColor(stringColor);
        pContext->setLineWidth(2.0);

        for (int i = 0; i < numStrings; ++i)
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
                // ナット（二重線）
                pContext->setFrameColor(nutColor);

                pContext->setLineWidth(2.0);
                pContext->drawLine(CPoint(x, boardSize.top), CPoint(x, boardSize.bottom));

                pContext->setLineWidth(2.0);
                pContext->drawLine(CPoint(x + 3, boardSize.top), CPoint(x + 3, boardSize.bottom));
                continue;
            }

            // 通常フレット
            pContext->setFrameColor(fretColor);
            pContext->setLineWidth(2.0);

            pContext->drawLine(CPoint(x, boardSize.top), CPoint(x, boardSize.bottom));
        }

        // ------------------------
        // ポジションマーカー（3,5,7,9,12,15,17,19,21,23）
        // ------------------------
        auto isMarkerFret =
            [](int f)
            {
                return
                    (
                        f == 2 ||
                        f == 4 ||
                        f == 6 ||
                        f == 8 ||
                        f == 11 ||
                        f == 14 ||
                        f == 16 ||
                        f == 18 ||
                        f == 20 ||
                        f == 22);
            };

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
        CColor fretNumberColor(220, 220, 220, 255);
        pContext->setFontColor(fretNumberColor);

        // フォント（VSTGUI既定）
        auto font = kNormalFontVerySmall; // or kNormalFontSmall
        pContext->setFont(font, 10);

        // 表示Y位置（指板の下）
        double numberY = r.bottom - 8; // 下端から少し上

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
            CColor pressedColor(255, 140, 0, 255);
            pContext->setFillColor(pressedColor);
            pContext->setFrameColor(pressedColor);

            for (unsigned int stringIndex = 0; stringIndex < pressedFrets.size(); ++stringIndex)
            {
                int fret = pressedFrets[stringIndex];

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

        setDirty(false);
    }

    void CFretBoard::setValue(ParamID tag, int idx)
    {
        if (tag == static_cast<int32>(PARAM::CHORD_ROOT))
        {
            chord.root = idx;
        }
        else if (tag == static_cast<int32>(PARAM::CHORD_TYPE))
        {
            chord.type = idx;
        }
        else if (tag == static_cast<int32>(PARAM::FRET_POSITION))
        {
            chord.position = idx;
        }

        pressedFrets = ChordMap::Instance().getFretPositions(chord);

        invalid();
    }
}
