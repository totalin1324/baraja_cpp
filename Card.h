#pragma once

#include "Suit.h"
#include <string>
#include <vector>
#include <algorithm>
#include <random>

namespace Roguelike {

// ---------------------------------------------------------------------------
// CardRank
// ---------------------------------------------------------------------------
enum class CardRank : uint8_t {
    Two=2, Three=3, Four=4, Five=5, Six=6, Seven=7, Eight=8,
    Nine=9, Ten=10, Jack=11, Queen=12, King=13, Ace=14
};

inline std::string rankToString(CardRank r) {
    switch (r) {
        case CardRank::Jack:  return "J";
        case CardRank::Queen: return "Q";
        case CardRank::King:  return "K";
        case CardRank::Ace:   return "A";
        default: return std::to_string(static_cast<int>(r));
    }
}

// ---------------------------------------------------------------------------
// CardEffect
// ---------------------------------------------------------------------------
enum class CardEffect { Attack, Defend, Heal, DrawCard, DoubleAtk };

inline std::string effectToString(CardEffect e) {
    switch (e) {
        case CardEffect::Attack:    return "공격";
        case CardEffect::Defend:    return "방어";
        case CardEffect::Heal:      return "회복";
        case CardEffect::DrawCard:  return "드로우";
        case CardEffect::DoubleAtk: return "강타";
        default: return "?";
    }
}

// ---------------------------------------------------------------------------
// Card
// ---------------------------------------------------------------------------
struct Card {
    Suit       suit;
    CardRank   rank;
    CardEffect effect;
    int        value;
    bool       isPermanent = false; // true: 고정 슬롯 카드 (사용 후 소모 X)

