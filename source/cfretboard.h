//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <base/source/fstring.h>
#include <vector>
#include <vstgui/lib/cbuttonstate.h>
#include <vstgui/lib/ccolor.h>
#include <vstgui/lib/cdrawcontext.h>
#include <vstgui/lib/cdrawdefs.h>
#include <vstgui/lib/cfont.h>
#include <vstgui/lib/controls/ccontrol.h>
#include <vstgui/lib/cpoint.h>
#include <vstgui/lib/crect.h>
#include <vstgui/lib/vstguibase.h>
#include <vstgui/lib/vstguifwd.h>

namespace MinMax
{
    enum class FretBoardContext
    {
        View,   // 通常時
        Edit,   // コード編集
        Guess   // コード推定
    };

    struct BarreSpan
    {
        int fret;
        int stringFrom; // inclusive
        int stringTo;   // inclusive
    };

    // ギターフレット表示コントロール
    class CFretBoard
        : public VSTGUI::CControl
    {
    public:
        using EditChordChanged = std::function<void(CFretBoard*, StringSet)>;

    private:
        // 設定
        static constexpr int lastFret = 19;                 // 最大フレット数
        const int firstFret = -1;                           // -1フレット（ナット外側）
        const int numFrets = (lastFret - firstFret + 1);
        const double outerMargin = 10.0;                    // 上部余白
        const float markerRadius = 8.0f;

        const VSTGUI::CColor bg = VSTGUI::CColor(55, 35, 18, 255);
        const VSTGUI::CColor stringColor = VSTGUI::CColor(230, 230, 230, 255);
        const VSTGUI::CColor fretColor = VSTGUI::CColor(180, 180, 180, 255);
        const VSTGUI::CColor nutColor = VSTGUI::CColor(255, 255, 255, 255);
        const VSTGUI::CColor markerColor = VSTGUI::CColor(200, 200, 200, 255);
        const VSTGUI::CColor fretNumberColor = VSTGUI::CColor(220, 220, 220, 255);
        const VSTGUI::CColor pressedColor = VSTGUI::CColor(255, 140, 0, 255);
        const VSTGUI::CColor pressedOffsetColor = VSTGUI::CColor(255, 0, 255, 255);
        const VSTGUI::CColor muteColor = VSTGUI::CColor(255, 0, 0, 255);
        const VSTGUI::CColor kBarreColor = VSTGUI::CColor(255, 140, 0, 80);

        VSTGUI::CRect frameSize;
        VSTGUI::CRect boardSize;

        double usableHeight;
        double stringSpacing;
        double fretSpacing;

        using BarreFlags = std::array<bool, STRING_COUNT>;
        using BarreInfoArray = std::array<BarreFlags, lastFret>;

        // 押さえているフレット
        StringSet currentSet{};

        StringSet workingSet{};

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

        void saveStringSet()
        {
            workingSet.size = currentSet.size;
            for (int i = 0; i < (int)currentSet.size; i++)
            {
                workingSet.data[i] = currentSet.data[i];
                workingSet.setOffset(i, currentSet.getOffset(i));
                currentSet.setOffset(i, 0);
            }
        }

        void restoreStringSet()
        {
            currentSet.size = workingSet.size;
            for (int i = 0; i < (int)workingSet.size; i++)
            {
                currentSet.data[i] = workingSet.data[i];
                currentSet.setOffset(i, workingSet.getOffset(i));
            }
        }

        FretBoardContext context{ FretBoardContext::View };

        BarreInfoArray buildBarreInfo(const StringSet& pressed)
        {
            BarreInfoArray info{};

            // --- 第1段：フレットごとの初期バレー判定 ---
            for (int fret = 0; fret < lastFret; ++fret)
            {
                bool barreSW = false;

                // どこかの弦がこの fret を押しているか
                for (int s = 0; s < STRING_COUNT; ++s)
                {
                    if (pressed.data[s] == fret)
                    {
                        barreSW = true;
                        break;
                    }
                }

                for (int s = 0; s < STRING_COUNT; ++s)
                {
                    // ミュート弦があれば以降不可
                    if (pressed.data[s] == -1)
                        barreSW = false;

                    info[fret][s] = barreSW;
                }
            }

            // --- 第2段：弦ごとの後処理（C# 版と同等） ---
            for (int s = 0; s < STRING_COUNT; ++s)
            {
                int minFret = -1;
                int count = 0;

                // 候補フレット探索
                for (int f = 0; f < lastFret; ++f)
                {
                    if (info[f][s])
                    {
                        if (minFret < 0)
                            minFret = f;
                        ++count;
                    }
                }

                if (count <= 1)
                    continue;

                // minFret より上で「実際に押さえていない」ものを除外
                for (int f = minFret + 1; f < lastFret; ++f)
                {
                    if (pressed.data[s] != f)
                        info[f][s] = false;
                }

                // 開放弦との矛盾除去
                if (info[0][s] && pressed.data[s] != 0)
                {
                    info[0][s] = false;
                }
            }

            return info;
        }

        std::vector<BarreSpan> extractBarreSpans(const BarreInfoArray& info)
        {
            std::vector<BarreSpan> result;

            for (int fret = 0; fret < lastFret; ++fret)
            {
                if (fret <= 0)
                    continue;

                const BarreFlags& flags = info[fret];

                int start = -1;

                for (int s = 0; s < STRING_COUNT; ++s)
                {
                    if (flags[s])
                    {
                        if (start < 0)
                            start = s;
                    }
                    else
                    {
                        if (start >= 0)
                        {
                            // 2弦以上（＝1フレットで複数弦）ならバレー
                            if (s - start >= 2)
                            {
                                result.push_back({ fret, start, s - 1 });
                            }
                            start = -1;
                        }
                    }
                }

                // 末尾まで続いていた場合
                if (start >= 0 && STRING_COUNT - start >= 2)
                {
                    result.push_back({ fret, start, STRING_COUNT - 1 });
                }
            }

            return result;
        }

        float GetFretX(int fret)
        {
            int internalFret = fret - 1;

            return
                static_cast<float>(
                    boardSize.left
                    + fretSpacing * (internalFret - firstFret)
                    + fretSpacing * 0.5
                    );
        }

        float GetStringY(int stringIndex)
        {
            return
                static_cast<float>(
                    boardSize.top
                    + outerMargin
                    + stringSpacing * stringIndex
                    );
        }

		EditChordChanged editChordChangedCallback;

    public:
        CFretBoard(const VSTGUI::CRect& size, EditChordChanged cb)
            : CControl(size)
			, editChordChangedCallback(cb)
        {
            using namespace VSTGUI;

            // 初期値設定
            frameSize = size;
            boardSize = VSTGUI::CRect(frameSize.left + 65, frameSize.top + 20, frameSize.right, frameSize.bottom - 15);
            usableHeight = boardSize.getHeight() - outerMargin * 2;
            stringSpacing = usableHeight / (STRING_COUNT - 1);
            fretSpacing = boardSize.getWidth() / numFrets;
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
                auto barreInfo = buildBarreInfo(currentSet);
                auto barreSpans = extractBarreSpans(barreInfo);

                DrawBarres(pContext, barreSpans, markerRadius);

                for (unsigned int stringindex = 0; stringindex < currentSet.size; ++stringindex)
                {
                    int fret = currentSet.data[stringindex];
                    int offset = currentSet.getOffset(stringindex);
                    bool hasOffset = offset != 0;

                    switch (offset + StringSet::CENTER_OFFSET)
                    {
                    case 0: // mute
						fret = -1;
                        hasOffset = false;
                        break;
                    case 1: // open
                        fret = 0;
                        hasOffset = false;
                        break;
                    default:
                        fret += offset;
                        break;
                    }

                    bool outOfRange = hasOffset && (fret < 0 || fret > lastFret);

                    double y = boardSize.top + outerMargin + stringSpacing * stringindex;

                    if (hasOffset)
                    {
                        pContext->setFillColor(pressedOffsetColor);
                        pContext->setFrameColor(pressedOffsetColor);
                    }
                    else
                    {
                        pContext->setFillColor(pressedColor);
                        pContext->setFrameColor(pressedColor);
                    }

                    // --- (1) ミュート (-1) は X を描画 ---
                    if (fret == -1 || outOfRange)
                    {
                        double x = boardSize.left + fretSpacing * 0.5;
                        const double s = 6.0;
                        if (!hasOffset)
                        {
                            pContext->setFrameColor(muteColor);
                        }
                        pContext->drawLine(CPoint(x - s, y - s), CPoint(x + s, y + s));
                        pContext->drawLine(CPoint(x - s, y + s), CPoint(x + s, y - s));
                        continue;
                    }

                    // --- (2) 開放弦 (0) は何も描かない ---
                    if (fret == 0 && !hasOffset)
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

        void DrawBarres(VSTGUI::CDrawContext* ctx, const std::vector<BarreSpan>& spans, float markerRadius)
        {
            if (!ctx)
                return;

            ctx->setFillColor(kBarreColor);

            for (const auto& span : spans)
            {
                float x = GetFretX(span.fret);
                float y1 = GetStringY(span.stringFrom);
                float y2 = GetStringY(span.stringTo);

                VSTGUI::CRect barreRect(
                    x - markerRadius,
                    y1 - markerRadius * 0.6f,
                    x + markerRadius,
                    y2 + markerRadius * 0.6f
                );

                auto path = ctx->createGraphicsPath();
                if (path)
                {
                    path->addRoundRect(
                        barreRect,
                        markerRadius * 0.6f   // ← 半径は1つだけ
                    );

                    ctx->drawGraphicsPath(
                        path,
                        VSTGUI::CDrawContext::kPathFilled
                    );
                    path->forget();
                }
            }
        }

        VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState&) override
        {
            if (context != FretBoardContext::Edit) return VSTGUI::kMouseEventNotHandled;

            int stringIndex =
                int((where.y - (boardSize.top + outerMargin)) / stringSpacing + 0.5);

            if (stringIndex < 0 || stringIndex >= STRING_COUNT)
                return VSTGUI::kMouseEventNotHandled;

            int fret = int((where.x - boardSize.left) / fretSpacing) - 1;

            if (fret < firstFret || fret > lastFret - 1)
                return VSTGUI::kMouseEventNotHandled;

            int& current = currentSet.data[stringIndex];

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

            if (editChordChangedCallback)
            {
                editChordChangedCallback(this, currentSet);
			}

            invalid();
            return VSTGUI::kMouseEventHandled;
        }

        // 現在の押弦情報
        StringSet getPressedFrets() const
        {
            return currentSet;
        }

        // 押弦情報を上書きして再描画
        void setPressedFrets(const StringSet& set)
        {
            currentSet = set;
            invalid();
        }

        void setContext(FretBoardContext ctx)
        {
            if (context == ctx) return;

            // --- exit ---
            switch (context)
            {
            case FretBoardContext::Edit:
                // Edit 終了時の後始末は View 側が判断する
                break;
            default:
                break;
            }

            // --- enter ---
            switch (ctx)
            {
            case FretBoardContext::Edit:
                saveStringSet();
                break;
            default:
                break;
            }

            context = ctx;
            invalid();
        }

        FretBoardContext getContext() const
        {
            return context;
        }

        void cancelEdit()
        {
            restoreStringSet();
            invalid();
        }

        CLASS_METHODS(CFretBoard, CControl)
    };
}