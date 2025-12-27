#pragma once

#include <type_traits>

// enum class のラッパー
template <typename E>
class EWR {
public:
    constexpr EWR(E e) : value(e) {}

    // int に変換する疑似メソッド
    constexpr auto toInt() const noexcept {
        return static_cast<std::underlying_type_t<E>>(value);
    }

    // operator int() を使えば暗黙変換も可能
    constexpr operator std::underlying_type_t<E>() const noexcept {
        return toInt();
    }

    // 必要なら値を取得するメソッド
    constexpr E get() const noexcept { return value; }

private:
    E value;
};