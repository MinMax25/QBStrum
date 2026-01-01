#pragma once

#if defined(_WIN32)

#include <windows.h>
#include <cstdarg>
#include <cstdio>

inline void DLogWrite(const char* fmt, ...)
{
    char buf[512];

    va_list args;
    va_start(args, fmt);
    vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, args);
    va_end(args);

    OutputDebugStringA(buf);
}

inline void DLogWriteLine(const char* fmt, ...)
{
    char buf[512];

    va_list args;
    va_start(args, fmt);
    vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, args);
    va_end(args);

    OutputDebugStringA(buf);
    OutputDebugStringA("\n");
}

#else

// ”ñWindowsŠÂ‹«‚Å‚Í no-op
inline void DebugLog(const char*, ...) {}

#endif
