#pragma once

#include <cstdint>
#include <string>

namespace Roguelike {

inline std::string utf8String(const char* s) {
    return std::string(s);
}

#if defined(__cpp_char8_t)
inline std::string utf8String(const char8_t* s) {
    return std::string(reinterpret_cast<const char*>(s));
}
#endif

// ---------------------------------------------------------------------------
// Suit: 트럼프 카드 4가지 속성
// ---------------------------------------------------------------------------
enum class Suit : uint8_t {
    Heart   = 0,  // ♥
    Diamond = 1,  // ♦
    Club    = 2,  // ♣
    Spade   = 3,  // ♠
    None    = 4,  // 무속성 (기본값)
};

// 상성 순환: ♦ > ♥ > ♠ > ♣ > ♦
// 유리: 1.5배 / 불리: 0.75배 / 중립: 1.0배 / 무속성(None): 항상 1.0배
inline float getSuitMultiplier(Suit attacker, Suit defender) noexcept {
    // 무속성이면 상성 없음
    if (attacker == Suit::None || defender == Suit::None) return 1.0f;

    // attacker가 defender에게 강한 경우
    if ((attacker == Suit::Heart   && defender == Suit::Spade)   ||
        (attacker == Suit::Spade   && defender == Suit::Club)    ||
        (attacker == Suit::Club    && defender == Suit::Diamond)  ||
        (attacker == Suit::Diamond && defender == Suit::Heart)) {
        return 1.5f;
    }
    // attacker가 defender에게 약한 경우 (역방향)
    if ((attacker == Suit::Spade   && defender == Suit::Heart)   ||
        (attacker == Suit::Club    && defender == Suit::Spade)   ||
        (attacker == Suit::Diamond && defender == Suit::Club)    ||
        (attacker == Suit::Heart   && defender == Suit::Diamond)) {
        return 0.75f;
    }
    // 같은 속성이거나 무관한 관계: 중립
    return 1.0f;
}

inline std::string suitToString(Suit s) noexcept {
    switch (s) {
        case Suit::Heart:   return utf8String(u8"♥ Heart");
        case Suit::Diamond: return utf8String(u8"♦ Diamond");
        case Suit::Club:    return utf8String(u8"♣ Club");
        case Suit::Spade:   return utf8String(u8"♠ Spade");
        case Suit::None:    return utf8String(u8"∅ None");
        default:            return "?";
    }
}

inline char suitToChar(Suit s) noexcept {
    switch (s) {
        case Suit::Heart:   return 'H';
        case Suit::Diamond: return 'D';
        case Suit::Club:    return 'C';
        case Suit::Spade:   return 'S';
        case Suit::None:    return 'N';
        default:            return '?';
    }
}

inline std::string suitToSymbol(Suit s) noexcept {
    switch (s) {
        case Suit::Heart:   return utf8String(u8"♥");
        case Suit::Diamond: return utf8String(u8"♦");
        case Suit::Club:    return utf8String(u8"♣");
        case Suit::Spade:   return utf8String(u8"♠");
        default:            return "?";
    }
}

} // namespace Roguelike
