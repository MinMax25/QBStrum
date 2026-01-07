//------------------------------------------------------------------------
// ChordMapX.h
//------------------------------------------------------------------------
#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

namespace MinMax
{

    //======================================================================
    // ギター/ウクレレの弦数
    //======================================================================
    enum class StringCount : uint8_t
    {
        Ukulele4 = 4,
        Guitar6 = 6,
        Guitar7 = 7,
    };

    struct StringCountEntry
    {
        const char* name;
        StringCount value;
    };

    inline constexpr StringCountEntry StringCountTable[] =
    {
        { "Ukulele4", StringCount::Ukulele4 },
        { "Guitar6",  StringCount::Guitar6 },
        { "Guitar7",  StringCount::Guitar7 },
    };

    inline StringCount parseStringCount(const char* str)
    {
        for (auto& e : StringCountTable)
        {
            if (std::strcmp(e.name, str) == 0)
            {
                return e.value;
            }
        }
        return StringCount::Guitar6;
    }

    //======================================================================
    // 定数
    //======================================================================
    inline constexpr int MAX_DEFAULT_ROOTS = 12;
    inline constexpr int MAX_DEFAULT_TYPES = 29;
    inline constexpr int MAX_DEFAULT_VOICINGS = 3;
    inline constexpr int MAX_USER_TYPES = 5;
    inline constexpr int MAX_USER_VOICINGS = 24;
    inline constexpr int MAX_FLATENTRIES = MAX_DEFAULT_ROOTS * MAX_DEFAULT_TYPES * MAX_DEFAULT_VOICINGS + MAX_USER_TYPES * MAX_USER_VOICINGS;

    //======================================================================
    // ChordSpec (DIでJSONから読み込む定数情報)
    //======================================================================
    struct ChordSpec
    {
        int stringCount = 6; // ここを追加
        int defaultRootCount = MAX_DEFAULT_ROOTS;
        int defaultTypeCount = MAX_DEFAULT_TYPES;
        int defaultVoicingCount = MAX_DEFAULT_VOICINGS;

        int userTypeCount = MAX_USER_TYPES;
        int userVoicingCount = MAX_USER_VOICINGS;

        int defaultBlockSize() const
        {
            return defaultRootCount * defaultTypeCount * defaultVoicingCount;
        }
    };

    //======================================================================
    // ChordMap データ構造
    //======================================================================
    struct StringSetX
    {
        std::array<int, 7> data{}; // 7弦まで対応
        size_t size = 0;
    };

    struct FlatChordEntry
    {
        int root = 0;
        int type = 0;
        int voicing = 0;
        std::string displayName;
    };

    struct ChordMapX
    {
        // Tunings (開放弦ピッチ)
        StringSetX Tunings{ {64, 59, 55, 50, 45, 40}, 6 };

        // フラット化されたコード
        std::array<FlatChordEntry, MAX_FLATENTRIES> flatChords{};

        ChordSpec spec;

        // Root/Type 名
        std::vector<std::string> RootNames;
        std::vector<std::string> TypeNames;

        //==================================================================
        // flatIndex への変換
        //==================================================================
        inline uint16_t toFlatIndex(int root, int type, int voicing) const
        {
            if (root < spec.defaultRootCount)
            {
                return (root * spec.defaultTypeCount + type) * spec.defaultVoicingCount + voicing;
            }
            return spec.defaultBlockSize() + type * spec.userVoicingCount + voicing;
        }

        //==================================================================
        // flatIndex から root/type/voicing を取得
        //==================================================================
        inline void getChordInfo(uint16_t index, int& root, int& type, int& voicing) const
        {
            if (index < spec.defaultBlockSize())
            {
                root = index / (spec.defaultTypeCount * spec.defaultVoicingCount);
                int rem = index % (spec.defaultTypeCount * spec.defaultVoicingCount);
                type = rem / spec.defaultVoicingCount;
                voicing = rem % spec.defaultVoicingCount;
            }
            else
            {
                int idx = index - spec.defaultBlockSize();
                root = spec.defaultRootCount; // ユーザー定義ブロック
                type = idx / spec.userVoicingCount;
                voicing = idx % spec.userVoicingCount;
            }
        }

        //==================================================================
        // ChordMap 初期化
        //==================================================================
        void initialize()
        {
            int index = 0;

            // デフォルトブロック
            for (int r = 0; r < spec.defaultRootCount; r++)
            {
                for (int t = 0; t < spec.defaultTypeCount; t++)
                {
                    for (int v = 0; v < spec.defaultVoicingCount; v++)
                    {
                        flatChords[index].root = r;
                        flatChords[index].type = t;
                        flatChords[index].voicing = v;
                        flatChords[index].displayName = RootNames[r] + " " + TypeNames[t] + " (" + std::to_string(v + 1) + ")";
                        index++;
                    }
                }
            }

            // ユーザーブロック
            for (int t = 0; t < spec.userTypeCount; t++)
            {
                for (int v = 0; v < spec.userVoicingCount; v++)
                {
                    if (index < MAX_FLATENTRIES)
                    {
                        flatChords[index].root = spec.defaultRootCount;
                        flatChords[index].type = t;
                        flatChords[index].voicing = v;
                        flatChords[index].displayName = "User Page " + std::to_string(t + 1) + " (" + std::to_string(v + 1) + ")";
                        index++;
                    }
                }
            }
        }

        //==================================================================
        // JSON から読み込み
        //==================================================================
        void loadFromJson(const std::filesystem::path& path)
        {
            std::ifstream ifs(path);
            if (!ifs.is_open())
            {
                throw std::runtime_error("Cannot open JSON file");
            }

            rapidjson::IStreamWrapper isw(ifs);
            rapidjson::Document doc;
            doc.ParseStream(isw);

            if (!doc.IsObject())
            {
                throw std::runtime_error("Invalid JSON format");
            }

            // Tunings
            if (doc.HasMember("Tunings") && doc["Tunings"].IsArray())
            {
                auto& arr = doc["Tunings"].GetArray();
                for (size_t i = 0; i < arr.Size() && i < static_cast<size_t>(spec.stringCount); i++)
                {
                    Tunings.data[i] = arr[i].GetInt();
                }
                Tunings.size = static_cast<size_t>(spec.stringCount);
            }

            // Root/Type 名 (固定)
            if (doc.HasMember("RootNames") && doc["RootNames"].IsArray())
            {
                auto& arr = doc["RootNames"].GetArray();
                RootNames.clear();
                for (auto& v : arr)
                {
                    RootNames.push_back(v.GetString());
                }
            }
            if (doc.HasMember("TypeNames") && doc["TypeNames"].IsArray())
            {
                auto& arr = doc["TypeNames"].GetArray();
                TypeNames.clear();
                for (auto& v : arr)
                {
                    TypeNames.push_back(v.GetString());
                }
            }

            // ChordMap をフラット化
            initialize();
        }
    };

}
