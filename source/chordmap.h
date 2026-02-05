#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <codecvt>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <sstream>
#include <string>

#include "files.h"

namespace MinMax
{
    inline constexpr int STRING_COUNT = 6;

    struct ChordSpec
    {
        static constexpr int defaultRootCount = 12;
        static constexpr int defaultTypeCount = 29;
        static constexpr int defaultVoicingCount = 3;

        static constexpr int userRootCount = 1;
        static constexpr int userTypeCount = 5;
        static constexpr int userVoicingCount = 24;

        static constexpr int TotalRootCount = defaultRootCount + userRootCount;

        static constexpr int flatEntryCount =
            (defaultRootCount * defaultTypeCount * defaultVoicingCount) +
            (userRootCount * userTypeCount * userVoicingCount);
    };

    struct StringSet
    {
        static const int CENTER_OFFSET = 6;

        int flatIndex = 0;
        uint32_t state = 0;
        std::array<int, STRING_COUNT> data{};

        int getOffset(int stringNumber) const
        {
            return offset[stringNumber];
        }

        void setOffset(int stringNumber, int value)
        {
            offset[stringNumber] = value;
        }

    protected:
        std::array<int, STRING_COUNT> offset{};
    };

    class ChordMap
    {
    private:
        struct FlatChordEntry
        {
            int root = 0;
            int type = 0;
            int voicing = 0;
            std::array<int, STRING_COUNT> fretPosition{};
            std::string displayName;
        };

        ChordMap() = default;
        ~ChordMap() = default;
        ChordMap(const ChordMap&) = delete;
        ChordMap& operator=(const ChordMap&) = delete;

        std::filesystem::path presetPath{};
        StringSet tunings{};

        std::array<FlatChordEntry, ChordSpec::flatEntryCount> flatChords{};
        std::array<std::string, ChordSpec::TotalRootCount> rootNames{};
        std::array<std::string, ChordSpec::defaultTypeCount> defaultTypeNames{};
        std::array<std::string, ChordSpec::userTypeCount> userTypeNames{};

    public:
        static ChordMap& instance()
        {
            static ChordMap _instance;
            return _instance;
        }

        StringSet getTunings() const { return tunings; }

        StringSet getChordVoicing(int flatIndex) const
        {
            if (flatIndex < 0 || flatIndex >= (int)flatChords.size())
            {
                return {};
            }

            const auto& v = flatChords[flatIndex];
            StringSet result{};
            result.flatIndex = flatIndex;

            for (int i = 0; i < STRING_COUNT; ++i)
            {
                result.data[i] = v.fretPosition[i];
                result.setOffset(i, 0);
            }
            return result;
        }

        int getFlatCount() const { return ChordSpec::flatEntryCount; }

        const FlatChordEntry& getChordInfoByIndex(int flatIndex) const { return flatChords[flatIndex]; }

        int getRootCount() const { return ChordSpec::TotalRootCount; }

        const std::string& getRootName(int r) const { return rootNames[r]; }

        bool isDefaultRoot(int r) const { return r < ChordSpec::defaultRootCount; }

        void loadChordPreset()
        {
            loadChordPreset(Files::getPresetPath().append(Files::DEFAULT_PRESET));
        }

        void loadChordPreset(const std::filesystem::path& path)
        {
            std::ifstream ifs(path);
            if (!ifs.is_open())
            {
                presetPath = "## Not Found ##";
                return;
            }

            rapidjson::IStreamWrapper isw(ifs);
            rapidjson::Document doc;
            if (doc.ParseStream(isw).HasParseError() || !doc.IsObject())
            {
                presetPath = "## Parse Error ##";
                return;
            }

            if (doc.HasMember("Tunings") && doc["Tunings"].IsArray())
            {
                const auto& arr = doc["Tunings"].GetArray();
                for (size_t i = 0; i < STRING_COUNT; ++i)
                {
                    tunings.data[i] = arr[(int)i].GetInt();
                }
            }

            if (doc.HasMember("ChordRoots") && doc["ChordRoots"].IsArray())
            {
                const auto& roots = doc["ChordRoots"].GetArray();
                int flatIdx = 0;

                for (int r = 0; r < (int)roots.Size() && r < ChordSpec::TotalRootCount; ++r)
                {
                    rootNames[r] = roots[r]["Name"].GetString();
                    const auto& types = roots[r]["ChordTypes"].GetArray();

                    for (int t = 0; t < (int)types.Size(); ++t)
                    {
                        std::string typeName = types[t]["Name"].GetString();
                        if (isDefaultRoot(r)) defaultTypeNames[t] = typeName;
                        else if (t < ChordSpec::userTypeCount) userTypeNames[t] = typeName;

                        const auto& voicings = types[t]["Voicings"].GetArray();
                        for (int v = 0; v < (int)voicings.Size() && flatIdx < ChordSpec::flatEntryCount; ++v)
                        {
                            auto& fc = flatChords[flatIdx++];
                            fc.root = r;
                            fc.type = t;
                            fc.voicing = v;
                            fc.displayName = rootNames[r] + " " + typeName + " (" + std::to_string(v + 1) + ")";

                            const auto& fretsArr = voicings[v]["FretPosition"].GetArray();
                            for (int p = 0; p < (int)fretsArr.Size() && p < STRING_COUNT; ++p)
                            {
                                fc.fretPosition[p] = fretsArr[p].GetInt();
                            }
                        }
                    }
                }
            }

            presetPath = path;
        }

