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
#include <vstgui/lib/cstring.h>

namespace MinMax
{
    // プリセットファイル操作
    const struct Files
    {
        /// 定数
        inline static const char* STR_USERPROFILE = "USERPROFILE";
        inline static const char* PLUGIN_ROOT = "Documents/VST3 Presets/MinMax/QBStrum";
        inline static const char* PRESET_ROOT = "Documents/VST3 Presets/MinMax/QBStrum/ChordPreset";

        inline static const VSTGUI::UTF8String TITLE = "QBStrum";
        inline static const VSTGUI::UTF8String FILTER = "Chord Preset(.qbs)";
        inline static const VSTGUI::UTF8String FILE_EXT = "qbs";

        inline static const std::string DEFAULT_PRESET = "Standard 6Strings.qbs";

        // プラグインパスを取得する
        inline static std::filesystem::path getPluginPath()
        {
            const char* home =
#ifdef _WIN32
                getenv("USERPROFILE");
#else
                getenv("HOME");
#endif
            if (!home) return std::filesystem::current_path();

            return std::filesystem::path(home).append(PLUGIN_ROOT).make_preferred();
        }

        // プラグインディレクトリを作成する
        inline static void createPluginDirectory()
        {
            auto p = getPluginPath();
            if (!std::filesystem::exists(p))
            {
                std::filesystem::create_directories(p);
            }
        }

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
                if (entry.path().extension() != std::string("." + Files::FILE_EXT)) continue;
                file_names.push_back(entry.path().string());
            }

            return Steinberg::kResultTrue;
        }
    };
}