#pragma once
#pragma once

#include <cstdint>
#include <string>

// 일부 Windows/외부 헤더가 None을 매크로로 정의해 enum class Suit::None을 깨뜨리는 경우 방지
#ifdef None
#undef None
#endif

namespace Roguelike {

    // ---------------------------------------------------------------------------
    // Suit: 트럼프 카드 4가지 속성
    // ---------------------------------------------------------------------------
    enum class Suit : uint8_t {
        Heart = 0,    // H
        Diamond = 1,    // D
        Club = 2,    // C
        Spade = 3,    // S
        None = 4,    // 무속성
    };

    // 상성 순환: H > D > C > S > H
    // 유리: 1.5배 / 불리: 0.75배 / 중립: 1.0배 / 무속성(None): 항상 1.0배
    inline float getSuitMultiplier(Suit attacker, Suit defender) noexcept {
        if (attacker == Suit::None || defender == Suit::None) return 1.0f;

        // attacker가 defender에게 강한 경우
        if ((attacker == Suit::Heart && defender == Suit::Diamond) ||
            (attacker == Suit::Diamond && defender == Suit::Club) ||
            (attacker == Suit::Club && defender == Suit::Spade) ||
            (attacker == Suit::Spade && defender == Suit::Heart))
        {
            return 1.5f;
        }

        // attacker가 defender에게 약한 경우
        if ((attacker == Suit::Diamond && defender == Suit::Heart) ||
            (attacker == Suit::Club && defender == Suit::Diamond) ||
            (attacker == Suit::Spade && defender == Suit::Club) ||
            (attacker == Suit::Heart && defender == Suit::Spade))
        {
            return 0.75f;
        }

        return 1.0f;
    }

    inline std::string suitToString(Suit s) noexcept {
        switch (s) {
        case Suit::Heart:   return "H Heart";
        case Suit::Diamond: return "D Diamond";
        case Suit::Club:    return "C Club";
        case Suit::Spade:   return "S Spade";
        case Suit::None:    return "N None";
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
        case Suit::Heart:   return "H";
        case Suit::Diamond: return "D";
        case Suit::Club:    return "C";
        case Suit::Spade:   return "S";
        case Suit::None:    return "N";
        default:            return "?";
        }
    }

} // namespace Roguelike