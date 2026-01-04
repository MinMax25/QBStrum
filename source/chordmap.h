#pragma once

#include <array>
#include <chrono>
#include <iomanip>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <sstream>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson//prettywriter.h>

namespace MinMax
{
    using std::string;
    using std::vector;

    inline constexpr int STRING_COUNT = 6;

    struct StringSet
    {
        std::array<int, STRING_COUNT> data{};
        size_t size = 0;
    };

    class ChordMap
    {
    private:

        class FlatChordEntry
        {
        public:

            int root = 0;
            int valueType = 0;
            int pos = 0;

            std::string rootName{};
            std::string typeName{};
            std::string posName{};

            std::string displayName{};
        };

        class Voicing
        {
        public:

            int Id = 0;
            string Name;
            vector<int> Frets;

            bool Deserialize(const rapidjson::Value& v)
            {
                if (!v.IsObject()) return false;

                if (v.HasMember("Id"))
                {
                    Id = v["Id"].GetInt();
                }

                if (v.HasMember("Name"))
                {
                    Name = v["Name"].GetString();
                }

                if (v.HasMember("Frets") && v["Frets"].IsArray())
                {
                    for (auto& f : v["Frets"].GetArray())
                    {
                        Frets.push_back(f.GetInt());
                    }
                }
                return true;
            }
        };

        class ParamChordType
        {
        public:

            int Id = 0;
            string Name;
            vector<Voicing> Voicings;

            bool Deserialize(const rapidjson::Value& v)
            {
                if (!v.IsObject()) return false;

                if (v.HasMember("Id"))
                {
                    Id = v["Id"].GetInt();
                }

                if (v.HasMember("Name"))
                {
                    Name = v["Name"].GetString();
                }

                if (v.HasMember("Voicings") && v["Voicings"].IsArray())
                {
                    for (auto& item : v["Voicings"].GetArray())
                    {
                        Voicing vc;
                        vc.Deserialize(item);
                        Voicings.push_back(vc);
                    }
                }
                return true;
            }
        };

        class ParamChordRoot
        {
        public:

            int Id = 0;
            string Name;
            vector<ParamChordType> ChordTypes;

            bool Deserialize(const rapidjson::Value& v)
            {
                if (!v.IsObject()) return false;

                if (v.HasMember("Id"))
                {
                    Id = v["Id"].GetInt();
                }

                if (v.HasMember("Name"))
                {
                    Name = v["Name"].GetString();
                }

                if (v.HasMember("ChordTypes") && v["ChordTypes"].IsArray())
                {
                    for (auto& item : v["ChordTypes"].GetArray())
                    {
                        ParamChordType c;
                        c.Deserialize(item);
                        ChordTypes.push_back(c);
                    }
                }
                return true;
            }
        };

    public:

        static ChordMap& Instance()
        {
            static ChordMap instance;
            return instance;
        }

        static void initFromPreset(const std::filesystem::path& path)
        {
            //
            // コードマップ定義ファイルを読み込む
            Instance() = LoadPreset(path);
        }

        StringSet getChordVoicing(int chordNumber) const
        {
            //
            // コードボイシングを取得する

            StringSet result{};

            auto& s = getByIndex(chordNumber);
            auto& v = ChordRoots[s.root].ChordTypes[s.valueType].Voicings[s.pos].Frets;

            result.size = STRING_COUNT;

            for (size_t i = 0; i < result.size; i++)
            {
                result.data[i] = v[i];
            }

            return result;
        }

        StringSet getTunings()
        {
            //
            // 開放弦のピッチを取得する

            return Tunings;
        }

        float getPositionAverage(int chordNumber)
        {
            //
            // コードボイシングの平均フレット位置を取得する

            auto& v = getChordVoicing(chordNumber);

            int sum = 0;

            for (size_t i = 0; i < v.size; i++)
            {
                if (v.data[i] < 0) continue;
                sum += v.data[i];
            }

            return sum / float(v.size);
        }

        const FlatChordEntry& getByIndex(int index) const
        {
            //
            // インデックス指定によるコード情報を取得する

            return flatChords.at((index < 0 || index > getFlatCount()) ? 0 : index);
        }

        int getFlatCount() const
        {
            //
            // 登録されているコードの数を取得する

            return static_cast<int>(flatChords.size());
        }