    std::string toString() const {
        std::string s = suitToSymbol(suit) + rankToString(rank)
                      + "[" + effectToString(effect) + " " + std::to_string(value) + "]";
        if (isPermanent) s += "(고)";
        return s;
    }
};

// ---------------------------------------------------------------------------
// PokerHand
// ---------------------------------------------------------------------------
enum class PokerHand {
    None=0, OnePair=1, TwoPair=2, ThreeOfAKind=3,
    Straight=4, Flush=5, FullHouse=6, FourOfAKind=7,
    StraightFlush=8, RoyalFlush=9
};

inline std::string pokerHandToString(PokerHand h) {
    switch (h) {
        case PokerHand::None:          return "없음";
        case PokerHand::OnePair:       return "원 페어";
        case PokerHand::TwoPair:       return "투 페어";
        case PokerHand::ThreeOfAKind:  return "트리플";
        case PokerHand::Straight:      return "스트레이트";
        case PokerHand::Flush:         return "플러시";
        case PokerHand::FullHouse:     return "풀하우스";
        case PokerHand::FourOfAKind:   return "포카드";
        case PokerHand::StraightFlush: return "스트레이트 플러시";
        case PokerHand::RoyalFlush:    return "로얄 플러시";
        default: return "?";
    }
}

inline float pokerHandMultiplier(PokerHand h) {
    switch (h) {
        case PokerHand::None:          return 1.0f;
        case PokerHand::OnePair:       return 1.2f;
        case PokerHand::TwoPair:       return 1.4f;
        case PokerHand::ThreeOfAKind:  return 1.6f;
        case PokerHand::Straight:      return 1.8f;
        case PokerHand::Flush:         return 2.0f;
        case PokerHand::FullHouse:     return 2.3f;
        case PokerHand::FourOfAKind:   return 2.8f;
        case PokerHand::StraightFlush: return 3.5f;
        case PokerHand::RoyalFlush:    return 5.0f;
        default: return 1.0f;
    }
}

// ---------------------------------------------------------------------------
// evaluateHand
// 규칙:
//   플러시/스트레이트/스트레이트플러시/로얄플러시 → 최소 5장 필요
//   포카드/풀하우스/트리플/투페어/원페어       → 2장 이상
//   플러시는 중복 랭크 없이 전부 다른 랭크여야 성립 (페어+같은문양 → 페어 우선)
// ---------------------------------------------------------------------------
inline PokerHand evaluateHand(const std::vector<Card>& cards) {
    const int n = static_cast<int>(cards.size());
    if (n < 2) return PokerHand::None;

    // ── 랭크 빈도 ──
    int freq[15] = {};
    for (const auto& c : cards) freq[static_cast<int>(c.rank)]++;

    std::vector<int> counts;
    for (int i = 2; i <= 14; ++i)
        if (freq[i] > 0) counts.push_back(freq[i]);
    std::sort(counts.rbegin(), counts.rend());

    // 중복 랭크가 있으면 스트레이트/플러시 계열 불가
    bool allUnique = (counts.empty() || counts[0] == 1);

    // ── 플러시: 5장 이상 + 전부 같은 문양 + 모든 랭크 다름 ──
    bool flush = false;
    if (n >= 5 && allUnique) {
        flush = std::all_of(cards.begin(), cards.end(),
            [&](const Card& c){ return c.suit == cards[0].suit; });
    }

    // ── 스트레이트: 5장 이상 + 중복 없음 + 연속 ──
    bool straight = false, royalStraight = false;
    if (n >= 5 && allUnique) {
        std::vector<int> ranks;
        ranks.reserve(n);
        for (const auto& c : cards) ranks.push_back(static_cast<int>(c.rank));
        std::sort(ranks.begin(), ranks.end());
        // 중복 제거 후 크기가 n과 같아야 (allUnique이므로 항상 같지만 명시)
        straight = (ranks.back() - ranks.front() == n - 1);

        if (straight) {
            // 로얄: 10-J-Q-K-A 포함 여부
            std::vector<int> royal = {10, 11, 12, 13, 14};
            royalStraight = std::includes(ranks.begin(), ranks.end(),
                                          royal.begin(), royal.end());
        }
    }

    // ── 판정 (강한 족보 우선) ──
    if (flush && royalStraight)  return PokerHand::RoyalFlush;
    if (flush && straight)       return PokerHand::StraightFlush;
    if (!counts.empty() && counts[0] == 4) return PokerHand::FourOfAKind;
    if (counts.size() >= 2 && counts[0] == 3 && counts[1] == 2) return PokerHand::FullHouse;
    if (flush)                   return PokerHand::Flush;
    if (straight)                return PokerHand::Straight;
    if (!counts.empty() && counts[0] == 3) return PokerHand::ThreeOfAKind;
    if (counts.size() >= 2 && counts[0] == 2 && counts[1] == 2) return PokerHand::TwoPair;
    if (!counts.empty() && counts[0] == 2) return PokerHand::OnePair;
    return PokerHand::None;
}

// ---------------------------------------------------------------------------
// Deck: 드로우 파일 + 버림 파일 + 핸드 + 고정 슬롯
// ---------------------------------------------------------------------------
class Deck {
public:
    static constexpr int HAND_SIZE    = 5;
    static constexpr int MAX_PERM     = 3; // 고정 슬롯 최대 개수

    Deck() {
        for (Suit s : {Suit::Heart, Suit::Diamond, Suit::Club, Suit::Spade}) {
            addCard({s, CardRank::Five,  CardEffect::Attack, 5, false});
            addCard({s, CardRank::Three, CardEffect::Defend, 3, false});
        }
        shuffle_();
    }

    // ---------- 카드 추가 ----------
    void addCard(const Card& card) {
        Card c = card;
        c.isPermanent = false;
        drawPile_.push_back(c);
    }

    // 고정 슬롯에 장착 (최대 MAX_PERM)
    bool equipPermanent(const Card& card) {
        if ((int)permanentSlots_.size() >= MAX_PERM) return false;
        Card c = card;
        c.isPermanent = true;
        permanentSlots_.push_back(c);
        return true;
    }

    bool canEquipMore() const { return (int)permanentSlots_.size() < MAX_PERM; }
    int  permCount()   const { return (int)permanentSlots_.size(); }

