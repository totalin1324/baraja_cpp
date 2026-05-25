#pragma once

#include "Suit.h"
#include "Card.h"
#include <string>

namespace Roguelike {

class MonsterBase;

class Player {
public:
    Player(int x, int y, Suit suit = Suit::None);

    // --- 위치 ---
    int  getX() const;
    int  getY() const;
    void setPosition(int x, int y);

    // --- 전투 스탯 ---
    int  getHp()     const;
    int  getMaxHp()  const;
    int  getAttack() const;
    int  getDefense()const;
    int  getLevel()  const;
    int  getExp()    const;
    Suit getSuit()   const;
    void setSuit(Suit suit);
    void setDefense(int def);  // 디버프 등으로 방어력 변경 (0 미만 클램프)

    // --- 전투 ---
    void takeDamage(int rawDamage, Suit attackerSuit);
    void takeDamage(int finalDamage);
    int  calcDamage(int targetDefense, Suit targetSuit) const;
    int  attackMonster(MonsterBase& target);
    void heal(int amount);
    bool isAlive() const;
    void gainExp(int amount);

    // --- 덱 빌딩 ---
    Deck&       getDeck()       { return deck_; }
    const Deck& getDeck() const { return deck_; }

    // 카드 보상 수령
    void receiveCard(const Card& card) { deck_.addCard(card); }

    // 전투 시작 시 핸드 드로우
    void drawHand() { deck_.drawHand(); }

private:
    int  x_, y_;
    int  hp_, maxHp_;
    int  attack_, defense_;
    int  level_, exp_, expToNext_;
    Suit suit_;
    Deck deck_;  // 플레이어 덱

    void levelUp();
};

} // namespace Roguelike
