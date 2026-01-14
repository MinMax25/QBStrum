#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace MinMax
{
    class MidiWriter
    {
    public:
        MidiWriter(int tpq = 480)
            : ticksPerQuarter(tpq)
        {
        }

        // ノートイベントを追加
        void addNote(uint8_t note, uint8_t velocity, uint32_t startTick, uint32_t duration)
        {
            notes.push_back({ note, velocity, startTick, duration });
        }

        // MIDI ファイルを書き出す
        bool write(const std::wstring& filename)
        {
            std::ofstream ofs(filename, std::ios::binary);
            if (!ofs) return false;

            // --- MThd ヘッダ ---
            writeString(ofs, "MThd");
            writeBE32(ofs, 6);
            writeBE16(ofs, 0);
            writeBE16(ofs, 1);
            writeBE16(ofs, ticksPerQuarter);

            // --- トラックデータ ---
            std::vector<uint8_t> trackData;

            // ノートイベントをオン／オフに分解して時間順に並べる
            struct MidiEvent { uint32_t tick; uint8_t status; uint8_t note; uint8_t vel; };
            std::vector<MidiEvent> events;
            for (auto& n : notes)
            {
                events.push_back({ n.startTick, 0x90, n.note, n.velocity });
                events.push_back({ n.startTick + n.duration, 0x80, n.note, 0 });
            }
            std::sort(events.begin(), events.end(), [](const MidiEvent& a, const MidiEvent& b) { return a.tick < b.tick; });

            uint32_t lastTick = 0;
            for (auto& e : events)
            {
                writeDelta(trackData, e.tick - lastTick);
                trackData.push_back(e.status);
                trackData.push_back(e.note);
                trackData.push_back(e.vel);
                lastTick = e.tick;
            }

            // 終了イベント
            writeDelta(trackData, 0);
            trackData.push_back(0xFF);
            trackData.push_back(0x2F);
            trackData.push_back(0x00);

            // トラックチャンク
            writeString(ofs, "MTrk");
            writeBE32(ofs, static_cast<uint32_t>(trackData.size()));
            ofs.write(reinterpret_cast<const char*>(trackData.data()), trackData.size());

            return true;
        }

    private:
        struct NoteEvent
        {
            uint8_t note;
            uint8_t velocity;
            uint32_t startTick;
            uint32_t duration;
        };

        std::vector<NoteEvent> notes;
        int ticksPerQuarter;

        // --- 書き込みユーティリティ ---
        static void writeString(std::ofstream& ofs, const std::string& s)
        {
            ofs.write(s.c_str(), s.size());
        }

        static void writeBE16(std::ofstream& ofs, uint16_t v)
        {
            uint8_t buf[2] = { uint8_t((v >> 8) & 0xFF), uint8_t(v & 0xFF) };
            ofs.write(reinterpret_cast<const char*>(buf), 2);
        }

        static void writeBE32(std::ofstream& ofs, uint32_t v)
        {
            uint8_t buf[4] = {
                uint8_t((v >> 24) & 0xFF),
                uint8_t((v >> 16) & 0xFF),
                uint8_t((v >> 8) & 0xFF),
                uint8_t(v & 0xFF)
            };
            ofs.write(reinterpret_cast<const char*>(buf), 4);
        }

        // 可変長 quantity
        static void writeDelta(std::vector<uint8_t>& buf, uint32_t delta)
        {
            std::vector<uint8_t> tmp;

            // 下位7ビットずつ取り出して逆順に入れる
            tmp.push_back(delta & 0x7F);
            while ((delta >>= 7) > 0)
            {
                tmp.push_back(0x80 | (delta & 0x7F));
            }

            // 逆順に buf へ追加
            for (auto it = tmp.rbegin(); it != tmp.rend(); ++it)
                buf.push_back(*it);
        }
    };
}
