#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <numeric>

namespace MinMax
{
    using std::string;
    using std::vector;

    inline constexpr int STRING_COUNT = 6;

    class CChord
    {
    public:

        int lsb = 0;
        int msb = 0;

        std::string toString() const
        {
            return "[Chord lsb:" + std::to_string(lsb) + " msb:" + std::to_string(msb) + "]";
        }
    };

    class ChordMap
    {
    private:

        struct FlatChordEntry
        {
            int root = 0;
            int type = 0;
            int position = 0;

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
            Instance() = LoadPreset(path);
        }


        std::vector<int> getFretPositions(int chordNumber) const
        {
            return { };
        }
        std::vector<int> getTunings()
        {
            return Tunings;
        }

        float getPositionAverage(int chordNumber)
        {
            return 0;
        }

        const FlatChordEntry& getByIndex(int index) const
        {
            return flatChords.at(index);
        }

        int getFlatCount() const
        {
            return static_cast<int>(flatChords.size());
        }

    protected:

        string Name;
        vector<int> Tunings;
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
                        e.position = p;
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
                for (auto& item : v["Notes"].GetArray())
                {
                    Tunings.push_back(item.GetInt());
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