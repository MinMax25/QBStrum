#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <array>

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
            int type = 0;
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
            auto& v = ChordRoots[s.root].ChordTypes[s.type].Voicings[s.pos].Frets;

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

            auto& f = getByIndex(chordNumber);
            auto& v = ChordRoots[f.root].ChordTypes[f.type].Voicings[f.pos].Frets;

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

    protected:

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
                        e.type = t;
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

            string filename = path.filename().string();
            size_t dot = filename.find_last_of('.');
            map.Name = filename.substr(0, dot);

            return map;
        }
    };
}