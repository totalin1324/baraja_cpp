#pragma once
// Monster.h
#pragma once

#include "Suit.h"
#include <string>
#include <algorithm>

namespace Roguelike {

    class Player;
    class Map;

    // ---------------------------------------------------------------------------
    // MonsterBase
    // ---------------------------------------------------------------------------
    class MonsterBase {
    public:
        MonsterBase(int x, int y, Suit suit,
            int hp, int attack, int defense, int expReward)
            : x_(x), y_(y), suit_(suit)
            , hp_(hp), maxHp_(hp)
            , attack_(attack), defense_(defense)
            , expReward_(expReward)
        {
        }

        virtual ~MonsterBase() = default;

        MonsterBase(const MonsterBase&) = delete;
        MonsterBase& operator=(const MonsterBase&) = delete;
        MonsterBase(MonsterBase&&) = default;
        MonsterBase& operator=(MonsterBase&&) = default;

        virtual std::string name()  const = 0;
        virtual char        glyph() const = 0;
        virtual void        onTurn(Player& player, Map& map) = 0;

        // 피격 (raw): 상성·방어력 적용
        void takeDamage(int rawDamage, Suit attackerSuit) {
            float multiplier = getSuitMultiplier(attackerSuit, suit_);
            int   effective = static_cast<int>(rawDamage * multiplier) - defense_;
            int   finalDmg = std::max(1, effective);
            hp_ = std::max(0, hp_ - finalDmg);
        }

        // 피격 (final): 이미 계산된 최종 데미지를 그대로 적용
        void takeDamage(int finalDamage) {
            hp_ = std::max(0, hp_ - finalDamage);
        }

        void heal(int amount) {
            hp_ = std::min(maxHp_, hp_ + std::max(0, amount));
        }

        int calcDamage(int targetDefense, Suit targetSuit) const {
            float multiplier = getSuitMultiplier(suit_, targetSuit);
            int   effective = static_cast<int>(attack_ * multiplier) - targetDefense;
            return std::max(1, effective);
        }

        int attackPlayer(Player& player);

        int  getX()         const { return x_; }
        int  getY()         const { return y_; }
        int  getHp()        const { return hp_; }
        int  getMaxHp()     const { return maxHp_; }
        int  getAttack()    const { return attack_; }
        int  getDefense()   const { return defense_; }
        int  getExpReward() const { return expReward_; }
        Suit getSuit()      const { return suit_; }

        bool isAlive() const { return hp_ > 0; }

        void setPosition(int x, int y) { x_ = x; y_ = y; }
        void setSuit(Suit suit) { suit_ = suit; }

    protected:
        bool tryMove(int nx, int ny, Map& map);

        int  x_, y_;
        Suit suit_;
        int  hp_, maxHp_;
        int  attack_, defense_;
        int  expReward_;
    };

    // ---------------------------------------------------------------------------
    // BasicMonster
    // ---------------------------------------------------------------------------
    class BasicMonster final : public MonsterBase {
    public:
        BasicMonster(int x, int y, Suit suit = Suit::None)
            : MonsterBase(x, y, suit, /*hp*/10, /*atk*/4, /*def*/1, /*exp*/8)
        {
        }

        std::string name()  const override { return "Goblin"; }
        char        glyph() const override { return 'g'; }

        void onTurn(Player& player, Map& map) override;
    };

} // namespace Roguelike