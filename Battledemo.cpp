#include "BattleDemo.h"

#include <iostream>
#include <sstream>     /* 더블 버퍼링용 ostringstream */
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>
#include <algorithm>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <cstdio>      /* va_list 정의 포함 (conio.h 가 내부적으로 필요로 함) */
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

/* ─────────────────────────────────────────────
   ANSI 색상 코드
   ───────────────────────────────────────────── */
namespace Color {
    constexpr const char* RESET = "\033[0m";
    constexpr const char* BOLD = "\033[1m";
    constexpr const char* DIM = "\033[2m";
    constexpr const char* RED = "\033[31m";
    constexpr const char* GREEN = "\033[32m";
    constexpr const char* YELLOW = "\033[33m";
    constexpr const char* CYAN = "\033[36m";
    constexpr const char* WHITE = "\033[37m";
}

/* ─────────────────────────────────────────────
   한국어 UI 문자열 (UTF-8 바이트 리터럴)
   ───────────────────────────────────────────── */
static const char* K_POTION = "\xed\x8c\x8c\xeb\x9e\x80 \xeb\xac\xbc\xec\x95\xbd";
static const char* K_COUNT = "\xea\xb0\x9c";
static const char* K_GUIDE = "W/S \xeb\x98\x90\xeb\x8a\x94 \xeb\xb0\xa9\xed\x96\xa5\xed\x82\xa4 : \xec\x9d\xb4\xeb\x8f\x99   Enter/Space : \xec\x84\xa0\xed\x83\x9d";
static const char* K_ATTACK = "\xea\xb3\xb5\xea\xb2\xa9";
static const char* K_DEFEND = "\xeb\xb0\xa9\xec\x96\xb4";
static const char* K_ITEM = "\xec\x95\x84\xec\x9d\xb4\xed\x85\x9c";
static const char* K_RUN = "\xeb\x8f\x84\xeb\xa7\x9d";
static const char* K_APPEAR = " \xeb\x93\xb1\xec\x9e\xa5!";
static const char* K_OF_ATTACK = "\xec\x9d\x98 \xea\xb3\xb5\xea\xb2\xa9! ";
static const char* K_TO = "\xec\x97\x90\xea\xb2\x8c ";
static const char* K_DAMAGE = " \xeb\x8d\xb0\xeb\xaf\xb8\xec\xa7\x80!";
static const char* K_DEFEND_LOG = "\xeb\xb0\xa9\xec\x96\xb4 \xec\x9e\x90\xec\x84\xb8! \xec\x9d\xb4\xeb\xb2\x88 \xed\x84\xb4 \xed\x94\xbc\xed\x95\xb4\xea\xb0\x80 \xec\xa0\x88\xeb\xb0\x98\xec\x9c\xbc\xeb\xa1\x9c \xea\xb0\x90\xec\x86\x8c!";
static const char* K_NO_ITEM = "\xec\x82\xac\xec\x9a\xa9\xed\x95\xa0 \xec\x95\x84\xec\x9d\xb4\xed\x85\x9c\xec\x9d\xb4 \xec\x97\x86\xec\x8a\xb5\xeb\x8b\x88\xeb\x8b\xa4!";
static const char* K_USE_POTION = "\xed\x8c\x8c\xeb\x9e\x80 \xeb\xac\xbc\xec\x95\xbd \xec\x82\xac\xec\x9a\xa9! HP 20 \xed\x9a\x8c\xeb\xb3\xb5!";
static const char* K_RUN_OK = "\xeb\x8f\x84\xeb\xa7\x9d \xec\x84\xb1\xea\xb3\xb5!";
static const char* K_RUN_FAIL = "\xeb\x8f\x84\xeb\xa7\x9d \xec\x8b\xa4\xed\x8c\xa8! \xec\xa0\x81\xec\x9d\xb4 \xeb\xb0\x98\xea\xb2\xa9\xed\x95\xa9\xeb\x8b\x88\xeb\x8b\xa4!";
static const char* K_KILLED = " \xec\xb2\x98\xec\xb9\x98! ";
static const char* K_WIN = "\xec\x8a\xb9\xeb\xa6\xac!";
static const char* K_COUNTER = "\xec\x9d\x98 \xeb\xb0\x98\xea\xb2\xa9! ";
static const char* K_DEF_APPLY = " (\xeb\xb0\xa9\xec\x96\xb4 \xed\x9a\xa8\xea\xb3\xbc \xec\xa0\x81\xec\x9a\xa9)";
static const char* K_GAMEOVER = "\xed\x94\x8c\xeb\xa0\x88\xec\x9d\xb4\xec\x96\xb4\xea\xb0\x80 \xec\x93\xb0\xeb\xa0\x88\xec\xa1\x8c\xec\x8a\xb5\xeb\x8b\x88\xeb\x8b\xa4... Game Over";
static const char* K_ANYKEY = "\xec\x95\x84\xeb\xac\xb4 \xed\x82\xa4\xeb\x82\x98 \xeb\x88\x84\xeb\xa5\xb4\xec\x84\xb8\xec\x9a\x94...";