        int getRootCount() const { return (int)ChordRoots.size(); }

        const string& getRootName(int r) const { return ChordRoots[r].Name; }

        int getTypeCount(int r) const
        {
            return (int)ChordRoots[r].ChordTypes.size();
        }

        const string& getTypeName(int r, int t) const
        {
            return ChordRoots[r].ChordTypes[t].Name;
        }

        int getVoicingCount(int r, int t) const
        {
            return (int)ChordRoots[r].ChordTypes[t].Voicings.size();
        }

        const string& getVoicingName(int r, int t, int v) const
        {
            return ChordRoots[r].ChordTypes[t].Voicings[v].Name;
        }

        const StringSet getVoicing(int chordNumber)
        {
            StringSet result{};

            if (chordNumber > getFlatCount())
            {
                result.size = STRING_COUNT;
                for (size_t i = 0; i < result.size; i++)
                {
                    result.data[i] = -1;
                }
                return result;
            }

            auto& f = getByIndex(chordNumber);
            auto& v = ChordRoots[f.root].ChordTypes[f.valueType].Voicings[f.pos].Frets;

            for (int i = 0; i < (int)v.size(); i++)
            {
                result.data[i] = v[i];
                result.size++;
            }

            return result;
        }

        int getFlatIndex(int r, int t, int v) const
        {
            return indexTable[r][t][v];
        }
        
        void setVoicing(int chordNumber, const StringSet& frets)
        {
            auto& f = getByIndex(chordNumber);
            auto& v = ChordRoots[f.root].ChordTypes[f.valueType].Voicings[f.pos].Frets;

            bool changed = false;
            for (size_t i = 0; i < frets.size && i < v.size(); ++i)
            {
                if (v[i] != frets.data[i])
                {
                    v[i] = frets.data[i];
                    changed = true;
                }
            }

            if (changed)
                modified = true;
        }

        bool isModified() const { return modified; }

        // 現在の ChordMap 内容を JSON ファイルに保存
        void saveToFile(std::string path)
        {
            presetPath = std::filesystem::path(path);

            if (!modified) return; // 変更がないならスキップ

            // 1. 元ファイルのバックアップ作成（日時付き）
            if (std::filesystem::exists(presetPath))
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
                oss << presetPath.stem().string() << "_"
                    << std::put_time(&tm, "%Y%m%d_%H%M%S")
                    << presetPath.extension().string();

                std::filesystem::path backupPath = presetPath.parent_path() / oss.str();
                std::filesystem::copy_file(presetPath, backupPath, std::filesystem::copy_options::overwrite_existing);
            }

            // 2. JSON オブジェクト作成
            rapidjson::Document doc;
            doc.SetObject();
            auto& allocator = doc.GetAllocator();

            doc.AddMember("Name", rapidjson::Value(Name.c_str(), allocator), allocator);

            // Tunings
            rapidjson::Value notes(rapidjson::kArrayType);
            for (size_t i = 0; i < Tunings.size; ++i)
            {
                notes.PushBack(Tunings.data[i], allocator);
            }
            doc.AddMember("Notes", notes, allocator);

            // ChordRoots
            rapidjson::Value chordRoots(rapidjson::kArrayType);
            for (auto& r : ChordRoots)
            {
                rapidjson::Value rootObj(rapidjson::kObjectType);
                rootObj.AddMember("Id", r.Id, allocator);
                rootObj.AddMember("Name", rapidjson::Value(r.Name.c_str(), allocator), allocator);

                rapidjson::Value types(rapidjson::kArrayType);
                for (auto& t : r.ChordTypes)
                {
                    rapidjson::Value typeObj(rapidjson::kObjectType);
                    typeObj.AddMember("Id", t.Id, allocator);
                    typeObj.AddMember("Name", rapidjson::Value(t.Name.c_str(), allocator), allocator);

                    rapidjson::Value voicings(rapidjson::kArrayType);
                    for (auto& v : t.Voicings)
                    {
                        rapidjson::Value vObj(rapidjson::kObjectType);
                        vObj.AddMember("Id", v.Id, allocator);
                        vObj.AddMember("Name", rapidjson::Value(v.Name.c_str(), allocator), allocator);

                        rapidjson::Value frets(rapidjson::kArrayType);
                        for (auto f : v.Frets)
                            frets.PushBack(f, allocator);

                        vObj.AddMember("Frets", frets, allocator);
                        voicings.PushBack(vObj, allocator);
                    }
                    typeObj.AddMember("Voicings", voicings, allocator);
                    types.PushBack(typeObj, allocator);
                }
                rootObj.AddMember("ChordTypes", types, allocator);
                chordRoots.PushBack(rootObj, allocator);
            }
            doc.AddMember("ChordRoots", chordRoots, allocator);

