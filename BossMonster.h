#pragma once

#include "Monster.h"

namespace Roguelike {

    // ---------------------------------------------------------------------------
    // J Boss
    // 연속 공격형 보스
    // ---------------------------------------------------------------------------
    class JBoss final : public MonsterBase {
    public:
        JBoss(int x, int y,
            Suit suit = Suit::Spade)
            : MonsterBase(x, y, suit,
                /*hp*/40,
                /*atk*/8,
                /*def*/2,
                /*exp*/30)
        {}

        std::string name() const override {
            return "Jack";
        }

        char glyph() const override {
            return 'J';
        }

        // 맵에서는 정지형(이동·원거리 공격 없음)
        void onTurn(Player& player,
            Map& map) override;

        // 전투: 기본 공격 + 30% 확률 추가 1타(콤보)
        EnemyAction battleTurn(Player& player, std::mt19937& rng) override;
    };

    // ---------------------------------------------------------------------------
    // Q Boss
    // 디버프형 보스
    // ---------------------------------------------------------------------------
    class QBoss final : public MonsterBase {
    public:
        QBoss(int x, int y,
            Suit suit = Suit::Diamond)
            : MonsterBase(x, y, suit,
                /*hp*/70,
                /*atk*/12,
                /*def*/4,
                /*exp*/60)
            , debuffTurn_(0)
        {}

        std::string name() const override {
            return "Queen";
        }

        char glyph() const override {
            return 'Q';
        }

        // 맵에서는 정지형
        void onTurn(Player& player,
            Map& map) override;

        // 전투: 공격 + 3턴마다 플레이어 방어력 감소 디버프
        EnemyAction battleTurn(Player& player, std::mt19937& rng) override;

    private:
        int debuffTurn_;
    };

    // ---------------------------------------------------------------------------
    // K Boss
    // 광폭화형 보스
    // ---------------------------------------------------------------------------
    class KBoss final : public MonsterBase {
    public:
        KBoss(int x, int y,
            Suit suit = Suit::Heart)
            : MonsterBase(x, y, suit,
                /*hp*/120,
                /*atk*/18,
                /*def*/8,
                /*exp*/120)
            , enraged_(false)
        {}

        std::string name() const override {
            return "King";
        }

        char glyph() const override {
            return 'K';
        }

        // 맵에서는 정지형
        void onTurn(Player& player,
            Map& map) override;

        // 전투: HP 절반 이하 시 1회 광폭화 후 공격
        EnemyAction battleTurn(Player& player, std::mt19937& rng) override;

    private:
        bool enraged_;
    };

    // ---------------------------------------------------------------------------
    // Joker Boss
    // 랜덤 패턴형 최종 보스
    // ---------------------------------------------------------------------------
    class JokerBoss final : public MonsterBase {
    public:
        JokerBoss(int x, int y)
            : MonsterBase(x, y,
                Suit::None,
                /*hp*/200,
                /*atk*/25,
                /*def*/10,
                /*exp*/300)
        {}

        std::string name() const override {
            return "Joker";
        }

        char glyph() const override {
            return 'X';
        }

        // 맵에서는 정지형
        void onTurn(Player& player,
            Map& map) override;

        // 전투: 매턴 랜덤 패턴(공격 / 자가회복 / 속성변경)
        EnemyAction battleTurn(Player& player, std::mt19937& rng) override;

    private:
        void randomizeSuit(std::mt19937& rng);
    };

} // namespace Roguelike