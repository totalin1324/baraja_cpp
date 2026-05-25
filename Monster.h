#pragma once

#include "Suit.h"
#include <string>
#include <algorithm>
#include <random>

namespace Roguelike {

// 전방 선언
class Player;
class Map;

// 전투 중 적의 한 턴 행동 결과 — 데미지(애니메이션용)와 로그(메시지)를 함께 전달
struct EnemyAction {
    int         damage;  // 플레이어에게 가한 데미지. 0이면 비공격 턴(회복·속성변경 등)
    std::string log;     // 전투 로그에 표시할 메시지
};

// ---------------------------------------------------------------------------
// MonsterBase: 추상 기반 클래스
// ---------------------------------------------------------------------------
class MonsterBase {
public:
    // suit 기본값 None — 속성 없는 잡몹 등을 별도 서브클래스 없이 생성 가능
    MonsterBase(int x, int y, Suit suit,
                int hp, int attack, int defense, int expReward)
        : x_(x), y_(y), suit_(suit)
        , hp_(hp), maxHp_(hp)
        , attack_(attack), defense_(defense)
        , expReward_(expReward)
    {}

    virtual ~MonsterBase() = default;

    // Rule of Five
    MonsterBase(const MonsterBase&)            = delete;
    MonsterBase& operator=(const MonsterBase&) = delete;
    MonsterBase(MonsterBase&&)                 = default;
    MonsterBase& operator=(MonsterBase&&)      = default;


    virtual std::string name()  const = 0;
    virtual char        glyph() const = 0;  // 맵에 표시될 문자

    // 해당 몬스터의 행동 (이동 + 공격 등) — 구현체마다 다름
    virtual void onTurn(Player& player, Map& map) = 0;

    // --- 공통 전투 ---

    // 피격 (raw): 공격자 rawDamage와 속성을 받아 방어력·상성 적용 후 hp 감소
    void takeDamage(int rawDamage, Suit attackerSuit) {
        float multiplier = getSuitMultiplier(attackerSuit, suit_);
        int   effective  = static_cast<int>(rawDamage * multiplier) - defense_;
        int   finalDmg   = std::max(1, effective);
        hp_ = std::max(0, hp_ - finalDmg);
    }

    // 피격 (final): 이미 계산된 최종 데미지를 그대로 적용 — 이중 계산 방지용
    void takeDamage(int finalDamage) {
        hp_ = std::max(0, hp_ - finalDamage);
    }

    // 공격: 플레이어에게 가하는 최종 데미지 계산 (상성 반영)
    int calcDamage(int targetDefense, Suit targetSuit) const {
        float multiplier = getSuitMultiplier(suit_, targetSuit);
        int   effective  = static_cast<int>(attack_ * multiplier) - targetDefense;
        return std::max(1, effective);
    }

    // 플레이어를 직접 공격 — 데미지 계산 및 적용을 한 번에 처리
    // 반환값: 실제로 들어간 최종 데미지 (로그 출력용)
    // 서브클래스의 onTurn()에서 공통 패턴으로 호출하면 됨
    int attackPlayer(Player& player);

    // 카드 전투 중 적의 한 턴 행동. 기본 구현은 단순 반격.
    // 보스는 오버라이드하여 콤보·디버프·광폭화 등 특수 패턴을 구현한다.
    // rng는 게임 전역 난수원 — 보스 특수기의 확률 판정에 사용한다.
    virtual EnemyAction battleTurn(Player& player, std::mt19937& rng);

    // --- 조회 ---
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

    // 속성 변경 (이벤트나 버프로 런타임 중 속성을 바꿀 여지를 남김)
    void setSuit(Suit suit) { suit_ = suit; }

protected:
    // 4방향 이동 시도: walkable이고 몬스터 없으면 이동 후 map 위치도 갱신
    bool tryMove(int nx, int ny, Map& map);

    // 카드 전투 반격 데미지 계산 — runCardBattle의 기존 공식을 한 곳에 모음
    // (attack_ - 플레이어 방어) × 상성, 최소 1
    int computeBattleDamage(const Player& player) const;

    int  x_, y_;
    Suit suit_;
    int  hp_, maxHp_;
    int  attack_, defense_;
    int  expReward_;
};

// ---------------------------------------------------------------------------
// [구현체 1] BasicMonster — 단순 근접 공격 몬스터
// ---------------------------------------------------------------------------
class BasicMonster final : public MonsterBase {
public:
    BasicMonster(int x, int y, Suit suit = Suit::None)
        : MonsterBase(x, y, suit,
                      /*hp*/10, /*atk*/4, /*def*/1, /*exp*/8)
    {}

    std::string name()  const override { return "Goblin"; }
    char        glyph() const override { return 'g'; }

    void onTurn(Player& player, Map& map) override;
};

} // namespace Roguelike
