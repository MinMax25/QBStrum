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
    // 7弦まで対応可能とする
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

    //======================================================================
    // ChordMap Spec
    //======================================================================
    struct ChordSpec
    {
        static constexpr int defaultRootCount = 12;
        static constexpr int defaultTypeCount = 29;
        static constexpr int defaultVoicingCount = 3;

        static constexpr int userRootCount = 1;
        static constexpr int userTypeCount = 5;
        static constexpr int userVoicingCount = 24;

        static constexpr int flatEntryCount = 
            (defaultRootCount * defaultTypeCount * defaultVoicingCount) + 
            (userRootCount * userTypeCount * userVoicingCount);

        int defaultBlockSize() const
        {
            return defaultRootCount * defaultTypeCount * defaultVoicingCount;
        }
    };

    struct StringSetX
    {
        // fret position
        std::array<int, MAX_STRINGS> data{};

        // valid strings
        size_t size = 0;
    };

    //======================================================================
    // ChordMap
    //======================================================================
    class ChordMapX
    {
    private:
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

        // コンストラクターは private にして直接生成を禁止
        ChordMapX() = default;

        // コピー禁止
        ChordMapX(const ChordMapX&) = delete;
        ChordMapX& operator=(const ChordMapX&) = delete;

        // ムーブ禁止
        ChordMapX(ChordMapX&&) = delete;
        ChordMapX& operator=(ChordMapX&&) = delete;

    public:
        //==================================================================
        // シングルトン
        //==================================================================
        static ChordMapX& instance()
        {
            static ChordMapX _instance;
            return _instance;
        }

        // Tunings
        StringSetX Tunings{ { 64, 59, 55, 50, 45, 40, 0 }, MAX_STRINGS };

        // フラット化されたコード
        std::array<FlatChordEntry, ChordSpec::flatEntryCount> flatChords{};

        std::array<std::string, (ChordSpec::defaultRootCount + ChordSpec::userRootCount)> RootNames{};
        std::array<std::string, ChordSpec::defaultTypeCount> DefaultTypeNames{};
        std::array<std::string, ChordSpec::userTypeCount> UserTypeNames{};

        //==================================================================
        // flatIndex への変換
        //==================================================================
        inline uint16_t toFlatIndex(int root, int type, int voicing) const
        {
            ChordSpec spec;

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
            ChordSpec spec;

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

                for (int r = 0; r < (int)roots.Size() && r < ChordSpec::defaultRootCount + 1; r++)
                {
                    auto& rootObj = roots[r];
                    RootNames[r] = rootObj["Name"].GetString();

                    auto& types = rootObj["ChordTypes"].GetArray();

                    if (r < ChordSpec::defaultRootCount)
                    {
                        // システムルート
                        for (int t = 0; t < (int)types.Size() && t < ChordSpec::defaultTypeCount; t++)
                        {
                            DefaultTypeNames[t] = types[t]["Name"].GetString();

                            auto& voicings = types[t]["Voicings"].GetArray();
                            for (size_t v = 0; v < voicings.Size() && v < ChordSpec::defaultVoicingCount; v++)
                            {
                                flatChords[toFlatIndex((int)r, (int)t, (int)v)].
                                    generateDisplayName(RootNames[r].c_str(), DefaultTypeNames[t].c_str());
                            }
                        }
                    }
                    else
                    {
                        // ユーザールート（root=12）
                        for (int t = 0; t < (int)types.Size() && t < ChordSpec::userTypeCount; t++)
                        {
                            UserTypeNames[t] = types[t]["Name"].GetString();

                            auto& voicings = types[t]["Voicings"].GetArray();
                            for (size_t v = 0; v < voicings.Size() && v < ChordSpec::userVoicingCount; v++)
                            {
                                flatChords[toFlatIndex((int)r, (int)t, (int)v)].
                                    generateDisplayName(RootNames[r].c_str(), UserTypeNames[t].c_str());
                            }
                        }
                    }
                }
            }
        }
    };

} // namespace MinMax
