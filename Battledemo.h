#pragma once
#ifndef BATTLE_DEMO_H
#define BATTLE_DEMO_H

#include <string>
#include <vector>

/*
    BattleDemo.h  (v2)
    인코딩 안전 버전.
    소스 주석은 모두 한국어로 작성.
    한국어 UI 문자열은 BattleDemo.cpp 에 UTF-8 바이트 리터럴로 정의.

    v1 대비 변경 사항:
    - drawBattleUI: 피격 플래시 파라미터 추가
      (enemyHit, hitDmg, playerHit, playerHitDmg)
    - 레이아웃: 전투 씬 (상단) / 메뉴 + 로그 (하단) 으로 분리
    - drawAttackAnimation 제거 →
      drawPlayerAttackAnimation / drawEnemyAttackAnimation 으로 분리
*/

/* 키 입력 코드 */
enum KeyCode {
    KEY_UP = 300,
    KEY_DOWN = 301,
    KEY_ENTER = 302
};

/* 전투에 참여하는 캐릭터 구조체 */
struct Fighter {
    std::string name;   /* 이름 */
    int hp;             /* 현재 HP */
    int maxHp;          /* 최대 HP */
    int atk;            /* 공격력 */
    int def;            /* 방어력 */
    bool defending;     /* 방어 자세 여부 */
    int potions;        /* 보유 포션 수 */

    Fighter(const std::string& n, int h, int a, int d, int pot = 2);

    bool isDead() const;
    void takeDamage(int dmg);
    void heal(int amount);
};

void initConsole();
void clearScreen();
std::string makeHpBar(int hp, int maxHp, int width = 20);

/* drawBattleUI
 *   enemyHit      - 이번 프레임에 적 아트를 빨간색으로 플래시
 *   hitDmg        - 적 근처에 표시할 데미지 수치 (0 이면 숨김)
 *   playerHit     - 이번 프레임에 플레이어 아트를 빨간색으로 플래시
 *   playerHitDmg  - 플레이어 근처에 표시할 데미지 수치 (0 이면 숨김)
 *   피격 파라미터는 모두 기본값이 false/0 이므로 기존 호출부는 수정 불필요.
 */
void drawBattleUI(const Fighter& player,
    const Fighter& enemy,
    const std::vector<std::string>& menu,
    int selected,
    const std::string& log,
    bool enemyHit = false,
    int  hitDmg = 0,
    bool playerHit = false,
    int  playerHitDmg = 0);

int readKey();

int selectMenu(const Fighter& player,
    const Fighter& enemy,
    const std::vector<std::string>& menu,
    const std::string& log);

void runBattle(Fighter player, Fighter enemy);

#endif // BATTLE_DEMO_H