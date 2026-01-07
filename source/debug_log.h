#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <vector>        // ← 追加
#include <string>

#if defined(_WIN32)
#include <windows.h>
#endif

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

namespace MinMax
{

    //======================================================================
    // デバッグ出力ラッパー
    //======================================================================
    inline void DLogWrite(const char* fmt, ...)
    {
#if defined(_WIN32)
        va_list args;
        va_start(args, fmt);

        std::vector<char> buf(512);
        int n = vsnprintf_s(buf.data(), buf.size(), _TRUNCATE, fmt, args);
        if (n < 0 || n >= static_cast<int>(buf.size()))
        {
            buf.resize(n + 1);
            vsnprintf_s(buf.data(), buf.size(), _TRUNCATE, fmt, args);
        }

        va_end(args);
        OutputDebugStringA(buf.data());
#else
        (void)fmt;
#endif
    }

    inline void DLogWriteLine(const char* fmt, ...)
    {
#if defined(_WIN32)
        va_list args;
        va_start(args, fmt);

        std::vector<char> buf(512);
        int n = vsnprintf_s(buf.data(), buf.size(), _TRUNCATE, fmt, args);
        if (n < 0 || n >= static_cast<int>(buf.size()))
        {
            buf.resize(n + 1);
            vsnprintf_s(buf.data(), buf.size(), _TRUNCATE, fmt, args);
        }

        va_end(args);
        OutputDebugStringA(buf.data());
        OutputDebugStringA("\n");
#else
        (void)fmt;
#endif
    }

    //======================================================================
    // RapidJSON Document/Value を文字列化してデバッグ出力
    //======================================================================
    inline void DLogJson(const rapidjson::Document& doc)
    {
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        DLogWriteLine("%s", buffer.GetString());
    }

    inline void DLogJson(const rapidjson::Value& val)
    {
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        val.Accept(writer);
        DLogWriteLine("%s", buffer.GetString());
    }

}
