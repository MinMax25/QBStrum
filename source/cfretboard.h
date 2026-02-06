//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <base/source/fstring.h>
#include <vector>
#include <array>
#include <functional>
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
    enum class FretBoardContext { View, Edit, Guess };

    struct BarreSpan
    {
        int fret;
        int stringFrom;
        int stringTo;
    };

    struct FretBoardStyle
    {
        VSTGUI::CColor bg{ 55, 35, 18, 255 };
        VSTGUI::CColor stringColor{ 230, 230, 230, 255 };
        VSTGUI::CColor fretColor{ 180, 180, 180, 255 };
        VSTGUI::CColor nutColor{ 255, 255, 255, 255 };
        VSTGUI::CColor markerColor{ 200, 200, 200, 255 };
        VSTGUI::CColor fretNumberColor{ 220, 220, 220, 255 };
        VSTGUI::CColor pressedColor{ 255, 140, 0, 255 };
        VSTGUI::CColor pressedOffsetColor{ 255, 0, 255, 255 };
        VSTGUI::CColor muteColor{ 255, 0, 0, 255 };
        VSTGUI::CColor barreColor{ 255, 140, 0, 80 };

        float markerRadius = 8.0f;
        double outerMargin = 10.0;
    };

    class CFretBoard : public VSTGUI::CControl
    {
    public:
        using EditChordChanged = std::function<void(CFretBoard*, StringSet)>;

        CFretBoard(const VSTGUI::CRect& size, EditChordChanged cb)
            : CControl(size), editChordChangedCallback(cb)
        {
            updateLayout();
        }

        void updateLayout()
        {
            const auto& frame = getViewSize();
            boardSize = VSTGUI::CRect(frame.left + 65, frame.top + 20, frame.right, frame.bottom - 15);
            double usableHeight = boardSize.getHeight() - style.outerMargin * 2;
            stringSpacing = usableHeight / (STRING_COUNT - 1);
            fretSpacing = boardSize.getWidth() / kNumFrets;
        }

        void draw(VSTGUI::CDrawContext* pContext) override
        {
            using namespace VSTGUI;

            pContext->setFillColor(style.bg);
            pContext->drawRect(boardSize, kDrawFilled);

            pContext->setFrameColor(style.stringColor);
            pContext->setLineWidth(2.0);
            for (int i = 0; i < STRING_COUNT; ++i)
            {
                float y = getStringY(i);
                pContext->drawLine({ (float)boardSize.left, y }, { (float)boardSize.right, y });
            }

            for (int f = kFirstFret; f <= kLastFret; ++f)
            {
                double x = boardSize.left + fretSpacing * (f - kFirstFret);
                if (f == 0)
                {
                    pContext->setFrameColor(style.nutColor);
                    pContext->setLineWidth(5.0);
                    pContext->drawLine({ (float)x - 3, (float)boardSize.top }, { (float)x - 3, (float)boardSize.bottom });
                }
                else
                {
                    pContext->setFrameColor(style.fretColor);
                    pContext->setLineWidth(2.0);
                    pContext->drawLine({ (float)x, (float)boardSize.top }, { (float)x, (float)boardSize.bottom });
                }
            }

            pContext->setFillColor(style.markerColor);
            for (int f = 1; f <= kLastFret; ++f)
            {
                if (!isMarkerFret(f)) continue;
                double x = getFretCenterX(f);
                if (f == 11 || f == 22)
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

            pContext->setFontColor(style.fretNumberColor);
            pContext->setFont(kNormalFontVerySmall, 10);
            for (int f = 0; f <= kLastFret; ++f)
            {
                double x = getFretCenterX(f);
                Steinberg::String text;
                text.printf("%d", f);
                CRect textRect(x - 10, boardSize.bottom + 2, x + 10, boardSize.bottom + 12);
                pContext->drawString(text, textRect, kCenterText, true);
            }

            drawBarres(pContext);
            drawNotes(pContext);
        }

        void drawBarres(VSTGUI::CDrawContext* ctx)
        {
            ctx->setFillColor(style.barreColor);
            for (const auto& span : cachedBarreSpans)
            {
                float x = getFretCenterX(span.fret);
                float y1 = getStringY(span.stringFrom);
                float y2 = getStringY(span.stringTo);
                float r = style.markerRadius;
                VSTGUI::CRect barreRect(x - r, y1 - r * 0.6f, x + r, y2 + r * 0.6f);
                auto path = ctx->createGraphicsPath();
                if (path)
                {
                    path->addRoundRect(barreRect, r * 0.6f);
                    ctx->drawGraphicsPath(path, VSTGUI::CDrawContext::kPathFilled);
                    path->forget();
                }
            }
        }

        VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState&) override
        {
            if (context != FretBoardContext::Edit) return VSTGUI::kMouseEventNotHandled;

            int sIdx = int((where.y - (boardSize.top + style.outerMargin)) / stringSpacing + 0.5);
            if (sIdx < 0 || sIdx >= STRING_COUNT) return VSTGUI::kMouseEventNotHandled;

            int fret = int((where.x - boardSize.left) / fretSpacing) - 1;
            if (fret < kFirstFret || fret > kLastFret - 1) return VSTGUI::kMouseEventNotHandled;

            int& cur = currentSet.data[sIdx];
            if (fret < 0)
            {
                cur = (cur == -1) ? 0 : -1;
            }
            else
            {
                int newVal = fret + 1;
                if (cur == newVal)
                {
                    cur = (cur > 0) ? 0 : (cur == 0 ? -1 : newVal);
                }
                else
                {
                    cur = newVal;
                }
            }

            updateBarreCache();
            if (editChordChangedCallback) editChordChangedCallback(this, currentSet);
            invalid();
            return VSTGUI::kMouseEventHandled;
        }

        void setPressedFrets(const StringSet& set)
        {
            currentSet = set;
            updateBarreCache();
            invalid();
        }

        void setContext(FretBoardContext ctx)
        {
            if (context == ctx) return;
            if (ctx == FretBoardContext::Edit)
            {
                saveStringSet();
                if (editChordChangedCallback) editChordChangedCallback(this, currentSet);
            }
            context = ctx;
            invalid();
        }

        void saveStringSet() { workingSet = currentSet; }
        void restoreStringSet() { currentSet = workingSet; updateBarreCache(); }
        void cancelEdit() { restoreStringSet(); invalid(); }
        FretBoardContext getContext() const { return context; }
        StringSet getPressedFrets() const { return currentSet; }

        CLASS_METHODS(CFretBoard, CControl)

    private:
        static constexpr int kLastFret = 19;
        static constexpr int kFirstFret = -1;
        static constexpr int kNumFrets = (kLastFret - kFirstFret + 1);

        FretBoardStyle style;
        VSTGUI::CRect boardSize;
        double stringSpacing = 0.0;
        double fretSpacing = 0.0;

        StringSet currentSet{};
        StringSet workingSet{};
        std::vector<BarreSpan> cachedBarreSpans;
        FretBoardContext context{ FretBoardContext::View };
        EditChordChanged editChordChangedCallback;

        // --- 内部ロジック：座標計算 ---
        float getFretCenterX(int fret) const
        {
            return static_cast<float>(boardSize.left + fretSpacing * (fret - kFirstFret - 1) + fretSpacing * 0.5);
        }

        float getStringY(int stringIndex) const
        {
            return static_cast<float>(boardSize.top + style.outerMargin + stringSpacing * stringIndex);
        }

        bool isMarkerFret(int f) const
        {
            static const std::vector<int> markers = { 2, 4, 6, 8, 11, 14, 16, 18 };
            return std::find(markers.begin(), markers.end(), f) != markers.end();
        }

        // --- 内部ロジック：バレーコード判定 ---
        void updateBarreCache()
        {
            using BarreFlags = std::array<bool, STRING_COUNT>;
            std::array<BarreFlags, kLastFret> info{};

            for (int fret = 0; fret < kLastFret; ++fret)
            {
                bool hasAnyNote = false;
                for (int s = 0; s < STRING_COUNT; ++s)
                {
                    if (currentSet.data[s] == fret)
                    {
                        hasAnyNote = true;
                        break;
                    }
                }
                for (int s = 0; s < STRING_COUNT; ++s)
                {
                    if (currentSet.data[s] == -1)
                    {
                        hasAnyNote = false;
                    }
                    info[fret][s] = hasAnyNote;
                }
            }

            for (int s = 0; s < STRING_COUNT; ++s)
            {
                int minFret = -1;
                int count = 0;
                for (int f = 0; f < kLastFret; ++f)
                {
                    if (info[f][s])
                    {
                        if (minFret < 0) minFret = f;
                        ++count;
                    }
                }
                if (count <= 1) continue;
                for (int f = minFret + 1; f < kLastFret; ++f)
                {
                    if (currentSet.data[s] != f) info[f][s] = false;
                }
                if (info[0][s] && currentSet.data[s] != 0) info[0][s] = false;
            }

            cachedBarreSpans.clear();
            for (int fret = 1; fret < kLastFret; ++fret)
            {
                int start = -1;
                for (int s = 0; s < STRING_COUNT; ++s)
                {
                    if (info[fret][s])
                    {
                        if (start < 0) start = s;
                    }
                    else
                    {
                        if (start >= 0)
                        {
                            if (s - start >= 2)
                            {
                                cachedBarreSpans.push_back({ fret, start, s - 1 });
                            }
                            start = -1;
                        }
                    }
                }
                if (start >= 0 && STRING_COUNT - start >= 2)
                {
                    cachedBarreSpans.push_back({ fret, start, STRING_COUNT - 1 });
                }
            }
        }

        // --- 描画ヘルパー ---
        void drawNotes(VSTGUI::CDrawContext* pContext)
        {
            for (unsigned int i = 0; i < STRING_COUNT; ++i)
            {
                int fret = currentSet.data[i];
                int offset = currentSet.getOffset(i);
                bool hasOffset = (offset != 0);

                int effectiveFret = fret;
                int rawType = offset + StringSet::CENTER_OFFSET;

                if (rawType == 0) effectiveFret = -1;       // Mute
                else if (rawType == 1) effectiveFret = 0;   // Open
                else effectiveFret += offset;

                bool outOfRange = hasOffset && (effectiveFret < -1 || effectiveFret > kLastFret);
                double y = getStringY(i);

                if (hasOffset)
                {
                    pContext->setFillColor(style.pressedOffsetColor);
                }
                else
                {
                    pContext->setFillColor(style.pressedColor);
                }
                pContext->setFrameColor(pContext->getFillColor());

                if (effectiveFret == -1 || outOfRange)
                {
                    double x = boardSize.left + fretSpacing * 0.5;
                    const double s = 6.0;
                    if (!hasOffset) pContext->setFrameColor(style.muteColor);
                    pContext->drawLine({ (float)(x - s), (float)(y - s) }, { (float)(x + s), (float)(y + s) });
                    pContext->drawLine({ (float)(x - s), (float)(y + s) }, { (float)(x + s), (float)(y - s) });
                    continue;
                }

                if (effectiveFret == 0 && !hasOffset) continue;

                int internalFret = effectiveFret - 1;
                if (internalFret < kFirstFret || internalFret >= kLastFret) continue;

                double x = getFretCenterX(effectiveFret);
                const double s = 8.0;
                std::vector<VSTGUI::CPoint> pts = {
                    {(float)x, (float)(y - s)}, {(float)(x + s), (float)y},
                    {(float)x, (float)(y + s)}, {(float)(x - s), (float)y}
                };
                pContext->drawPolygon(pts, VSTGUI::kDrawFilled);
            }
        }
    };
}