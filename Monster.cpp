// Monster.cpp
#include "Monster.h"
#include "Map.h"
#include "player.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>

namespace Roguelike {

    static bool isAdjacentFour(int mx, int my, int px, int py) {
        return (std::abs(mx - px) + std::abs(my - py)) == 1;
    }

    int MonsterBase::attackPlayer(Player& player) {
        int dmg = calcDamage(player.getDefense(), player.getSuit());
        player.takeDamage(dmg);
        return dmg;
    }

    bool MonsterBase::tryMove(int nx, int ny, Map& map) {
        if (!map.isWalkable(nx, ny))   return false;
        if (map.hasMonsterAt(nx, ny))  return false;
        auto idx = map.monsterIndexAt(x_, y_);
        if (idx.has_value())
            map.setMonsterPos(*idx, nx, ny);
        setPosition(nx, ny);
        return true;
    }

    void BasicMonster::onTurn(Player& player, Map& map) {
        if (!isAlive()) return;

        int px = player.getX(), py = player.getY();

        if (isAdjacentFour(x_, y_, px, py)) {
            int dmg = attackPlayer(player);
            std::cout << name() << " \xec\x9d\xb4(\xea\xb0\x80) \xed\x94\x8c\xeb\xa0\x88\xec\x9d\xb4\xec\x96\xb4\xeb\xa5\xbc \xea\xb3\xb5\xea\xb2\xa9! (" << dmg << " \xeb\x8d\xb0\xeb\xaf\xb8\xec\xa7\x80)\n";
            return;
        }

        static std::mt19937 rng{ std::random_device{}() };
        int dirs[4][2] = { {0,-1}, {0,1}, {-1,0}, {1,0} };
        std::shuffle(std::begin(dirs), std::end(dirs), rng);
        for (const auto& d : dirs) {
            int nx = x_ + d[0], ny = y_ + d[1];
            if (nx == px && ny == py) continue;
            if (tryMove(nx, ny, map)) break;
        }
    }

} // namespace Roguelike