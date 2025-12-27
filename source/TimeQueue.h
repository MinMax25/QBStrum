#pragma once

#include <array>
#include <Windows.h>

#include "schedulednote.h"

namespace MinMax
{
    // TimeQueue:
    //   ScheduledNote を onTime 昇順で保持する固定長コンテナ。
    //   ※前提：各 TimeQueue はモノフォニック（同時に1音のみ）
    //   - 複数 NoteOn が同じ sampleOffset に存在することはない
    //   - 同一弦内での競合や補正は addNoteOn で解決済み

    class TimeQueue
    {
    public:
        static constexpr size_t Capacity = 64;

        // --- 状態 ---
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

        // --- アクセス ---
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
        ScheduledNote* findNoteWithOnTime(uint64 onTime)
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
        ScheduledNote* findLastBefore(uint64 onTime)
        {
            ScheduledNote* result = nullptr;

            for (size_t i = 0; i < count_; ++i)
            {
                if (buf_[i].onTime < onTime)
                    result = &buf_[i];
                else
                    break; // onTime 昇順なので、以降は不要
            }
            return result;
        }

        std::string toString() const
        {
            std::string s;
            s += "TimeQueue[count=" + std::to_string(count_) + "]\n";
            for (size_t i = 0; i < count_; ++i)
            {
                s += "  ";
                s += buf_[i].toString();
                s += "\n";
            }
            return s;
        }

    private:
        std::array<ScheduledNote, Capacity> buf_{};
        size_t count_ = 0;
    };

}