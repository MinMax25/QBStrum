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
    inline constexpr int MAX_STRINGS = 7;

    //======================================================================
    // ギター/ウクレレの弦数
    //======================================================================
    enum class StringCount : uint8_t
    {
        Ukulele4 = 4,
        Guitar6 = 6,
        Guitar7 = MAX_STRINGS,
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
    // ChordSpec
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
        std::array<int, MAX_STRINGS> data{};
        size_t size = 0; // 実際に使用する弦数
    };

    struct FlatChordEntry
    {
        int root = 0;
        int type = 0;
        int voicing = 0;
        std::string displayName;

        void generateDisplayName(const char* rootName, const char* typeName)
        {
            displayName = std::string(rootName) + " " + typeName + " (" + std::to_string(voicing + 1) + ")";
        }
    };

    struct ChordMapX
    {
        // Tunings
        StringSetX Tunings{ 64, 59, 55, 50, 45, 40 };

        // フラット化されたコード
        std::array<FlatChordEntry, MAX_FLATENTRIES> flatChords{};

        ChordSpec spec;

        std::array<std::string, MAX_DEFAULT_TYPES> DefaultTypeNames{};
        std::array<std::string, MAX_USER_TYPES> UserTypeNames{};
        std::array<std::string, MAX_DEFAULT_ROOTS + 1> RootNames{};

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
        // JSON から読み込みとフラット化統合
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
                    Tunings.data[(int)i] = arr[(int)i].GetInt();
                }
                Tunings.size = MAX_STRINGS;
            }

            // ChordRoots
            if (doc.HasMember("ChordRoots") && doc["ChordRoots"].IsArray())
            {
                auto& roots = doc["ChordRoots"].GetArray();

                for (int r = 0; r < (int)roots.Size() && r < MAX_DEFAULT_ROOTS + 1; r++)
                {
                    auto& rootObj = roots[r];
                    RootNames[r] = rootObj["Name"].GetString();

                    auto& types = rootObj["ChordTypes"].GetArray();

                    if (r < MAX_DEFAULT_ROOTS)
                    {
                        // システムルート
                        for (int t = 0; t < (int)types.Size() && t < MAX_DEFAULT_TYPES; t++)
                        {
                            DefaultTypeNames[t] = types[t]["Name"].GetString();

                            auto& voicings = types[t]["Voicings"].GetArray();
                            for (size_t v = 0; v < voicings.Size() && v < MAX_DEFAULT_VOICINGS; v++)
                            {
                                flatChords[toFlatIndex((int)r, (int)t, (int)v)].
                                    generateDisplayName(RootNames[r].c_str(), DefaultTypeNames[t].c_str());
                            }
                        }
                    }
                    else
                    {
                        // ユーザールート（root=12）
                        for (int t = 0; t < (int)types.Size() && t < MAX_USER_TYPES; t++)
                        {
                            UserTypeNames[t] = types[t]["Name"].GetString();

                            auto& voicings = types[t]["Voicings"].GetArray();
                            for (size_t v = 0; v < voicings.Size() && v < MAX_USER_VOICINGS; v++)
                            {
                                flatChords[toFlatIndex((int)r, (int)t, (int)v)].
                                    generateDisplayName(RootNames[r].c_str(), UserTypeNames[t].c_str());
                            }
                        }
                    }
                }
            }
        }

        //==================================================================
        // シングルトン
        //==================================================================
        static ChordMapX& instance()
        {
            static ChordMapX _instance;
            return _instance;
        }

    private:

        // コンストラクターは private にして直接生成を禁止
        ChordMapX() = default;

        // コピー禁止
        ChordMapX(const ChordMapX&) = delete;
        ChordMapX& operator=(const ChordMapX&) = delete;

        // ムーブ禁止
        ChordMapX(ChordMapX&&) = delete;
        ChordMapX& operator=(ChordMapX&&) = delete;
    };

} // namespace MinMax
