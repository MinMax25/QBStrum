//------------------------------------------------------------------------
// ChordMap.h（基準版）
//------------------------------------------------------------------------
#pragma once


#include <array>
#include <chrono>
#include <codecvt>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <rapidjson//prettywriter.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/rapidjson.h>
#include <sstream>
#include <stdexcept>
#include <string>

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

    inline constexpr int STRING_COUNT = (int)StringCount::Guitar6;

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

        static constexpr int defaultBlockSize = defaultRootCount * defaultTypeCount * defaultVoicingCount;
    };

    struct StringSet
    {
        // fret position
        std::array<int, MAX_STRINGS> data{};

        // valid strings
        size_t size = 0;
    };

    //======================================================================
    // ChordMap
    //======================================================================
    class ChordMap
    {
    private:
        struct FlatChordEntry
        {
            int root = 0;
            int type = 0;
            int voicing = 0;

            std::array<int, MAX_STRINGS> fretPosition{};

            std::string displayName;

            void generateDisplayName(const char* rootName, const char* typeName)
            {
                displayName = std::string(rootName) + " " + typeName + " (" + std::to_string(voicing + 1) + ")";
            }
        };

        // コンストラクターは private にして直接生成を禁止
        ChordMap() = default;

        // コピー禁止
        ChordMap(const ChordMap&) = delete;
        ChordMap& operator=(const ChordMap&) = delete;

        // ムーブ禁止
        ChordMap(ChordMap&&) = delete;
        ChordMap& operator=(ChordMap&&) = delete;

        std::filesystem::path presetPath{};

    public:
        //==================================================================
        // シングルトン
        //==================================================================
        static ChordMap& instance()
        {
            static ChordMap _instance;
            return _instance;
        }

        // Tunings
        StringSet Tunings{};

        // フラット化されたコード
        std::array<FlatChordEntry, ChordSpec::flatEntryCount> flatChords{};

        std::array<std::string, (ChordSpec::defaultRootCount + ChordSpec::userRootCount)> RootNames{};
        std::array<std::string, ChordSpec::defaultTypeCount> DefaultTypeNames{};
        std::array<std::string, ChordSpec::userTypeCount> UserTypeNames{};

        StringSet getTunings() const 
        {
            return Tunings;
        }

        StringSet getChordVoicing(int flatIndex) const
        {
            StringSet result{};

            auto& v = flatChords[flatIndex];

            result.size = (int)StringCount::Guitar6;

            for (size_t i = 0; i < result.size; i++)
            {
                result.data[i] = v.fretPosition[i];
            }

            return result;
        }

        ChordSpec spec;

        float getPositionAverage(int chordNumber) const
        {
            //
            // コードボイシングの平均フレット位置を取得する

            auto& v = getChordVoicing(chordNumber);

            int sum = 0;
            int count = 0;

            for (size_t i = 0; i < v.size; i++)
            {
                if (v.data[i] < 0) continue;
                sum += v.data[i];
                ++count;
            }

            return count > 0 ? sum / float(count) : 0.0f;
        }
  
        const int getFlatCount() const
        {
            return spec.flatEntryCount;
        }

        const FlatChordEntry& getByIndex(int flatIndex) const
        {
            return flatChords[flatIndex];
        }

        const int getRootCount() const 
        {
            return (int)spec.TotalRootCount;
        }

        const std::string& getRootName(int r) const
        {
            return RootNames[r];
        }

        bool isDefault(int r) const
        {
            return r < spec.defaultRootCount;
        }

        int getTypeCount(int r) const
        {
            return isDefault(r) ? spec.defaultTypeCount : spec.userTypeCount;
        }

        std::string& getTypeName(int r, int t)
        {
            return isDefault(r) ? DefaultTypeNames[t] : UserTypeNames[t];
        }

        int getVoicingCount(int r, int t) const
        {
            return isDefault(r) ? spec.defaultVoicingCount : spec.userVoicingCount;
        }

        void setVoicing(int flatIndex, StringSet& frets)
        {
            auto& v = flatChords[flatIndex].fretPosition;

            for (size_t i = 0; i < frets.size && i < v.size(); ++i)
            {
                if (v[i] != frets.data[i])
                {
                    v[i] = frets.data[i];
                }
            }
        }

        std::string getPresetName()
        {
            return presetPath.stem().u8string();
        }

        std::filesystem::path getPresetPath()
        {
            return presetPath;
        }

        //==================================================================
        // コードプリセット読み込み
        //==================================================================
        void loadChordPreset(const std::filesystem::path& path)
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

                                int p = 0;
                                for (auto& fp : voicings[(int)v]["FretPosition"].GetArray())
                                {
                                    fc.fretPosition[p] = fp.GetInt();
                                    p++;
                                }
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
            int flatIndex = 0;
            rapidjson::Value chordRoots(rapidjson::kArrayType);
            for (int r = 0; r < spec.TotalRootCount; r++)
            {
                bool isDefault = r < spec.defaultRootCount;
                
                rapidjson::Value rootObj(rapidjson::kObjectType);
                rootObj.AddMember("Name", rapidjson::Value(RootNames[r].c_str(), allocator), allocator);

                rapidjson::Value types(rapidjson::kArrayType);
                int tmax = isDefault ? spec.defaultTypeCount : spec.userTypeCount;
                for (int t = 0; t < tmax; t++)
                {
                    auto typeName = isDefault ? DefaultTypeNames[t] : UserTypeNames[t];
                    rapidjson::Value typeObj(rapidjson::kObjectType);
                    typeObj.AddMember("Name", rapidjson::Value(typeName.c_str(), allocator), allocator);

                    rapidjson::Value voicings(rapidjson::kArrayType);
                    int vmax = isDefault ? spec.defaultVoicingCount : spec.userVoicingCount;
                    for (int v = 0; v < vmax; v++)
                    {
                        rapidjson::Value vObj(rapidjson::kObjectType);
                        rapidjson::Value frets(rapidjson::kArrayType);
                        for (auto f : flatChords[flatIndex].fretPosition)
                        {
                            frets.PushBack(f, allocator);
                        }
                        vObj.AddMember("FretPosition", frets, allocator);
                        voicings.PushBack(vObj, allocator);

                        flatIndex++;
                    }
                    typeObj.AddMember("Voicings", voicings, allocator);
                    types.PushBack(typeObj, allocator);
                }
                rootObj.AddMember("ChordTypes", types, allocator);
                chordRoots.PushBack(rootObj, allocator);
            }
            doc.AddMember("ChordRoots", chordRoots, allocator);

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
