//------------------------------------------------------------------------
// Copyright(c) 2024 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <cstdlib>
#include <filesystem>
#include <pluginterfaces/base/ftypes.h>
#include <pluginterfaces/base/funknown.h>
#include <string>
#include <system_error>
#include <vector>

namespace MinMax
{
    // プリセットファイル操作
    const struct Files
    {
        /// 定数
        inline static const char* STR_USERPROFILE = "USERPROFILE";
        inline static const char* PRESET_ROOT = "Documents/VST3 Presets/MinMax/QBStrum";

        inline static const VSTGUI::UTF8String TITLE = "QBStrum";
        inline static const VSTGUI::UTF8String FILTER = "Chord Preset(.json)";
        inline static const VSTGUI::UTF8String FILE_EXT = "json";

        // プリセットパスを取得する
        inline static std::filesystem::path getPresetPath()
        {
            const char* home =
#ifdef _WIN32
                getenv("USERPROFILE");
#else
                getenv("HOME");
#endif
            if (!home) return std::filesystem::current_path();

            return std::filesystem::path(home).append(PRESET_ROOT).make_preferred();
        }

        // プリセットディレクトリを作成する
        inline static void createPresetDirectory()
        {
            auto p = getPresetPath();
            if (!std::filesystem::exists(p))
            {
                std::filesystem::create_directories(p);
            }
        }

        // プリセットファイルのパス一覧を取得する 
        inline static Steinberg::tresult getPresetFiles(std::vector<std::string>& file_names)
        {
            auto path = getPresetPath();
            if (!std::filesystem::exists(path)) return Steinberg::kResultFalse;

            std::filesystem::directory_iterator iter(getPresetPath()), end;
            std::error_code err;

            for (; iter != end && !err; iter.increment(err))
            {
                const std::filesystem::directory_entry entry = *iter;
                if (entry.path().extension() != ".json") continue;
                file_names.push_back(entry.path().string());
            }

            return Steinberg::kResultTrue;
        }
    };
}