#pragma once

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

class BattleSystem {
public:
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

        return fullLog;
    }
};

} // namespace Roguelike
