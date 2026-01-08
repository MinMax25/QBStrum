//------------------------------------------------------------------------
// ChordMap.h（基準版）
//------------------------------------------------------------------------
#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <codecvt>
#include <ctime>
#include <iomanip>
#include <locale>
#include <sstream>

namespace MinMax
{
    // 7弦まで対応可能とする
    inline constexpr int MAX_STRINGS = 7;

    //======================================================================
    // 弦数定義
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
        // default table
        static constexpr int defaultRootCount = 12;
        static constexpr int defaultTypeCount = 29;
        static constexpr int defaultVoicingCount = 3;

        // user table
        static constexpr int userRootCount = 1;
        static constexpr int userTypeCount = 5;
        static constexpr int userVoicingCount = 24;

        static constexpr int TotalRootCount = defaultRootCount + userRootCount;

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

        std::filesystem::path presetPath{};

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
        StringSetX Tunings{};

        // フラット化されたコード
        std::array<FlatChordEntry, ChordSpec::flatEntryCount> flatChords{};

        std::array<std::string, (ChordSpec::defaultRootCount + ChordSpec::userRootCount)> RootNames{};
        std::array<std::string, ChordSpec::defaultTypeCount> DefaultTypeNames{};
        std::array<std::string, ChordSpec::userTypeCount> UserTypeNames{};

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
        // コードプリセット読み込み
        //==================================================================
        void loadChordPreset(const std::filesystem::path& path)
        {
            ChordSpec spec;

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
                for (size_t i = 0; i < arr.Size() && i < MAX_STRINGS; i++)
                {
                    Tunings.data[(int)i] = arr[(int)i].GetInt();
                }
                Tunings.size = arr.Size();
            }

            // ChordRoots
            int flatIndex = 0;

            if (doc.HasMember("ChordRoots") && doc["ChordRoots"].IsArray())
            {
                auto& roots = doc["ChordRoots"].GetArray();

                for (int r = 0; r < (int)roots.Size(); r++)
                {
                    auto& rootObj = roots[r];
                    RootNames[r] = rootObj["Name"].GetString();

                    auto& types = rootObj["ChordTypes"].GetArray();

                    if (r > spec.TotalRootCount)
                    {
                        throw "ChordMap Format Error";
                    }

                    if (r < spec.TotalRootCount)
                    {
                        for (int t = 0; t < (int)types.Size(); t++)
                        {
                            auto typeName = types[t]["Name"].GetString();
                            if (r < spec.defaultRootCount)
                            {
                                DefaultTypeNames[t] = typeName;
                            }
                            else if (r < spec.TotalRootCount)
                            {
                                UserTypeNames[t] = typeName;
                            }

                            auto& voicings = types[t]["Voicings"].GetArray();
                            for (size_t v = 0; v < voicings.Size(); v++)
                            {
                                auto& fc = flatChords[flatIndex++];
                                fc.root = r;
                                fc.type = t;
                                fc.voicing = (int)v;
                                fc.displayName = RootNames[r] + " " + typeName + " " + "(" + std::to_string(v + 1) + ")";
                            }
                        }
                    }
                }
            }
        }

        //==================================================================
        // コードプリセット書き出し
        //==================================================================
        void saveChordPreset(const std::filesystem::path& path)
        {
            namespace fs = std::filesystem;
            ChordSpec spec;

            fs::path filePath = fs::path(path);

            if (std::filesystem::exists(filePath))
            {
                auto t = std::chrono::system_clock::now();
                auto time = std::chrono::system_clock::to_time_t(t);
                std::tm tm;
#ifdef _WIN32
                localtime_s(&tm, &time);
#else
                localtime_r(&time, &tm);
#endif

                std::ostringstream oss;

                oss << path.stem().string() << "_"
                    << std::put_time(&tm, "%Y%m%d_%H%M%S")
                    << path.extension().string();

                fs::path backupFolder = filePath.parent_path().append("backup");
                fs::path backupFile = backupFolder / oss.str();

                if (!fs::directory_entry(backupFolder).exists())
                {
                    fs::create_directory(backupFolder);
                }

                fs::copy_file(filePath, backupFile, fs::copy_options::overwrite_existing);
            }

            // 2. JSON オブジェクト作成
            rapidjson::Document doc;
            doc.SetObject();
            auto& allocator = doc.GetAllocator();

            // Tunings
            rapidjson::Value notes(rapidjson::kArrayType);
            for (size_t i = 0; i < Tunings.size; ++i)
            {
                notes.PushBack(Tunings.data[i], allocator);
            }
            doc.AddMember("Tunings", notes, allocator);

            //
            for (int flatIndex = 0; flatIndex < spec.flatEntryCount; flatIndex++)
            {
                int root;
                int type;
                int voicing;
                getChordInfo(flatIndex, root, type, voicing);

            }

            // 3. ファイルに書き込み
            std::ofstream ofs(filePath);
            if (!ofs.is_open())
            {
                throw std::runtime_error("Cannot open file for writing: " + filePath.string());
            }

            rapidjson::OStreamWrapper osw(ofs);
            rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
            doc.Accept(writer);

            presetPath.clear();
            presetPath.replace_filename(filePath);
        }

        // 文字列変換 UTF8 -> UTF16
        static inline std::wstring convertUtf8ToUtf16(char const* str)
        {
#pragma warning(suppress : 4996)
            std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
#pragma warning(suppress : 4996)
            return converter.from_bytes(str);
        }
    };
}