/* ─────────────────────────────────────────────
   Fighter 멤버 함수
   ───────────────────────────────────────────── */
Fighter::Fighter(const std::string& n, int h, int a, int d, int pot)
    : name(n), hp(h), maxHp(h), atk(a), def(d), defending(false), potions(pot) {
}

bool Fighter::isDead() const { return hp <= 0; }

void Fighter::takeDamage(int dmg) {
    if (dmg < 1) dmg = 1;
    hp -= dmg;
    if (hp < 0) hp = 0;
}

void Fighter::heal(int amount) {
    hp += amount;
    if (hp > maxHp) hp = maxHp;
}

/* ─────────────────────────────────────────────
   콘솔 유틸리티
   ───────────────────────────────────────────── */

   /* Windows: UTF-8 출력 및 ANSI 이스케이프 코드 활성화 */
void initConsole() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode)) {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, mode);
    }
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif
}

/* 화면 전체 지우기 (최초 1회 또는 필요 시에만 사용) */
void clearScreen() {
    std::cout << "\033[2J\033[H" << std::flush;
}

/* HP 바 문자열 생성 (# = 남은 HP, - = 소진된 HP) */
std::string makeHpBar(int hp, int maxHp, int width) {
    if (maxHp <= 0) maxHp = 1;
    int filled = hp * width / maxHp;
    filled = std::max(0, std::min(filled, width));

    std::string bar = "[";
    for (int i = 0; i < width; ++i) bar += (i < filled) ? '#' : '-';
    bar += "]";
    return bar;
}

/* 지정된 밀리초만큼 슬립 */
static void sleepMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

/* ─────────────────────────────────────────────
   버퍼 헬퍼 함수들
   직접 std::cout 에 쓰지 않고 ostringstream 버퍼에 기록.
   drawBattleUI 가 모든 내용을 버퍼에 모은 뒤 한 번에 출력한다.
   ───────────────────────────────────────────── */

   /* 공통 헤더를 버퍼에 기록 */
static void printHeader(std::ostringstream& out) {
    out << Color::BOLD
        << "=================================================\n"
        << "          TURN-BASED BATTLE DEMO\n"
        << "=================================================\n"
        << Color::RESET << "\n";
}

/* ─────────────────────────────────────────────
   적 ASCII 아트를 버퍼에 기록
     offset : 가로 들여쓰기 (흔들림 효과에 활용)
     hit    : true 이면 빨간색으로 렌더링
     dmgNum : hit == true 이고 dmgNum > 0 이면 데미지 수치 표시
   평소: 초록색 고블린 / 피격 시: 빨간색 + 오른쪽으로 이동
   ───────────────────────────────────────────── */
static void drawEnemyArt(std::ostringstream& out, int offset, bool hit, int dmgNum) {
    std::string pad(offset, ' ');

    /* 피격 여부에 따라 색상 전환 */
    if (hit) { out << Color::BOLD << Color::RED; }
    else { out << Color::GREEN; }

    out << pad << "     /\\   /\\\n"       /* 뿔 */
        << pad << "    /  \\_/  \\\n"
        << pad << "   ( o     o )\n"      /* 눈 */
        << pad << "   |    ^    |\n"      /* 코 */
        << pad << "   |  \\___/  |\n"     /* 입 */
        << pad << "    \\_______/\n"       /* 턱 */
        << pad << "    /|     |\\\n"      /* 어깨 */
        << pad << "   (_|_____|_)\n";     /* 몸통 */

    out << Color::RESET;

    /* 피격 프레임에만 데미지 수치 표시 */
    if (hit && dmgNum > 0) {
        out << Color::BOLD << Color::YELLOW
            << std::string(offset + 3, ' ')
            << "[ -" << dmgNum << " HP! ]\n"
            << Color::RESET;
    }
    else {
        out << "\n"; /* 줄 수 일관성 유지 */
    }
}

