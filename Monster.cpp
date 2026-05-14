#include "Monster.h"
#include "Map.h"
#include "player.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>

namespace Roguelike {

// 4방향 인접 판별 (맨해튼 거리 1)
static bool isAdjacentFour(int mx, int my, int px, int py) {
    return (std::abs(mx - px) + std::abs(my - py)) == 1;
}

// ---------------------------------------------------------------------------
// MonsterBase::attackPlayer
// 상성·방어력을 반영한 최종 데미지를 계산하여 플레이어에게 적용한다.
// onTurn()의 공격 절차를 한 곳에 모아 서브클래스 코드 중복을 줄임.
// ---------------------------------------------------------------------------
int MonsterBase::attackPlayer(Player& player) {
    int dmg = calcDamage(player.getDefense(), player.getSuit());
    player.takeDamage(dmg);  // 이미 최종 데미지가 계산됐으므로 그대로 전달
    return dmg;
}

// ---------------------------------------------------------------------------
// MonsterBase::tryMove
// ---------------------------------------------------------------------------
bool MonsterBase::tryMove(int nx, int ny, Map& map) {
    if (!map.isWalkable(nx, ny)) return false;
    if (map.hasMonsterAt(nx, ny)) return false;
    // 현재 위치로 map의 몬스터 좌표도 동기화
    auto idx = map.monsterIndexAt(x_, y_);
    if (idx.has_value())
        map.setMonsterPos(*idx, nx, ny);
    setPosition(nx, ny);
    return true;
}

// ---------------------------------------------------------------------------
// BasicMonster
// ---------------------------------------------------------------------------
void BasicMonster::onTurn(Player& player, Map& map) {
    if (!isAlive()) return;

    int px = player.getX(), py = player.getY();

    // 4방향 인접이면 공격
    if (isAdjacentFour(x_, y_, px, py)) {
        int dmg = attackPlayer(player);
        std::cout << name() << " 이(가) 플레이어를 공격! (" << dmg << " 데미지)\n";
        return;
    }

    // 4방향을 랜덤하게 섞어서 이동 가능한 첫 번째 방향으로 이동
    static std::mt19937 rng{ std::random_device{}() };
    int dirs[4][2] = { {0,-1}, {0,1}, {-1,0}, {1,0} };
    std::shuffle(std::begin(dirs), std::end(dirs), rng);
    for (const auto& d : dirs) {
        int nx = x_ + d[0], ny = y_ + d[1];
        if (nx == px && ny == py) continue; // 플레이어 칸은 공격으로만 처리
        if (tryMove(nx, ny, map)) break;
    }
}

} // namespace Roguelike