        void saveChordPreset(const std::filesystem::path& path)
        {
            namespace fs = std::filesystem;
            fs::path filePath = path;
            filePath.replace_extension(std::string(Files::FILE_EXT));

            if (fs::exists(filePath))
            {
                createBackup(filePath);
            }

            rapidjson::Document doc;
            doc.SetObject();
            auto& allocator = doc.GetAllocator();

            rapidjson::Value tuningsArr(rapidjson::kArrayType);
            for (size_t i = 0; i < STRING_COUNT; ++i)
            {
                tuningsArr.PushBack(tunings.data[i], allocator);
            }
            doc.AddMember("Tunings", tuningsArr, allocator);

            int flatIdx = 0;
            rapidjson::Value rootList(rapidjson::kArrayType);

            for (int r = 0; r < ChordSpec::TotalRootCount; ++r)
            {
                rapidjson::Value rootObj(rapidjson::kObjectType);
                rootObj.AddMember("Name", rapidjson::Value(rootNames[r].c_str(), allocator), allocator);

                rapidjson::Value typeList(rapidjson::kArrayType);
                int tMax = isDefaultRoot(r) ? ChordSpec::defaultTypeCount : ChordSpec::userTypeCount;

                for (int t = 0; t < tMax; ++t)
                {
                    rapidjson::Value typeObj(rapidjson::kObjectType);
                    typeObj.AddMember("Name", rapidjson::Value(getTypeName(r, t).c_str(), allocator), allocator);

                    rapidjson::Value voicingList(rapidjson::kArrayType);
                    int vMax = isDefaultRoot(r) ? ChordSpec::defaultVoicingCount : ChordSpec::userVoicingCount;

                    for (int v = 0; v < vMax; ++v)
                    {
                        rapidjson::Value vObj(rapidjson::kObjectType);
                        rapidjson::Value fArr(rapidjson::kArrayType);
                        for (int f : flatChords[flatIdx].fretPosition)
                        {
                            fArr.PushBack(f, allocator);
                        }
                        vObj.AddMember("FretPosition", fArr, allocator);
                        voicingList.PushBack(vObj, allocator);
                        flatIdx++;
                    }
                    typeObj.AddMember("Voicings", voicingList, allocator);
                    typeList.PushBack(typeObj, allocator);
                }
                rootObj.AddMember("ChordTypes", typeList, allocator);
                rootList.PushBack(rootObj, allocator);
            }
            doc.AddMember("ChordRoots", rootList, allocator);

            std::ofstream ofs(filePath);
            if (ofs.is_open())
            {
                rapidjson::OStreamWrapper osw(ofs);
                rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
                doc.Accept(writer);
                presetPath = filePath;
            }
        }

        float getPositionAverage(int flatIndex) const
        {
            StringSet v = getChordVoicing(flatIndex);
            int sum = 0;
            int count = 0;

            for (size_t i = 0; i < STRING_COUNT; i++)
            {
                if (v.data[i] < 0) continue;
                sum += v.data[i];
                ++count;
            }

            return (count > 0) ? static_cast<float>(sum) / count : 0.0f;
        }

        void setVoicing(int flatIndex, const StringSet& frets)
        {
            if (flatIndex < 0 || flatIndex >= ChordSpec::flatEntryCount) return;

            auto& v = flatChords[flatIndex].fretPosition;
            for (size_t i = 0; i < STRING_COUNT; ++i)
            {
                v[i] = frets.data[i];
            }
        }

        static inline std::wstring convertUtf8ToUtf16(char const* str)
        {
#pragma warning(suppress : 4996)
            std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
            return converter.from_bytes(str);
        }

        std::string getPresetName() const
        {
            return presetPath.stem().u8string();
        }

        std::filesystem::path getPresetPath() const
        {
            return presetPath;
        }

        int getTypeCount(int r) const
        {
            return isDefault(r) ? ChordSpec::defaultTypeCount : ChordSpec::userTypeCount;
        }

        bool isDefault(int r) const
        {
            return r < ChordSpec::defaultRootCount;
        }

        std::string& getTypeName(int r, int t)
        {
            return isDefault(r) ? defaultTypeNames[t] : userTypeNames[t];
        }

        int getVoicingCount(int r, int t) const
        {
            return isDefaultRoot(r) ? ChordSpec::defaultVoicingCount : ChordSpec::userVoicingCount;
        }

        std::string& getTypeNameRef(int r, int t)
        {
            return isDefaultRoot(r) ? defaultTypeNames[t] : userTypeNames[t];
        }

        void restoreStringSet(StringSet& target, const StringSet& source)
        {
            target = source;
        }

        void saveStringSet(const StringSet& source, StringSet& backup)
        {
            backup = source;
        }

    private:
        void createBackup(const std::filesystem::path& filePath)
        {
            namespace fs = std::filesystem;
            auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::tm tm;
#ifdef _WIN32
            localtime_s(&tm, &now);
#else
            localtime_r(&now, &tm);
#endif
            std::ostringstream oss;
            oss << filePath.stem().string() << "_"
                << std::put_time(&tm, "%Y%m%d_%H%M%S")
                << filePath.extension().string();

            fs::path backupDir = filePath.parent_path() / "backup";
            if (!fs::exists(backupDir)) fs::create_directory(backupDir);
            fs::copy_file(filePath, backupDir / oss.str(), fs::copy_options::overwrite_existing);
        }
    };
}