    // ---------- 핸드 드로우 ----------
    // 핸드 = 고정 슬롯 + 드로우 파일에서 나머지
    void drawHand() {
        hand_.clear();
        // 고정 슬롯 먼저 (항상 핸드 앞자리)
        for (const auto& c : permanentSlots_) hand_.push_back(c);
        // 나머지를 덱에서 채움
        int toDraw = HAND_SIZE - (int)permanentSlots_.size();
        for (int i = 0; i < toDraw; ++i) drawOne_();
    }

    // ---------- 카드 사용 ----------
    // 고정 슬롯 카드는 버림더미 안 감, 일반 카드만 소모
    void useFromHand(int idx) {
        if (idx < 0 || idx >= (int)hand_.size()) return;
        if (hand_[idx].isPermanent) {
            // 고정 카드: 핸드에서만 제거(다음 drawHand 때 다시 나옴)
            hand_.erase(hand_.begin() + idx);
        } else {
            discardPile_.push_back(hand_[idx]);
            hand_.erase(hand_.begin() + idx);
        }
    }

    // 여러 장 사용 (내림차순 인덱스로 안전하게)
    void useMultiFromHand(std::vector<int> indices) {
        std::sort(indices.rbegin(), indices.rend());
        for (int idx : indices) useFromHand(idx);
    }

    void discardAll() {
        for (auto& c : hand_)
            if (!c.isPermanent) discardPile_.push_back(c);
        hand_.clear();
    }

    // ---------- 조회 ----------
    const std::vector<Card>& getHand()          const { return hand_; }
    std::vector<Card>&       getHand()                { return hand_; }
    const std::vector<Card>& drawPile()         const { return drawPile_; }
    const std::vector<Card>& discardPile()      const { return discardPile_; }
    const std::vector<Card>& permanentSlots()   const { return permanentSlots_; }

    int deckSize()    const { return (int)drawPile_.size(); }
    int discardSize() const { return (int)discardPile_.size(); }
    int totalCards()  const { return (int)(drawPile_.size() + discardPile_.size() + permanentSlots_.size()); }

private:
    std::vector<Card> drawPile_;
    std::vector<Card> discardPile_;
    std::vector<Card> hand_;
    std::vector<Card> permanentSlots_; // 고정 슬롯 (전투마다 재사용)

    void shuffle_() {
        static std::mt19937 rng{std::random_device{}()};
        std::shuffle(drawPile_.begin(), drawPile_.end(), rng);
    }

    void drawOne_() {
        if (drawPile_.empty()) {
            drawPile_ = std::move(discardPile_);
            discardPile_.clear();
            shuffle_();
        }
        if (!drawPile_.empty()) {
            hand_.push_back(drawPile_.back());
            drawPile_.pop_back();
        }
    }
};

// ---------------------------------------------------------------------------
// CardReward
// ---------------------------------------------------------------------------
struct CardReward {
    std::vector<Card> choices; // 3장 중 1장 선택
};

inline CardReward generateCardReward(int monsterLevel, std::mt19937& rng) {
    CardReward reward;
    Suit suits[4] = {Suit::Heart, Suit::Diamond, Suit::Club, Suit::Spade};
    std::uniform_int_distribution<int> suitDist(0, 3);
    std::uniform_int_distribution<int> rankDist(2, 14);
    std::uniform_int_distribution<int> effectDist(0, 4);
    CardEffect effects[] = {
        CardEffect::Attack, CardEffect::Defend,
        CardEffect::Heal,   CardEffect::DrawCard, CardEffect::DoubleAtk
    };
    for (int i = 0; i < 3; ++i) {
        Suit s       = suits[suitDist(rng)];
        int  rankVal = 2 + static_cast<int>(rankDist(rng) % static_cast<unsigned>(6 + monsterLevel));
        if (rankVal > 14) rankVal = 14;
        CardRank   r = static_cast<CardRank>(static_cast<uint8_t>(rankVal));
        CardEffect e = effects[effectDist(rng)];
        int val = 3 + rankVal / 2 + monsterLevel;
        reward.choices.push_back({s, r, e, val, false});
    }
    return reward;
}

} // namespace Roguelike
