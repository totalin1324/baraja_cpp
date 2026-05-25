#pragma once

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "Card.h"
#include "player.h"
#include "Monster.h"
#include <vector>
#include <string>
#include <algorithm>

namespace Roguelike {

struct BattleState {
    int  tempDefenseBonus = 0;
    bool doubleAtk        = false;
    int  extraDraws       = 0;
};

// 선택 조합의 예상 결과 (전투 중 커밋 전 표시용)
struct SelectionPreview {
    int       attackDamage = 0; // 상성·강타·족보 보너스 포함 총 공격 데미지
    int       healAmount   = 0;
    int       defenseBonus = 0;
    int       extraDraws   = 0;
    PokerHand poker        = PokerHand::None;
};

class BattleSystem {
public:
    // playCards와 동일한 공식을 비파괴로 계산만 한다.
    // ★ 데미지 공식 수정 시 playCards와 반드시 동기화할 것.
    static SelectionPreview previewSelection(
        const std::vector<Card>& played,
        const Player& player,
        const MonsterBase& monster)
    {
        SelectionPreview pv;
        if (played.empty()) return pv;

        pv.poker    = evaluateHand(played);
        float phMul = pokerHandMultiplier(pv.poker);

        // 유틸(방어/회복/드로우) — 순서가 결과에 영향 없음
        for (const auto& c : played) {
            switch (c.effect) {
                case CardEffect::Defend:   pv.defenseBonus += c.value; break;
                case CardEffect::Heal:     pv.healAmount   += c.value; break;
                case CardEffect::DrawCard: pv.extraDraws   += c.value; break;
                default: break;
            }
        }

        // 공격 + 강타 — playCards의 atkCards와 동일하게 played 순서 유지
        bool doubleAtk = false;
        for (const auto& c : played) {
            if (c.effect == CardEffect::DoubleAtk) {
                doubleAtk = true;
            } else if (c.effect == CardEffect::Attack) {
                float suitMult = getSuitMultiplier(c.suit, monster.getSuit());
                int   mult     = doubleAtk ? 2 : 1;
                int   baseDmg  = static_cast<int>(c.value * suitMult) * mult;
                pv.attackDamage += std::max(1, baseDmg - monster.getDefense());
                doubleAtk = false;
            }
        }

        // 족보 보너스
        if (pv.poker != PokerHand::None && phMul > 1.0f) {
            int bonus = static_cast<int>(player.getAttack() * (phMul - 1.0f));
            if (bonus > 0) pv.attackDamage += bonus;
        }
        return pv;
    }

    static std::string applyCard(
        const Card& card,
        Player& player,
        MonsterBase& monster,
        BattleState& state)
    {
        std::string log;
        float suitMult = getSuitMultiplier(card.suit, monster.getSuit());
        std::string tag = (suitMult > 1.0f) ? " \033[33m[상성↑]\033[0m"
                        : (suitMult < 1.0f) ? " \033[2m[역상성↓]\033[0m" : "";

        switch (card.effect) {
            case CardEffect::Attack: {
                int mult    = state.doubleAtk ? 2 : 1;
                int baseDmg = static_cast<int>(card.value * suitMult) * mult;
                int final_  = std::max(1, baseDmg - monster.getDefense());
                monster.takeDamage(final_);
                log = "\033[31m공격!\033[0m " + std::to_string(final_) + " 데미지" + tag;
                if (card.isPermanent) log += " \033[36m(고정)\033[0m";
                state.doubleAtk = false;
                break;
            }
            case CardEffect::Defend: {
                state.tempDefenseBonus += card.value;
                log = "\033[34m방어!\033[0m 이번 턴 방어 +" + std::to_string(card.value);
                break;
            }
            case CardEffect::Heal: {
                player.heal(card.value);
                log = "\033[32m회복!\033[0m HP +" + std::to_string(card.value);
                break;
            }
            case CardEffect::DrawCard: {
                state.extraDraws += card.value;
                log = "\033[36m드로우!\033[0m 카드 +" + std::to_string(card.value) + "장";
                break;
            }
            case CardEffect::DoubleAtk: {
                state.doubleAtk = true;
                log = "\033[33m강타 준비!\033[0m 다음 공격 2배";
                break;
            }
        }
        return log;
    }

    // selectedIndices: hand 인덱스들
    // 고정 카드는 사용 후 hand에서만 빠지고 버림더미 안 감
    static std::string playCards(
        const std::vector<int>& selectedIndices,
        Player& player,
        MonsterBase& monster,
        Deck& deck,
        std::string& pokerResult)
    {
        auto& hand = deck.getHand();
        if (selectedIndices.empty()) return "카드를 선택하지 않았습니다.";

        std::vector<Card> played;
        for (int idx : selectedIndices)
            if (idx >= 0 && idx < (int)hand.size())
                played.push_back(hand[idx]);

        PokerHand ph    = evaluateHand(played);
        float     phMul = pokerHandMultiplier(ph);
        pokerResult     = pokerHandToString(ph);

        BattleState state;
        std::string fullLog;

        // 유틸 먼저, 공격 나중
        std::vector<Card> atkCards, utilCards;
        for (const auto& c : played) {
            if (c.effect == CardEffect::Attack || c.effect == CardEffect::DoubleAtk)
                atkCards.push_back(c);
            else
                utilCards.push_back(c);
        }
        for (const auto& c : utilCards) fullLog += applyCard(c, player, monster, state) + "\n";
        for (const auto& c : atkCards)  fullLog += applyCard(c, player, monster, state) + "\n";

        // 족보 보너스
        if (ph != PokerHand::None && phMul > 1.0f) {
            int bonusDmg = static_cast<int>(player.getAttack() * (phMul - 1.0f));
            if (bonusDmg > 0) {
                monster.takeDamage(bonusDmg);
                fullLog += "\033[1;33m★ " + pokerHandToString(ph)
                         + " 보너스! +" + std::to_string(bonusDmg) + " 데미지\033[0m\n";
            }
        }

        // 카드 소모 (고정 카드 제외)
        deck.useMultiFromHand(selectedIndices);

        // 드로우 효과는 사용한 카드가 빠진 뒤 빈 핸드 칸만큼 채운다.
        if (state.extraDraws > 0) {
            int drawn = deck.drawCards(state.extraDraws);
            fullLog += "\033[36m드로우 처리:\033[0m "
                    + std::to_string(drawn) + "/"
                    + std::to_string(state.extraDraws)
                    + "장\n";
        }

        return fullLog;
    }
};

} // namespace Roguelike