/* ─────────────────────────────────────────────
   플레이어 ASCII 아트를 버퍼에 기록 (단순 스틱맨)
     hit    : 피격 시 빨간색으로 플래시
     dmgNum : 피격 데미지 수치 표시
   ───────────────────────────────────────────── */
static void drawPlayerArt(std::ostringstream& out, bool hit, int dmgNum) {
    /* 피격 여부에 따라 색상 전환 */
    if (hit) { out << Color::BOLD << Color::RED; }
    else { out << Color::CYAN; }

    out << "            O\n"
        << "           /|\\\n"
        << "           / \\\n";

    out << Color::RESET;

    /* 피격 프레임에만 데미지 수치 표시 */
    if (hit && dmgNum > 0) {
        out << Color::BOLD << Color::YELLOW
            << "          [ -" << dmgNum << " HP! ]\n"
            << Color::RESET;
    }
    else {
        out << "\n"; /* 줄 수 일관성 유지 */
    }
}

/* ─────────────────────────────────────────────
   메인 전투 UI 출력
   ┌ 깜빡임 방지 원리 ────────────────────────┐
   │ 모든 내용을 ostringstream 버퍼에 먼저    │
   │ 기록한 뒤, 커서를 홈(좌상단)으로 이동   │
   │ 하고 버퍼 전체를 std::cout 에 한 번에   │
   │ 출력한다. 화면이 비었다가 채워지는      │
   │ 과정이 없으므로 지직거림이 사라진다.    │
   └──────────────────────────────────────────┘
   레이아웃 (위 → 아래):
     헤더
     [전투 씬]
       적 이름 + HP바
       적 ASCII 아트  (피격 플래시 지원)
       -- VS 구분선 --
       플레이어 ASCII 아트 (피격 플래시 지원)
       플레이어 이름 + HP바 + 포션 수
     [메뉴 UI]
       키 조작 안내
       메뉴 선택지
       배틀 로그
     푸터
   ───────────────────────────────────────────── */
void drawBattleUI(const Fighter& player,
    const Fighter& enemy,
    const std::vector<std::string>& menu,
    int selected,
    const std::string& log,
    bool enemyHit,
    int  hitDmg,
    bool playerHit,
    int  playerHitDmg)
{
    /* ── 버퍼에 프레임 전체를 먼저 기록 ─────── */
    std::ostringstream buf;

    /* 화면 지우기 + 커서 홈 이동
     * 버퍼에 담긴 뒤 한 번에 출력되므로 지우기가 포함돼도 깜빡임 없음 */
    buf << "\033[2J\033[H";

    printHeader(buf);

    /* ── 전투 씬 (상단) ────────────────────── */

    /* 적 이름 + HP */
    buf << Color::RED << Color::BOLD << "  " << enemy.name << Color::RESET
        << "   HP " << makeHpBar(enemy.hp, enemy.maxHp)
        << "  " << enemy.hp << "/" << enemy.maxHp << "\n\n";

    /* 피격 시 오른쪽으로 5칸 이동 (흔들림 효과), 평소 들여쓰기 = 2 */
    int eOffset = enemyHit ? 5 : 2;
    drawEnemyArt(buf, eOffset, enemyHit, hitDmg);

    /* VS 구분선 */
    buf << "\n"
        << Color::DIM
        << "- - - - - - - - - - - - - - - - - - - - - - - -\n"
        << Color::RESET;

    /* 플레이어 아트 */
    drawPlayerArt(buf, playerHit, playerHitDmg);

    /* 플레이어 이름 + HP + 포션 */
    buf << "\n"
        << Color::CYAN << Color::BOLD << "  " << player.name << Color::RESET
        << "   HP " << makeHpBar(player.hp, player.maxHp)
        << "  " << player.hp << "/" << player.maxHp << "\n"
        << "  " << K_POTION << ": " << player.potions << K_COUNT << "\n";

    /* ── 메뉴 UI (하단) ────────────────────── */
    buf << "\n"
        << Color::DIM
        << "=================================================\n"
        << "  " << K_GUIDE << "\n"
        << "-------------------------------------------------\n"
        << Color::RESET;

    /* 선택된 항목은 노란색 + > 마커로 강조 */
    for (int i = 0; i < static_cast<int>(menu.size()); ++i) {
        if (i == selected) {
            buf << Color::YELLOW << Color::BOLD << "  > " << menu[i] << Color::RESET << "\n";
        }
        else {
            buf << "    " << menu[i] << "\n";
        }
    }

    buf << Color::DIM << "-------------------------------------------------\n" << Color::RESET;
    buf << "  " << log << "\n";
    buf << Color::BOLD << "=================================================\n" << Color::RESET;

    /* ── 버퍼 전체를 한 번에 출력 (핵심) ────── */
    std::cout << buf.str() << std::flush;
}

