//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------

#include <vstgui/plugin-bindings/vst3editor.h>
#include <vstgui/vstgui_uidescription.h>
#include <vstgui/uidescription/detail/uiviewcreatorattributes.h>

#include "cnotelabel.h"

namespace MinMax
{
    using namespace VSTGUI;

    class CNoteLabelFactory
        : public ViewCreatorAdapter
    {
    public:

        CNoteLabelFactory() { UIViewFactory::registerViewCreator(*this); }

        IdStringPtr getViewName() const override { return "UI:Note Label"; }

        IdStringPtr getBaseViewName() const override { return UIViewCreator::kCTextLabel; }

        CView* create(const UIAttributes& attributes, const IUIDescription* description) const override
        {
            return new CNoteLabel(CRect(CPoint(0, 0), CPoint(40, 15)));
        }
    };

    CNoteLabelFactory __gCNoteLabelFactory;
}