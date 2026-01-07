//------------------------------------------------------------------------
// ChordMap.h（基準版）
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
    struct StringSetX
    {
        std::array<int, 6> data{}; // Guitar6 がデフォルト
        size_t size = 0;
    };

    struct FlatChordEntry
    {
        int root = 0;
        int type = 0;
        int voicing = 0;
        std::string displayName; // JSON から読み込む表示名用
    };

    struct ChordMapX
    {
        // Tunings (開放弦ピッチ)
        StringSetX Tunings{ 64, 59, 55, 50, 45, 40 };

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
                        flatChords[index++] = { r, t, v, "" };
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
                        flatChords[index++] = { spec.defaultRootCount, t, v, "" };
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
                Tunings.size = 6;
            }

            // ChordMap をフラット化
            initialize();

            // Root/Type/Voicing の displayName も取り込み
            if (doc.HasMember("ChordRoots") && doc["ChordRoots"].IsArray())
            {
                const auto& roots = doc["ChordRoots"].GetArray();
                int flatIndex = 0;
                for (size_t r = 0; r < roots.Size() && flatIndex < MAX_FLATENTRIES; r++)
                {
                    const auto& rootObj = roots[r];
                    if (!rootObj.HasMember("ChordTypes") || !rootObj["ChordTypes"].IsArray())
                        continue;

                    const auto& types = rootObj["ChordTypes"].GetArray();
                    for (size_t t = 0; t < types.Size() && flatIndex < MAX_FLATENTRIES; t++)
                    {
                        const auto& typeObj = types[t];
                        if (!typeObj.HasMember("Voicings") || !typeObj["Voicings"].IsArray())
                            continue;

                        const auto& voicings = typeObj["Voicings"].GetArray();
                        for (size_t v = 0; v < voicings.Size() && flatIndex < MAX_FLATENTRIES; v++)
                        {
                            const auto& voObj = voicings[v];
                            if (voObj.HasMember("Name") && voObj["Name"].IsString())
                            {
                                flatChords[flatIndex].displayName = voObj["Name"].GetString();
                            }
                            flatIndex++;
                        }
                    }
                }
            }
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
            for (size_t i = 0; i < 6; i++)
            {
                tuningArray.PushBack(Tunings.data[i], allocator);
            }
            doc.AddMember("Tunings", tuningArray, allocator);

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

        //==================================================================
        // シングルトンアクセス
        //==================================================================
        static ChordMapX& instance()
        {
            static ChordMapX inst;
            return inst;
        }

    private:
        ChordMapX() { initialize(); } // private コンストラクタ
    };

} // namespace MinMax
