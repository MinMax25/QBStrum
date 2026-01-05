//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------

#include <cstdint>
#include <functional>
#include <vstgui/lib/cbuttonstate.h>
#include <vstgui/lib/ccolor.h>
#include <vstgui/lib/cdrawcontext.h>
#include <vstgui/lib/cdrawdefs.h>
#include <vstgui/lib/cfont.h>
#include <vstgui/lib/controls/cbuttons.h>
#include <vstgui/lib/controls/icontrollistener.h>
#include <vstgui/lib/cpoint.h>
#include <vstgui/lib/crect.h>
#include <vstgui/lib/cstring.h>
#include <vstgui/lib/vstguifwd.h>

namespace MinMax
{
    // メニュー表示
    class CMenuButton
        : public VSTGUI::CTextButton
    {
    public:
        using Callback = std::function<void(CMenuButton*)>;

        CMenuButton(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag, const VSTGUI::UTF8String& title, Callback cb)
            : VSTGUI::CTextButton(size, listener, tag, title)
            , callback(cb)
        {
            setFrameColor(VSTGUI::kTransparentCColor);
            setGradient(nullptr);
            setTextColor(VSTGUI::kWhiteCColor);
            setFont(VSTGUI::kNormalFontSmall);
        }

        VSTGUI::CMouseEventResult onMouseEntered(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override
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
            // hover 背景
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
}
