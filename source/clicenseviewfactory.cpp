//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------

#include <vstgui/lib/controls/ctextedit.h>
#include <vstgui/lib/cscrollview.h>
#include <vstgui/lib/crect.h>
#include <vstgui/lib/cview.h>
#include <vstgui/lib/vstguibase.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>
#include <vstgui/uidescription/iuidescription.h>
#include <vstgui/uidescription/iviewcreator.h>
#include <vstgui/uidescription/uiattributes.h>
#include <vstgui/uidescription/uiviewfactory.h>

namespace MinMax
{
    class CLicenseViewFactory
        : public  VSTGUI::ViewCreatorAdapter
    {
    private:
        const char* kFullLicenseText =
            "/**\n"
            " * QBStrum\n"
            " */\n"
            "MIT License\n"
            "\n"
            "Copyright (c) 2025 Min Max\n"
            "\n"
            "Permission is hereby granted, free of charge, to any person obtaining a copy\n"
            "of this software and associated documentation files (the \"Software\"), to deal\n"
            "in the Software without restriction, including without limitation the rights\n"
            "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
            "copies of the Software, and to permit persons to whom the Software is\n"
            "furnished to do so, subject to the following conditions:\n"
            "\n"
            "The above copyright notice and this permission notice shall be included in all\n"
            "copies or substantial portions of the Software.\n"
            "\n"
            "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
            "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
            "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
            "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
            "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
            "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
            "SOFTWARE.\n"
            "\n"
            "/**\n"
            " * Steinberg VST3 SDK\n"
            " */\n"
            "MIT License\n"
            "\n"
            "Copyright (c) 2025 Steinberg Media Technologies GmbH\n"
            "\n"
            "...（以下、貼ってくれた全文をそのまま続ける）\n";

    public:

        CLicenseViewFactory() { VSTGUI::UIViewFactory::registerViewCreator(*this); }

        VSTGUI::IdStringPtr getViewName() const override { return "UI:License View"; }

        VSTGUI::IdStringPtr getBaseViewName() const override { return  VSTGUI::UIViewCreator::kCView; }

        VSTGUI::CView* create(const  VSTGUI::UIAttributes& attributes, const  VSTGUI::IUIDescription* description) const override
        {
            using namespace VSTGUI;

            const CRect size = CRect(0, 0, 200, 200);

            // ScrollView
            auto* scroll = new CScrollView(size, size, 0);
            scroll->setScrollbarWidth(14);
            scroll->setAutosizeFlags(kAutosizeAll);
            scroll->setBackgroundColor(kGreyCColor);

            // TextLabel（高さは十分大きく）
            CRect textSize(0, 0, size.getWidth() - 14, 200);

            auto* label = new CTextLabel(textSize, kFullLicenseText);
            label->setHoriAlign(kLeftText);
            label->setFontColor(kWhiteCColor);
            label->setBackColor(kGreyCColor);

            scroll->addView(label);
            scroll->setContainerSize(textSize);

            return scroll;
        }
    };

    CLicenseViewFactory __gCLicenseViewFactory;
}