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
#include <sstream>
#include <vstgui/lib/cstring.h>

#pragma warning(disable : 4996)

namespace MinMax
{
    struct Files
    {
        // 定数 (文字列リテラル)
        inline static const char* PLUGIN_ROOT = "Documents/VST3 Presets/MinMax/QBStrum";
        inline static const char* PLUGIN_TEMP = "Documents/VST3 Presets/MinMax/QBStrum/Temp";
        inline static const char* PRESET_ROOT = "Documents/VST3 Presets/MinMax/QBStrum/ChordPreset";

        inline static const VSTGUI::UTF8String TITLE = "QBStrum";
        inline static const VSTGUI::UTF8String FILTER = "Chord Preset(.qbs)";

        // ファイル操作で使いやすいよう std::string で定義
        inline static const VSTGUI::UTF8String FILE_EXT = "qbs";
        inline static const std::string DEFAULT_PRESET = "default.qbs";

    private:
        // ホームディレクトリ取得の共通ロジック
        static std::filesystem::path getBasePath()
        {
            const char* home =
#ifdef _WIN32
                getenv("USERPROFILE");
#else
                getenv("HOME");
#endif
            return home ? std::filesystem::path(home) : std::filesystem::current_path();
        }

    public:
        inline static std::filesystem::path getRootPath()
        {
            return (getBasePath() / PLUGIN_ROOT).make_preferred();
        }

        inline static std::filesystem::path getTempPath()
        {
            return (getBasePath() / PLUGIN_TEMP).make_preferred();
        }

        inline static std::filesystem::path getPresetPath()
        {
            return (getBasePath() / PRESET_ROOT).make_preferred();
        }

        inline static void createPluginDirectory()
        {
            const std::array<std::filesystem::path, 3> paths = { getRootPath(), getTempPath(), getPresetPath() };
            for (const auto& path : paths)
            {
                if (!std::filesystem::exists(path))
                {
                    std::filesystem::create_directories(path);
                }
            }
        }

        inline static void clearTempFiles()
        {
            auto path = getTempPath();
            if (!std::filesystem::exists(path)) return;

            const auto now = std::filesystem::file_time_type::clock::now();
            constexpr auto limit = std::chrono::hours(24);

            std::error_code err;
            for (const auto& entry : std::filesystem::directory_iterator(path, err))
            {
                if (entry.is_regular_file(err))
                {
                    if (now - entry.last_write_time(err) >= limit)
                    {
                        std::filesystem::remove(entry.path(), err);
                    }
                }
            }
        }

        inline static Steinberg::tresult getPresetFiles(std::vector<std::string>& file_names)
        {
            auto path = getPresetPath();
            if (!std::filesystem::exists(path)) return Steinberg::kResultFalse;

            std::error_code err;
            std::string extWithDot = "." + FILE_EXT;

            for (const auto& entry : std::filesystem::directory_iterator(path, err))
            {
                if (entry.path().extension() == extWithDot)
                {
                    file_names.push_back(entry.path().string());
                }
            }
            return Steinberg::kResultTrue;
        }
    };
}