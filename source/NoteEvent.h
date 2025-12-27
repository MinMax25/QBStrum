#pragma once

#include <cstdint>

namespace MinMax
{
    enum class NoteEventType : uint8_t
    {
        On,
        Off,
    };

    // NoteEvent:
    //   Scheduler から Processor へ渡される
    //   送信用の MIDI ノートイベント。
    //   状態やロジックは持たない。
    class NoteEvent
    {
    public:
        bool on;    // debug 削除候補
        int noteid; // debug 削除候補

        NoteEventType eventType;

        int channel;
        int noteId;

        // 現在の ProcessBlock 先頭からのサンプルオフセット
        int sampleOffset;

        int pitch;

        // 0.0f – 1.0f 正規化ベロシティ
        float velocity;
    };
}
