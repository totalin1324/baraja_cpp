#include "player.h"
#include "Monster.h"
#include <algorithm>
#include <iostream>

namespace Roguelike {

Player::Player(int x, int y, Suit suit)
    : x_(x), y_(y)
    , hp_(30), maxHp_(30)
    , attack_(8), defense_(3)
    , level_(1), exp_(0), expToNext_(20)
    , suit_(suit)
    , deck_()
{}

int  Player::getX()      const { return x_; }
int  Player::getY()      const { return y_; }
void Player::setPosition(int x, int y) { x_ = x; y_ = y; }

int  Player::getHp()      const { return hp_; }
int  Player::getMaxHp()   const { return maxHp_; }
int  Player::getAttack()  const { return attack_; }
int  Player::getDefense() const { return defense_; }
int  Player::getLevel()   const { return level_; }
int  Player::getExp()     const { return exp_; }
Suit Player::getSuit()    const { return suit_; }
void Player::setSuit(Suit suit) { suit_ = suit; }

void Player::takeDamage(int rawDamage, Suit attackerSuit) {
    float multiplier = getSuitMultiplier(attackerSuit, suit_);
    int   effective  = static_cast<int>(rawDamage * multiplier) - defense_;
    int   finalDmg   = std::max(1, effective);
    hp_ = std::max(0, hp_ - finalDmg);
}

void Player::takeDamage(int finalDamage) {
    hp_ = std::max(0, hp_ - finalDamage);
}

int Player::calcDamage(int targetDefense, Suit targetSuit) const {
    float multiplier = getSuitMultiplier(suit_, targetSuit);
    int   effective  = static_cast<int>(attack_ * multiplier) - targetDefense;
    return std::max(1, effective);
}

int Player::attackMonster(MonsterBase& target) {
    int dmg = calcDamage(target.getDefense(), target.getSuit());
    target.takeDamage(dmg);
    return dmg;
}

void Player::heal(int amount) {
    hp_ = std::min(maxHp_, hp_ + amount);
}

bool Player::isAlive() const { return hp_ > 0; }

void Player::gainExp(int amount) {
    exp_ += amount;
    while (exp_ >= expToNext_) {
        exp_ -= expToNext_;
        levelUp();
    }
}

void Player::levelUp() {
    ++level_;
    maxHp_   += 5;
    hp_       = maxHp_;
    attack_  += 2;
    defense_ += 1;
    expToNext_ = static_cast<int>(expToNext_ * 1.5f);
    std::cout << "[Level Up!] Lv." << level_
              << "  HP:" << maxHp_
              << "  ATK:" << attack_
              << "  DEF:" << defense_ << "\n";
}

} // namespace Roguelike
