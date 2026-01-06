#pragma once

#include <array>
#include <cstdio>
#include <pluginterfaces/base/ftypes.h>
#include <sal.h>
#include <string>

namespace MinMax
{
    class ScheduledNote
    {
    public:
        bool valid = false;

        bool isSendNoteOn = false;

        Steinberg::uint64 onTime = 0;
        Steinberg::uint64 offTime = 0;

        int pitch = 0;
        int noteId = 0;
        int channel = 0;

        float velocity = 0.0f;

        std::string toString() const
        {
            char buf[256];
            std::snprintf(
                buf, sizeof(buf),
                "ScheduledNote{on=%llu off=%llu pitch=%d id=%d sent=%d vel=%.2f}",
                onTime, offTime, pitch, noteId, isSendNoteOn, velocity
            );
            return buf;
        }
    };

    // ScheduledNote を onTime 昇順で保持する固定長コンテナ。
    class TimeQueue
    {
    public:
        static constexpr size_t Capacity = 64;

        bool empty() const noexcept
        {
            return count_ == 0;
        }

        size_t size() const noexcept
        {
            return count_;
        }

        bool full() const noexcept
        {
            return count_ >= Capacity;
        }

        void clear() noexcept
        {
            count_ = 0;
        }

        ScheduledNote& current() noexcept
        {
            return buf_[0];
        }

        const ScheduledNote& current() const noexcept
        {
            return buf_[0];
        }

        // --- 追加（onTime 昇順） ---
        bool push(const ScheduledNote& note) noexcept
        {
            if (full()) return false;

            size_t i = count_;
            while (i > 0 && buf_[i - 1].onTime > note.onTime)
            {
                buf_[i] = buf_[i - 1]; // shift
                --i;
            }

            __analysis_assume(i < Capacity);
            buf_[i] = note;
            ++count_;
            return true;
        }

        // --- 時間(onTime)を指定して要素を取得 ---
        ScheduledNote* findNoteWithOnTime(Steinberg::uint64 onTime)
        {
            for (size_t i = 0; i < count_; ++i)
            {
                if (buf_[i].onTime == onTime)
                    return &buf_[i];
            }
            return nullptr;
        }

        // --- 現在要素を削除 ---
        void eraseCurrent() noexcept
        {
            if (count_ == 0) return;

            for (size_t i = 1; i < count_; ++i)
            {
                buf_[i - 1] = buf_[i]; // shift
            }
            --count_;
        }

        // --- 指定した onTime より「前」にある最後のノートを取得 ---
        // onTime より小さい onTime を持つ要素のうち、
        // 最も後ろ（時間的に直前）の ScheduledNote を返す。
        // 存在しない場合は nullptr を返す。
        ScheduledNote* findLastBefore(Steinberg::uint64 onTime)
        {
            ScheduledNote* result = nullptr;

            for (size_t i = 0; i < count_; ++i)
            {
                if (buf_[i].onTime < onTime)
                    result = &buf_[i];
                else
                    break;
            }
            return result;
        }

    private:

        std::array<ScheduledNote, Capacity> buf_{};
        
        size_t count_ = 0;
    };
}