//------------------------------------------------------------------------
// ChordMap.h
//------------------------------------------------------------------------
#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
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
    inline constexpr int STRING_COUNT = 6;
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
    struct StringSet
    {
        std::array<int, STRING_COUNT> data{};
        size_t size = 0;
    };

    struct FlatChordEntry
    {
        int root = 0;
        int type = 0;
        int voicing = 0;
    };

    struct ChordMapX
    {
        // Tunings (開放弦ピッチ)
        StringSet Tunings{ 64, 59, 55, 50, 45, 40 };

        // フラット化されたコード
        std::array<FlatChordEntry, MAX_FLATENTRIES> flatChords{};

        ChordSpec spec;

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
                        flatChords[index++] = { r, t, v };
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
                        flatChords[index++] = { spec.defaultRootCount, t, v };
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
                for (size_t i = 0; i < arr.Size() && i < Tunings.size; i++)
                {
                    Tunings.data[i] = arr[i].GetInt();
                }
                Tunings.size = STRING_COUNT;
            }

            // ChordSpec
            if (doc.HasMember("system") && doc["system"].IsObject())
            {
                auto& sys = doc["system"];
                if (sys.HasMember("rootCount")) spec.defaultRootCount = sys["rootCount"].GetInt();
                if (sys.HasMember("typeCount")) spec.defaultTypeCount = sys["typeCount"].GetInt();
                if (sys.HasMember("voicingCount")) spec.defaultVoicingCount = sys["voicingCount"].GetInt();
            }
            if (doc.HasMember("user") && doc["user"].IsObject())
            {
                auto& usr = doc["user"];
                if (usr.HasMember("typeCount")) spec.userTypeCount = usr["typeCount"].GetInt();
                if (usr.HasMember("voicingCount")) spec.userVoicingCount = usr["voicingCount"].GetInt();
            }

            // ChordMap をフラット化
            initialize();
        }

        //==================================================================
        // JSON へ書き込み
        //==================================================================
        void saveToJson(const std::filesystem::path& path) const
        {
            rapidjson::Document doc;
            doc.SetObject();
            auto& allocator = doc.GetAllocator();

            // Tunings
            rapidjson::Value tuningArray(rapidjson::kArrayType);
            for (size_t i = 0; i < STRING_COUNT; i++)
            {
                tuningArray.PushBack(Tunings.data[i], allocator);
            }
            doc.AddMember("Tunings", tuningArray, allocator);

            // system
            rapidjson::Value systemObj(rapidjson::kObjectType);
            systemObj.AddMember("rootCount", spec.defaultRootCount, allocator);
            systemObj.AddMember("typeCount", spec.defaultTypeCount, allocator);
            systemObj.AddMember("voicingCount", spec.defaultVoicingCount, allocator);
            doc.AddMember("system", systemObj, allocator);

            // user
            rapidjson::Value userObj(rapidjson::kObjectType);
            userObj.AddMember("typeCount", spec.userTypeCount, allocator);
            userObj.AddMember("voicingCount", spec.userVoicingCount, allocator);
            doc.AddMember("user", userObj, allocator);

            // 書き込み
            std::ofstream ofs(path);
            if (!ofs.is_open())
            {
                throw std::runtime_error("Cannot open file for writing");
            }
            rapidjson::OStreamWrapper osw(ofs);
            rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
            doc.Accept(writer);
        }
    };

}