            // 3. ファイルに書き込み
            std::ofstream ofs(presetPath);
            if (!ofs.is_open())
            {
                throw std::runtime_error("Cannot open file for writing: " + presetPath.string());
            }

            rapidjson::OStreamWrapper osw(ofs);
            //rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
            rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
            doc.Accept(writer);

            modified = false;
        }

    protected:
        std::filesystem::path presetPath;

        string Name;

        StringSet Tunings{};

        vector<ParamChordRoot> ChordRoots;
        vector<FlatChordEntry> flatChords;
        vector<vector<vector<int>>> indexTable;

        ChordMap() = default;
        ChordMap(const ChordMap&) = delete;
        ChordMap& operator=(const ChordMap&) = delete;
        ChordMap(ChordMap&&) = default;
        ChordMap& operator=(ChordMap&&) = default;

        bool modified = false;

        void buildFlatTable()
        {
            //
            // 階層化されているコードマップを直列に並べ替える

            flatChords.clear();
            indexTable.clear();

            indexTable.resize(ChordRoots.size());

            int index = 0;

            for (int r = 0; r < (int)ChordRoots.size(); ++r)
            {
                indexTable[r].resize(ChordRoots[r].ChordTypes.size());

                for (int t = 0; t < (int)ChordRoots[r].ChordTypes.size(); ++t)
                {
                    auto& types = ChordRoots[r].ChordTypes[t];
                    indexTable[r][t].resize(types.Voicings.size());

                    for (int p = 0; p < (int)types.Voicings.size(); ++p)
                    {
                        FlatChordEntry e;
                        e.root = r;
                        e.valueType = t;
                        e.pos = p;
                        e.rootName = ChordRoots[r].Name;
                        e.typeName = ChordRoots[r].ChordTypes[t].Name;
                        e.posName = ChordRoots[r].ChordTypes[t].Voicings[p].Name;
                        e.displayName =
                            ChordRoots[r].Name + " " + types.Name + " (" + std::to_string(p + 1) + ")";

                        flatChords.push_back(e);
                        indexTable[r][t][p] = index++;
                    }
                }
            }
        }

        bool Deserialize(const rapidjson::Value& v)
        {
            if (!v.IsObject()) return false;

            if (v.HasMember("Name"))
            {
                Name = v["Name"].GetString();
            }

            if (v.HasMember("Notes") && v["Notes"].IsArray())
            {
                Tunings.size = STRING_COUNT;

                size_t i = 0;
                for (auto& item : v["Notes"].GetArray())
                {
                    if (i > Tunings.size) break;
                    Tunings.data[i++] = item.GetInt();
                }
            }

            if (v.HasMember("ChordRoots") && v["ChordRoots"].IsArray())
            {
                for (auto& item : v["ChordRoots"].GetArray())
                {
                    ParamChordRoot r;
                    r.Deserialize(item);
                    ChordRoots.push_back(r);
                }
            }

            return true;
        }

        static ChordMap LoadPreset(std::filesystem::path path)
        {
            std::ifstream ifs(path);
            if (!ifs.is_open())
            {
                throw std::runtime_error("Cannot open file: ");
            }

            rapidjson::IStreamWrapper isw(ifs);
            rapidjson::Document doc;
            doc.ParseStream(isw);

            if (!doc.IsObject())
            {
                throw std::runtime_error("JSON format error in file: ");
            }

            ChordMap map;
            map.Deserialize(doc);
            map.buildFlatTable();

            string presetFileName = path.filename().string();
            size_t dot = presetFileName.find_last_of('.');
            map.Name = presetFileName.substr(0, dot);

            map.presetPath = path;

            return map;
        }
    };
}