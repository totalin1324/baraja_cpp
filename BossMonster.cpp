#include "BossMonster.h"
#include "player.h"
#include "Map.h"

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include <algorithm>

namespace Roguelike {

    // ---------------------------------------------------------------------------
    // 보스는 모두 "방에서 기다리는 정지형" — 맵에서는 이동·공격하지 않는다.
    // 실제 특수 패턴은 카드 전투의 battleTurn에서 발동한다.
    // ---------------------------------------------------------------------------
    void JBoss::onTurn(Player&, Map&)     {}
    void QBoss::onTurn(Player&, Map&)     {}
    void KBoss::onTurn(Player&, Map&)     {}
    void JokerBoss::onTurn(Player&, Map&) {}

    // ---------------------------------------------------------------------------
    // JBoss — 연속 공격형
    // 기본 공격 + 30% 확률로 추가 1타(콤보)
    // ---------------------------------------------------------------------------
    EnemyAction JBoss::battleTurn(Player& player, std::mt19937& rng) {

        int dmg = computeBattleDamage(player);
        player.takeDamage(dmg);

        std::string log = "Jack 공격! " + std::to_string(dmg) + " 데미지";

        // 30% 확률로 추가 공격
        if (std::uniform_int_distribution<>(0, 99)(rng) < 30) {
            int combo = computeBattleDamage(player);
            player.takeDamage(combo);
            dmg += combo;
            log += "\nJack 연속 공격! +" + std::to_string(combo) + " 데미지";
        }

        return { dmg, log };
    }

    // ---------------------------------------------------------------------------
    // QBoss — 디버프형
    // 공격 + 3턴마다 플레이어 방어력 1 감소
    // ---------------------------------------------------------------------------
    EnemyAction QBoss::battleTurn(Player& player, std::mt19937& /*rng*/) {

        int dmg = computeBattleDamage(player);
        player.takeDamage(dmg);

        std::string log = "Queen 공격! " + std::to_string(dmg) + " 데미지";

        debuffTurn_++;
        if (debuffTurn_ >= 3) {
            debuffTurn_ = 0;
            player.setDefense(player.getDefense() - 1);
            log += "\n플레이어의 방어력이 감소했다!";
        }

        return { dmg, log };
    }

    // ---------------------------------------------------------------------------
    // KBoss — 광폭화형
    // HP 절반 이하 시 1회 광폭화(공격+10, 방어+5) 후 공격
    // ---------------------------------------------------------------------------
    EnemyAction KBoss::battleTurn(Player& player, std::mt19937& /*rng*/) {

        std::string log;

        if (!enraged_ && hp_ <= maxHp_ / 2) {
            enraged_ = true;
            attack_  += 10;
            defense_ += 5;
            log = "King이 광폭화했다!\n";
        }

        int dmg = computeBattleDamage(player);
        player.takeDamage(dmg);
        log += "King 공격! " + std::to_string(dmg) + " 데미지";

        return { dmg, log };
    }

    // ---------------------------------------------------------------------------
    // JokerBoss — 랜덤 패턴형 최종 보스
    // 매턴 공격 / 자가회복 / 속성변경 중 하나
    // ---------------------------------------------------------------------------
    EnemyAction JokerBoss::battleTurn(Player& player, std::mt19937& rng) {

        int pattern = std::uniform_int_distribution<>(0, 2)(rng);
        switch (pattern) {

        // 일반 공격
        case 0: {
            int dmg = computeBattleDamage(player);
            player.takeDamage(dmg);
            return { dmg, "Joker 공격! " + std::to_string(dmg) + " 데미지" };
        }

        // 자가 회복 (비공격 턴)
        case 1: {
            hp_ = std::min(hp_ + 15, maxHp_);
            return { 0, "Joker가 체력을 회복했다!" };
        }

        // 속성 변경 (비공격 턴)
        default: {
            randomizeSuit(rng);
            return { 0, "Joker의 속성이 변경되었다!" };
        }
        }
    }

    // ---------------------------------------------------------------------------
    // JokerBoss::randomizeSuit
    // ---------------------------------------------------------------------------
    void JokerBoss::randomizeSuit(std::mt19937& rng) {

        switch (std::uniform_int_distribution<>(0, 3)(rng)) {
        case 0:  suit_ = Suit::Spade;   break;
        case 1:  suit_ = Suit::Heart;   break;
        case 2:  suit_ = Suit::Diamond; break;
        default: suit_ = Suit::Club;    break;
        }
    }

} // namespace Roguelike
