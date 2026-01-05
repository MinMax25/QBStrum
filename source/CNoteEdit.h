//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <string>
#include <vstgui/lib/cbitmap.h>
#include <vstgui/lib/controls/ctextedit.h>
#include <vstgui/lib/controls/icontrollistener.h>
#include <vstgui/lib/crect.h>
#include <vstgui/lib/vstguibase.h>

#include "plugdefine.h"

namespace MinMax
{
    using namespace VSTGUI;

    // ノート番号入力
    class CNoteEdit
        : public CTextEdit
    {
        static constexpr int NOTE_COUNT = 128;
    public:

        CNoteEdit(const CRect& size, IControlListener* listener, int32_t tag, UTF8StringPtr txt = nullptr, CBitmap* background = nullptr, const int32_t style = 0)
            : CTextEdit(size, listener, tag, txt, background, style)
        {
            setStringToValueFunction(StringToValueFunction);
        }

        static bool StringToValueFunction(UTF8StringPtr txt, float& result, CTextEdit* textEdit)
        {
            // バインドされていない場合は無視
            if (textEdit->getTag() < 0) return false;

            std::string key = txt;
            std::transform(key.begin(), key.end(), key.begin(), ::toupper);

            // 入力がノート名の場合、辞書引したインデックスを値とする
            auto& itr = std::find(NoteNames.begin(), NoteNames.end(), key);
            if (itr != NoteNames.end())
            {
                result = std::distance(NoteNames.begin(), itr);
                return true;
            }

            // 入力が数値だった場合はそのまま変換する
            int val = std::atoi(key.c_str());
            if (val < NOTE_COUNT) {
                result = val;
                return true;
            }

            return false;
        }

        CLASS_METHODS(CNoteEdit, CTextEdit)
    };
}