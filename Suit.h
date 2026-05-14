#pragma once

#include <cstdint>
#include <string>

namespace Roguelike {

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
        case Suit::Heart:   return "♥ Heart";
        case Suit::Diamond: return "♦ Diamond";
        case Suit::Club:    return "♣ Club";
        case Suit::Spade:   return "♠ Spade";
        case Suit::None:    return "∅ None";
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
        case Suit::Heart:   return "♥";
        case Suit::Diamond: return "♦";
        case Suit::Club:    return "♣";
        case Suit::Spade:   return "♠";
        default:            return "?";
    }
}

} // namespace Roguelike
