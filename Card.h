// Card.h
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
        Two = 2, Three = 3, Four = 4, Five = 5, Six = 6, Seven = 7, Eight = 8,
        Nine = 9, Ten = 10, Jack = 11, Queen = 12, King = 13, Ace = 14
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
        case CardEffect::Attack:    return "\xea\xb3\xb5\xea\xb2\xa9"; // 공격
        case CardEffect::Defend:    return "\xeb\xb0\xa9\xec\x96\xb4"; // 방어
        case CardEffect::Heal:      return "\xed\x9a\x8c\xeb\xb3\xb5"; // 회복
        case CardEffect::DrawCard:  return "\xeb\x93\x9c\xeb\xa1\x9c\xec\x9a\xb0"; // 드로우
        case CardEffect::DoubleAtk: return "\xea\xb0\x95\xed\x83\x80"; // 강타
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
            if (isPermanent) s += "(\xea\xb3\xa0)"; // (고)
            return s;
        }
    };

    // ---------------------------------------------------------------------------
    // PokerHand
    // ---------------------------------------------------------------------------
    enum class PokerHand {
        None = 0, OnePair = 1, TwoPair = 2, ThreeOfAKind = 3,
        Straight = 4, Flush = 5, FullHouse = 6, FourOfAKind = 7,
        StraightFlush = 8, RoyalFlush = 9
    };

    inline std::string pokerHandToString(PokerHand h) {
        switch (h) {
        case PokerHand::None:          return "\xec\x97\x86\xec\x9d\x8c"; // 없음
        case PokerHand::OnePair:       return "\xec\x9b\x90 \xed\x8e\x98\xec\x96\xb4"; // 원 페어
        case PokerHand::TwoPair:       return "\xed\x88\xac \xed\x8e\x98\xec\x96\xb4"; // 투 페어
        case PokerHand::ThreeOfAKind:  return "\xed\x8a\xb8\xeb\xa6\xac\xed\x94\x8c"; // 트리플
        case PokerHand::Straight:      return "\xec\x8a\xa4\xed\x8a\xb8\xeb\xa0\x88\xec\x9d\xb4\xed\x8a\xb8"; // 스트레이트
        case PokerHand::Flush:         return "\xed\x94\x8c\xeb\x9f\xac\xec\x8b\x9c"; // 플러시
        case PokerHand::FullHouse:     return "\xed\x92\x80\xed\x95\x98\xec\x9a\xb0\xec\x8a\xa4"; // 풀하우스
        case PokerHand::FourOfAKind:   return "\xed\x8f\xac\xec\xb9\xb4\xeb\x93\x9c"; // 포카드
        case PokerHand::StraightFlush: return "\xec\x8a\xa4\xed\x8a\xb8\xeb\xa0\x88\xec\x9d\xb4\xed\x8a\xb8 \xed\x94\x8c\xeb\x9f\xac\xec\x8b\x9c"; // 스트레이트 플러시
        case PokerHand::RoyalFlush:    return "\xeb\xa1\x9c\xec\x96\xb4 \xed\x94\x8c\xeb\x9f\xac\xec\x8b\x9c"; // 로얄 플러시
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
    // ---------------------------------------------------------------------------
    inline PokerHand evaluateHand(const std::vector<Card>& cards) {
        const int n = static_cast<int>(cards.size());
        if (n < 2) return PokerHand::None;

        int freq[15] = {};
        for (const auto& c : cards) freq[static_cast<int>(c.rank)]++;

        std::vector<int> counts;
        for (int i = 2; i <= 14; ++i)
            if (freq[i] > 0) counts.push_back(freq[i]);
        std::sort(counts.rbegin(), counts.rend());

        bool allUnique = (counts.empty() || counts[0] == 1);

        bool flush = false;
        if (n >= 5 && allUnique) {
            flush = std::all_of(cards.begin(), cards.end(),
                [&](const Card& c) { return c.suit == cards[0].suit; });
        }

        bool straight = false, royalStraight = false;
        if (n >= 5 && allUnique) {
            std::vector<int> ranks;
            ranks.reserve(n);
            for (const auto& c : cards) ranks.push_back(static_cast<int>(c.rank));
            std::sort(ranks.begin(), ranks.end());
            straight = (ranks.back() - ranks.front() == n - 1);

            if (straight) {
                std::vector<int> royal = { 10, 11, 12, 13, 14 };
                royalStraight = std::includes(ranks.begin(), ranks.end(),
                    royal.begin(), royal.end());
            }
        }

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
    // Deck
    // ---------------------------------------------------------------------------
    class Deck {
    public:
        enum { HAND_SIZE = 5, MAX_PERM = 3 };

        Deck() {
            for (Suit s : {Suit::Heart, Suit::Diamond, Suit::Club, Suit::Spade}) {
                addCard({ s, CardRank::Five,  CardEffect::Attack, 5, false });
                addCard({ s, CardRank::Three, CardEffect::Defend, 3, false });
            }
            shuffle_();
        }

        void addCard(const Card& card) {
            Card c = card;
            c.isPermanent = false;
            drawPile_.push_back(c);
        }

        bool equipPermanent(const Card& card) {
            if ((int)permanentSlots_.size() >= MAX_PERM) return false;
            Card c = card;
            c.isPermanent = true;
            permanentSlots_.push_back(c);
            return true;
        }

        bool canEquipMore() const { return (int)permanentSlots_.size() < MAX_PERM; }
        int  permCount()   const { return (int)permanentSlots_.size(); }

        void drawHand() {
            hand_.clear();
            for (const auto& c : permanentSlots_) hand_.push_back(c);
            int toDraw = HAND_SIZE - (int)permanentSlots_.size();
            for (int i = 0; i < toDraw; ++i) drawOne_();
        }

        int drawCards(int count) {
            int drawn = 0;
            for (int i = 0; i < count && (int)hand_.size() < HAND_SIZE; ++i) {
                int before = (int)hand_.size();
                drawOne_();
                if ((int)hand_.size() > before) ++drawn;
            }
            return drawn;
        }

        void useFromHand(int idx) {
            if (idx < 0 || idx >= (int)hand_.size()) return;
            if (hand_[idx].isPermanent) {
                hand_.erase(hand_.begin() + idx);
            }
            else {
                discardPile_.push_back(hand_[idx]);
                hand_.erase(hand_.begin() + idx);
            }
        }

        void useMultiFromHand(std::vector<int> indices) {
            std::sort(indices.rbegin(), indices.rend());
            for (int idx : indices) useFromHand(idx);
        }

        void discardAll() {
            for (auto& c : hand_)
                if (!c.isPermanent) discardPile_.push_back(c);
            hand_.clear();
        }

        // 핸드의 handIdx번 카드와 가방(드로우+버림) bagIdx번 카드를 1:1 교환.
        // 고정 카드이거나 인덱스가 범위를 벗어나면 false 반환.
        bool swapOneFromBag(int handIdx, int bagIdx) {
            if (handIdx < 0 || handIdx >= (int)hand_.size()) return false;
            if (hand_[handIdx].isPermanent) return false;
            const int dpSz = (int)drawPile_.size();
            const int total = dpSz + (int)discardPile_.size();
            if (bagIdx < 0 || bagIdx >= total) return false;

            // 가방 카드 꺼내기
            Card bagCard;
            if (bagIdx < dpSz) {
                bagCard = drawPile_[bagIdx];
                drawPile_.erase(drawPile_.begin() + bagIdx);
            }
            else {
                int di = bagIdx - dpSz;
                bagCard = discardPile_[di];
                discardPile_.erase(discardPile_.begin() + di);
            }
            // 기존 핸드 카드 → 버림더미
            discardPile_.push_back(hand_[handIdx]);
            hand_[handIdx] = bagCard;
            return true;
        }
        //   드로우+버림더미 합산 인덱스 목록을 받아
        //   현재 비-고정 핸드를 버림더미로 보내고 지정 카드를 핸드로 가져온다.
        //   bagIndices: 0 ~ drawPile_.size()-1  → 드로우 파일
        //               drawPile_.size() ~ ...  → 버림 파일
        // -----------------------------------------------------------------------
        void swapHandFromBag(const std::vector<int>& bagIndices) {
            // 1) 비-고정 핸드 → 버림더미
            for (int i = (int)hand_.size() - 1; i >= 0; --i)
                if (!hand_[i].isPermanent) {
                    discardPile_.push_back(hand_[i]);
                    hand_.erase(hand_.begin() + i);
                }

            // 2) 인덱스를 드로우/버림으로 분리 (스냅샷 크기 기준)
            const int dpSz = (int)drawPile_.size();
            std::vector<int> dpIdx, discIdx;
            for (int idx : bagIndices) {
                if (idx < dpSz) dpIdx.push_back(idx);
                else            discIdx.push_back(idx - dpSz);
            }
            // 높은 인덱스부터 제거해야 앞 인덱스가 밀리지 않음
            std::sort(dpIdx.rbegin(), dpIdx.rend());
            std::sort(discIdx.rbegin(), discIdx.rend());

            // 3) 버림더미에서 먼저 꺼내기 (드로우 파일 인덱스에 영향 없음)
            std::vector<Card> toAdd;
            for (int i : discIdx) {
                if (i >= 0 && i < (int)discardPile_.size()) {
                    toAdd.push_back(discardPile_[i]);
                    discardPile_.erase(discardPile_.begin() + i);
                }
            }
            // 4) 드로우 파일에서 꺼내기
            for (int i : dpIdx) {
                if (i >= 0 && i < (int)drawPile_.size()) {
                    toAdd.push_back(drawPile_[i]);
                    drawPile_.erase(drawPile_.begin() + i);
                }
            }
            // 5) 핸드에 추가 (고정 슬롯 뒤)
            for (const auto& c : toAdd) hand_.push_back(c);
        }

        const std::vector<Card>& getHand()        const { return hand_; }
        std::vector<Card>& getHand() { return hand_; }
        const std::vector<Card>& drawPile()       const { return drawPile_; }
        const std::vector<Card>& discardPile()    const { return discardPile_; }
        const std::vector<Card>& permanentSlots() const { return permanentSlots_; }

        int deckSize()    const { return (int)drawPile_.size(); }
        int discardSize() const { return (int)discardPile_.size(); }
        int totalCards()  const { return (int)(drawPile_.size() + discardPile_.size() + permanentSlots_.size()); }

    private:
        std::vector<Card> drawPile_;
        std::vector<Card> discardPile_;
        std::vector<Card> hand_;
        std::vector<Card> permanentSlots_;

        void shuffle_() {
            static std::mt19937 rng{ std::random_device{}() };
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
        std::vector<Card> choices;
    };

    inline CardReward generateCardReward(int monsterLevel, std::mt19937& rng) {
        CardReward reward;
        Suit suits[4] = { Suit::Heart, Suit::Diamond, Suit::Club, Suit::Spade };
        std::uniform_int_distribution<int> suitDist(0, 3);
        std::uniform_int_distribution<int> rankDist(2, 14);
        std::uniform_int_distribution<int> effectDist(0, 4);
        CardEffect effects[] = {
            CardEffect::Attack, CardEffect::Defend,
            CardEffect::Heal,   CardEffect::DrawCard, CardEffect::DoubleAtk
        };
        for (int i = 0; i < 3; ++i) {
            Suit s = suits[suitDist(rng)];
            int  rankVal = 2 + static_cast<int>(rankDist(rng) % static_cast<unsigned>(6 + monsterLevel));
            if (rankVal > 14) rankVal = 14;
            CardRank   r = static_cast<CardRank>(static_cast<uint8_t>(rankVal));
            CardEffect e = effects[effectDist(rng)];
            int val = 3 + rankVal / 2 + monsterLevel;
            reward.choices.push_back({ s, r, e, val, false });
        }
        return reward;
    }

} // namespace Roguelike