/* ─────────────────────────────────────────────
   키보드 입력 읽기
   방향키(↑↓) 및 W/S → KEY_UP / KEY_DOWN
   Enter / Space    → KEY_ENTER
   ───────────────────────────────────────────── */
int readKey() {
#ifdef _WIN32
    int ch = _getch();
    if (ch == 0 || ch == 224) {
        int arrow = _getch();
        if (arrow == 72) return KEY_UP;
        if (arrow == 80) return KEY_DOWN;
    }
    if (ch == 13 || ch == 10 || ch == ' ') return KEY_ENTER;
    if (ch == 'w' || ch == 'W') return KEY_UP;
    if (ch == 's' || ch == 'S') return KEY_DOWN;
    return ch;
#else
    struct termios oldt {}, newt{};
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    int ch = getchar();

    /* ESC 시퀀스로 방향키 처리 */
    if (ch == 27) {
        int a = getchar();
        if (a == '[') {
            int b = getchar();
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            if (b == 'A') return KEY_UP;
            if (b == 'B') return KEY_DOWN;
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    if (ch == '\n' || ch == '\r' || ch == ' ') return KEY_ENTER;
    if (ch == 'w' || ch == 'W') return KEY_UP;
    if (ch == 's' || ch == 'S') return KEY_DOWN;
    return ch;
#endif
}

/* ─────────────────────────────────────────────
   메뉴 선택 루프
   플레이어가 항목을 선택할 때까지 UI를 반복 렌더링
   ───────────────────────────────────────────── */
int selectMenu(const Fighter& player,
    const Fighter& enemy,
    const std::vector<std::string>& menu,
    const std::string& log)
{
    int selected = 0;
    while (true) {
        drawBattleUI(player, enemy, menu, selected, log);
        int key = readKey();

        if (key == KEY_UP) {
            --selected;
            if (selected < 0) selected = static_cast<int>(menu.size()) - 1;
        }
        else if (key == KEY_DOWN) {
            ++selected;
            if (selected >= static_cast<int>(menu.size())) selected = 0;
        }
        else if (key == KEY_ENTER) {
            return selected;
        }
    }
}

/* 아무 키나 누를 때까지 대기 */
static void waitAnyKey(const std::string& msg) {
    std::cout << "\n  " << msg << "\n" << std::flush;
    readKey();
}

/* ─────────────────────────────────────────────
   플레이어 공격 애니메이션
     프레임 1   : 돌진 메시지 표시, 적 정상 상태
     프레임 2~3 : 적 아트 빨간 플래시 on/off 반복
     프레임 4   : 최종 피격 상태 유지 (데미지 수치 표시)
   ───────────────────────────────────────────── */
static void drawPlayerAttackAnimation(
    const Fighter& player,
    const Fighter& enemy,
    const std::vector<std::string>& menu,
    const std::string& resultLog,
    int dmg)
{
    /* 프레임 1: 돌진 */
    std::string chargeLog =
        std::string(Color::YELLOW) + std::string(Color::BOLD) +
        player.name + "  ---->  " + enemy.name + "  !!!" +
        std::string(Color::RESET);
    drawBattleUI(player, enemy, menu, 0, chargeLog);
    sleepMs(160);

    /* 프레임 2~3: 피격 플래시 (히트 / 노히트 교차) */
    for (int i = 0; i < 2; ++i) {
        drawBattleUI(player, enemy, menu, 0, resultLog,
            /*enemyHit=*/true, dmg);
        sleepMs(170);
        drawBattleUI(player, enemy, menu, 0, resultLog,
            /*enemyHit=*/false, 0);
        sleepMs(80);
    }

    /* 프레임 4: 데미지 수치가 보이도록 히트 상태 유지 */
    drawBattleUI(player, enemy, menu, 0, resultLog,
        /*enemyHit=*/true, dmg);
    sleepMs(260);
}

/* ─────────────────────────────────────────────
   적 반격 애니메이션
   플레이어 공격 애니메이션과 동일한 구조,
   플레이어 아트를 빨간색으로 플래시
   ───────────────────────────────────────────── */
static void drawEnemyAttackAnimation(
    const Fighter& player,
    const Fighter& enemy,
    const std::vector<std::string>& menu,
    const std::string& resultLog,
    int dmg)
{
    /* 프레임 1: 적 돌진 */
    std::string chargeLog =
        std::string(Color::RED) + std::string(Color::BOLD) +
        enemy.name + "  ---->  " + player.name + "  !!!" +
        std::string(Color::RESET);
    drawBattleUI(player, enemy, menu, 0, chargeLog);
    sleepMs(160);

    /* 프레임 2~3: 플레이어 피격 플래시 */
    for (int i = 0; i < 2; ++i) {
        drawBattleUI(player, enemy, menu, 0, resultLog,
            false, 0, /*playerHit=*/true, dmg);
        sleepMs(170);
        drawBattleUI(player, enemy, menu, 0, resultLog,
            false, 0, false, 0);
        sleepMs(80);
    }

    /* 프레임 4: 피격 상태 유지 */
    drawBattleUI(player, enemy, menu, 0, resultLog,
        false, 0, /*playerHit=*/true, dmg);
    sleepMs(260);
}

/* ─────────────────────────────────────────────
   전투 결과 화면 출력 후 키 입력 대기
   ───────────────────────────────────────────── */
static void showResult(const Fighter& player,
    const Fighter& enemy,
    const std::vector<std::string>& menu,
    const std::string& resultMsg)
{
    drawBattleUI(player, enemy, menu, 0, resultMsg);
    sleepMs(300);
    waitAnyKey(K_ANYKEY);
}

/* ─────────────────────────────────────────────
   메인 전투 루프
   ───────────────────────────────────────────── */
void runBattle(Fighter player, Fighter enemy) {
    initConsole();
    srand(static_cast<unsigned int>(time(nullptr)));

    std::vector<std::string> menu = { K_ATTACK, K_DEFEND, K_ITEM, K_RUN };
    std::string log = enemy.name + K_APPEAR;

    /* 최초 1회만 화면 전체를 지워 이전 내용을 제거 */
    clearScreen();

    /* 등장 메시지 표시 */
    drawBattleUI(player, enemy, menu, 0, log);
    sleepMs(800);

    while (!player.isDead() && !enemy.isDead()) {
        player.defending = false;
        int choice = selectMenu(player, enemy, menu, log);

        /* ── 플레이어 행동 처리 ──────────────── */
        if (choice == 0) {          /* 공격 */
            int dmg = player.atk - enemy.def;
            if (dmg < 1) dmg = 1;
            enemy.takeDamage(dmg);
            log = player.name + K_OF_ATTACK + enemy.name + K_TO
                + std::to_string(dmg) + K_DAMAGE;
            drawPlayerAttackAnimation(player, enemy, menu, log, dmg);

        }
        else if (choice == 1) {   /* 방어 */
            player.defending = true;
            log = K_DEFEND_LOG;
            drawBattleUI(player, enemy, menu, choice, log);
            sleepMs(700);

        }
        else if (choice == 2) {   /* 아이템 사용 */
            if (player.potions <= 0) {
                log = K_NO_ITEM;
                drawBattleUI(player, enemy, menu, choice, log);
                sleepMs(700);
                continue;
            }
            --player.potions;
            player.heal(20);
            log = K_USE_POTION;
            drawBattleUI(player, enemy, menu, choice, log);
            sleepMs(700);

        }
        else if (choice == 3) {   /* 도망 */
            if (rand() % 100 < 50) {
                log = K_RUN_OK;
                showResult(player, enemy, menu, log);
                return;
            }
            log = K_RUN_FAIL;
            drawBattleUI(player, enemy, menu, choice, log);
            sleepMs(700);
        }

        /* ── 적 사망 체크 ────────────────────── */
        if (enemy.isDead()) {
            log = enemy.name + K_KILLED + K_WIN;
            showResult(player, enemy, menu, log);
            return;
        }

        /* ── 적 반격 ─────────────────────────── */
        int eDmg = enemy.atk - player.def;
        if (eDmg < 1) eDmg = 1;

        /* 방어 자세이면 피해 절반 */
        if (player.defending) {
            eDmg /= 2;
            if (eDmg < 1) eDmg = 1;
        }

        player.takeDamage(eDmg);
        log = enemy.name + K_COUNTER + std::to_string(eDmg) + K_DAMAGE;
        if (player.defending) log += K_DEF_APPLY;

        drawEnemyAttackAnimation(player, enemy, menu, log, eDmg);

        /* ── 플레이어 사망 체크 ──────────────── */
        if (player.isDead()) {
            log = K_GAMEOVER;
            showResult(player, enemy, menu, log);
            return;
        }
    }
}