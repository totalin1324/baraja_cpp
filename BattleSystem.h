#pragma once
// BattleSystem.h
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
        bool doubleAtk = false;
        int  extraDraws = 0;
    };

    struct SelectionPreview {
        int       attackDamage = 0;
        int       healAmount = 0;
        int       defenseBonus = 0;
        int       extraDraws = 0;
        PokerHand poker = PokerHand::None;
    };

    class BattleSystem {
    public:
        // 비파괴 미리보기 계산 (playCards와 동일 공식 유지)
        static SelectionPreview previewSelection(
            const std::vector<Card>& played,
            const Player& player,
            const MonsterBase& monster)
        {
            SelectionPreview pv;
            if (played.empty()) return pv;

            pv.poker = evaluateHand(played);
            float phMul = pokerHandMultiplier(pv.poker);

            for (const auto& c : played) {
                switch (c.effect) {
                case CardEffect::Defend:   pv.defenseBonus += c.value; break;
                case CardEffect::Heal:     pv.healAmount += c.value; break;
                case CardEffect::DrawCard: pv.extraDraws += c.value; break;
                default: break;
                }
            }

            bool doubleAtk = false;
            for (const auto& c : played) {
                if (c.effect == CardEffect::DoubleAtk) {
                    doubleAtk = true;
                }
                else if (c.effect == CardEffect::Attack) {
                    float suitMult = getSuitMultiplier(c.suit, monster.getSuit());
                    int   mult = doubleAtk ? 2 : 1;
                    int   baseDmg = static_cast<int>(c.value * suitMult) * mult;
                    pv.attackDamage += std::max(1, baseDmg - monster.getDefense());
                    doubleAtk = false;
                }
            }

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
            std::string tag = (suitMult > 1.0f) ? " \033[33m[\xec\x83\x81\xec\x84\xb1\xe2\x86\x91]\033[0m"   // 상성↑
                : (suitMult < 1.0f) ? " \033[2m[\xec\x97\xad\xec\x83\x81\xec\x84\xb1\xe2\x86\x93]\033[0m" : ""; // 역상성↓

            switch (card.effect) {
            case CardEffect::Attack: {
                int mult = state.doubleAtk ? 2 : 1;
                int baseDmg = static_cast<int>(card.value * suitMult) * mult;
                int final_ = std::max(1, baseDmg - monster.getDefense());
                monster.takeDamage(final_);
                log = "\033[31m\xea\xb3\xb5\xea\xb2\xa9!\033[0m " + std::to_string(final_) + " \xeb\x8d\xb0\xeb\xaf\xb8\xec\xa7\x80" + tag; // 공격! N 데미지
                if (card.isPermanent) log += " \033[36m(\xea\xb3\xa0\xec\xa0\x95)\033[0m"; // (고정)
                state.doubleAtk = false;
                break;
            }
            case CardEffect::Defend: {
                state.tempDefenseBonus += card.value;
                log = "\033[34m\xeb\xb0\xa9\xec\x96\xb4!\033[0m \xec\x9d\xb4\xeb\xb2\x88 \xed\x84\xb4 \xeb\xb0\xa9\xec\x96\xb4 +" + std::to_string(card.value); // 방어! 이번 턴 방어 +N
                break;
            }
            case CardEffect::Heal: {
                player.heal(card.value);
                log = "\033[32m\xed\x9a\x8c\xeb\xb3\xb5!\033[0m HP +" + std::to_string(card.value); // 회복! HP +N
                break;
            }
            case CardEffect::DrawCard: {
                state.extraDraws += card.value;
                log = "\033[36m\xeb\x93\x9c\xeb\xa1\x9c\xec\x9a\xb0!\033[0m \xec\xb9\xb4\xeb\x93\x9c +" + std::to_string(card.value) + "\xec\x9e\xa5"; // 드로우! 카드 +N장
                break;
            }
            case CardEffect::DoubleAtk: {
                state.doubleAtk = true;
                log = "\033[33m\xea\xb0\x95\xed\x83\x80 \xec\xa4\x80\xeb\xb9\x84!\033[0m \xeb\x8b\xa4\xec\x9d\x8c \xea\xb3\xb5\xea\xb2\xa9 2\xeb\xb0\xb0"; // 강타 준비! 다음 공격 2배
                break;
            }
            }
            return log;
        }

        static std::string playCards(
            const std::vector<int>& selectedIndices,
            Player& player,
            MonsterBase& monster,
            Deck& deck,
            std::string& pokerResult)
        {
            auto& hand = deck.getHand();
            if (selectedIndices.empty()) return "\xec\xb9\xb4\xeb\x93\x9c\xeb\xa5\xbc \xec\x84\xa0\xed\x83\x9d\xed\x95\x98\xec\xa7\x80 \xec\x95\x8a\xec\x95\x98\xec\x8a\xb5\xeb\x8b\x88\xeb\x8b\xa4."; // 카드를 선택하지 않았습니다.

            std::vector<Card> played;
            for (int idx : selectedIndices)
                if (idx >= 0 && idx < (int)hand.size())
                    played.push_back(hand[idx]);

            PokerHand ph = evaluateHand(played);
            float     phMul = pokerHandMultiplier(ph);
            pokerResult = pokerHandToString(ph);

            BattleState state;
            std::string fullLog;

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
                    fullLog += "\033[1;33m\xe2\x98\x85 " + pokerHandToString(ph)
                        + " \xeb\xb3\xb4\xeb\x84\x88\xec\x8a\xa4! +" + std::to_string(bonusDmg) + " \xeb\x8d\xb0\xeb\xaf\xb8\xec\xa7\x80\033[0m\n"; // ★ N 보너스! +X 데미지
                }
            }

            deck.useMultiFromHand(selectedIndices);

            if (state.extraDraws > 0) {
                int drawn = deck.drawCards(state.extraDraws);
                fullLog += "\033[36m\xeb\x93\x9c\xeb\xa1\x9c\xec\x9a\xb0 \xec\xb2\x98\xeb\xa6\xac:\033[0m "
                    + std::to_string(drawn) + "/"
                    + std::to_string(state.extraDraws)
                    + "\xec\x9e\xa5\n"; // 드로우 처리: N/M장
            }

            return fullLog;
        }
    };

} // namespace Roguelike