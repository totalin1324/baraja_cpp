// main.cpp
// ============================================================
//  [수정] BattleDemo.h / BattleDemo.cpp 프로젝트에서 제거.
//  Color namespace, initConsole, clearScreen, makeHpBar,
//  readKey 를 main.cpp 안에서만 static 으로 정의하여
//  LNK4221 / 중복 심볼 경고를 완전히 없앤다.
// ============================================================

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <conio.h>

#include "Map.h"
#include "MapGenerator.h"
#include "Monster.h"
#include "player.h"
#include "Card.h"
#include "BattleSystem.h"
#include "Suit.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace Roguelike;

// ===========================================================================
// ANSI 색상
// ===========================================================================
namespace Color {
    constexpr const char* RESET = "\033[0m";
    constexpr const char* BOLD = "\033[1m";
    constexpr const char* DIM = "\033[2m";
    constexpr const char* RED = "\033[31m";
    constexpr const char* GREEN = "\033[32m";
    constexpr const char* YELLOW = "\033[33m";
    constexpr const char* BLUE = "\033[34m";
    constexpr const char* MAGENTA = "\033[35m";
    constexpr const char* CYAN = "\033[36m";
    constexpr const char* WHITE = "\033[37m";
}

// ===========================================================================
// 콘솔 유틸리티
// ===========================================================================
static void initConsole() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode)) {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, mode);
    }
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(hOut, &ci);
    ci.bVisible = false;
    SetConsoleCursorInfo(hOut, &ci);
}

static void sleepMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

static void clearScreen() { std::cout << "\033[2J\033[H" << std::flush; }

static std::string makeHpBar(int hp, int maxHp, int width = 20) {
    if (maxHp <= 0) maxHp = 1;
    int filled = std::max(0, std::min(hp * width / maxHp, width));
    std::string bar = "[";
    for (int i = 0; i < width; ++i) bar += (i < filled ? '#' : '-');
    return bar + "]";
}

static constexpr int KEY_UP = 300;
static constexpr int KEY_DOWN = 301;
static constexpr int KEY_ENTER = 302;

static int readKey() {
    int ch = _getch();
    if (ch == 0 || ch == 224) {
        int a = _getch();
        if (a == 72) return KEY_UP;
        if (a == 80) return KEY_DOWN;
    }
    if (ch == 13 || ch == 10 || ch == ' ') return KEY_ENTER;
    if (ch == 'w' || ch == 'W') return KEY_UP;
    if (ch == 's' || ch == 'S') return KEY_DOWN;
    return ch;
}

// ===========================================================================
// 카드 표시 헬퍼
// ===========================================================================
static const char* suitSym(Suit s) {
    switch (s) {
    case Suit::Heart:   return "H";
    case Suit::Diamond: return "D";
    case Suit::Club:    return "C";
    case Suit::Spade:   return "S";
    case Suit::None:    return "N";
    default:            return "?";
    }
}

static const char* effectName(CardEffect e) {
    switch (e) {
    case CardEffect::Attack:    return "\xea\xb3\xb5\xea\xb2\xa9";             // 공격
    case CardEffect::Defend:    return "\xeb\xb0\xa9\xec\x96\xb4";             // 방어
    case CardEffect::Heal:      return "\xed\x9a\x8c\xeb\xb3\xb5";             // 회복
    case CardEffect::DrawCard:  return "\xeb\x93\x9c\xeb\xa1\x9c\xec\x9a\xb0"; // 드로우
    case CardEffect::DoubleAtk: return "\xea\xb0\x95\xed\x83\x80";             // 강타
    default: return "?";
    }
}

static std::string cardDisplay(const Card& c) {
    std::string s = suitSym(c.suit) + rankToString(c.rank)
        + "[" + effectName(c.effect) + " " + std::to_string(c.value) + "]";
    if (c.isPermanent) s += "(\xea\xb3\xa0)"; // (고)
    return s;
}

static const char* cardColor(CardEffect e) {
    switch (e) {
    case CardEffect::Attack:    return Color::RED;
    case CardEffect::Defend:    return Color::BLUE;
    case CardEffect::Heal:      return Color::GREEN;
    case CardEffect::DrawCard:  return Color::CYAN;
    case CardEffect::DoubleAtk: return Color::YELLOW;
    default: return Color::WHITE;
    }
}

// ===========================================================================
// 적 ASCII 아트
// ===========================================================================
static std::vector<std::string> getEnemyArt(char glyph, bool hit) {
    std::string col;
    if (hit) col = std::string(Color::BOLD) + Color::RED;
    else {
        switch (glyph) {
        case 'K': col = "\033[1;33m"; break;
        case 'Q': col = "\033[1;95m"; break;
        case 'J': col = "\033[1;36m"; break;
        case '*': col = "\033[1;97m"; break;
        case 'G': col = "\033[92m";   break;
        default:  col = "\033[32m";   break;
        }
    }
    const std::string r = Color::RESET;

    if (glyph == 'g') {
        return {
            col + "       ,      ," + r,
            col + "      /(.-\"\"-.)\\  " + r,
            col + " |\\  \\/  o  o  \\/  /|" + r,
            col + "  \\ V    <>    V /" + r,
            col + "   \\    .--.    /" + r,
            col + "    |  /    \\  |" + r,
            col + "    |_|__||__|_|" + r,
            col + "      /_/  \\_\\" + r,
        };
    }
    else if (glyph == 'G') {
        return {
            col + " *    /(.-\"\"\"-.)\\   *" + r,
            col + " |\\  \\/  @ @  \\/  /|" + r,
            col + "  \\ V   <_*_>   V /" + r,
            col + "   \\   .-===-.   /" + r,
            col + "    | /  |||  \\ |" + r,
            col + "    ||   |||   ||" + r,
            col + "    |_|__|||__|_|" + r,
            col + "      /_/   \\_\\" + r,
        };
    }
    else if (glyph == 'K') {
        return {
            col + "       /K\\  /K\\" + r,
            col + "    .--|=K=K=|--." + r,
            col + "   /   | ^^ |   \\" + r,
            col + "  |  K | -- | K  |" + r,
            col + "  | .--|____|--. |" + r,
            col + "  | |  /####\\  | |" + r,
            col + "  |_|  /####\\  |_|" + r,
            col + "      /_/  \\_\\" + r,
        };
    }
    else if (glyph == 'Q') {
        return {
            col + "       /\\ /\\ /\\" + r,
            col + "      <  Q   Q  >" + r,
            col + "       .--\\^/--." + r,
            col + "      /   .-.   \\" + r,
            col + "     |   /   \\   |" + r,
            col + "     |  | \\_/ |  |" + r,
            col + "      '--.___.-'" + r,
            col + "      /_|_____|_\\" + r,
        };
    }
    else if (glyph == 'J') {
        return {
            col + "        / J \\" + r,
            col + "       /___J_\\" + r,
            col + "     .---||---." + r,
            col + "    |   _||_   |" + r,
            col + "    |  / || \\  |" + r,
            col + "     \\ \\_||_/ /" + r,
            col + "      \\/  /\\ \\/" + r,
            col + "      /_/    \\_\\" + r,
        };
    }
    else if (glyph == '*') {
        return {
            col + "    *  /_\\ /_\\  *" + r,
            col + "      (/ o o \\)" + r,
            col + "   .-oo\\  ^  /oo-." + r,
            col + "  / /  /_|*|_\\  \\ \\" + r,
            col + " | |    / | \\    | |" + r,
            col + "  \\ \\ .--+--.  / /" + r,
            col + "   '._\\  / \\  /_.' " + r,
            col + "       /_/ \\_\\" + r,
        };
    }
    return {
        col + "     /\\   /\\" + r,
        col + "    /  \\_/  \\" + r,
        col + "   ( o     o )" + r,
        col + "   |    ^    |" + r,
        col + "   |  \\___/  |" + r,
        col + "    \\_______/" + r,
        col + "    /|     |\\" + r,
        col + "   (_|_____|_)" + r,
    };
}

static void printPlayerArt(std::ostringstream& buf, bool hit, int dmgNum) {
    const char* col = hit ? Color::RED : Color::CYAN;
    buf << col << "            O\n"
        << "           /|\\\n"
        << "           / \\\n" << Color::RESET;
    if (hit && dmgNum > 0)
        buf << Color::BOLD << Color::YELLOW << "          [ -" << dmgNum << " HP! ]\n" << Color::RESET;
    else
        buf << "\n";
}

// ===========================================================================
// 배틀 UI
// ===========================================================================
struct BattleUI {
    std::string            enemyName;
    int   enemyHp = 0, enemyMaxHp = 1, enemyAtk = 0, enemyDef = 0;
    Suit  enemySuit = Suit::None;
    char  enemyGlyph = 'g';
    int   playerHp = 0, playerMaxHp = 1, playerAtk = 0, playerDef = 0;
    Suit  playerSuit = Suit::None;
    const std::vector<Card>* hand = nullptr;
    const std::vector<int>* selected = nullptr;
    int   deckSz = 0, discardSz = 0, permCount = 0, permMax = 3;
    std::string log;
    bool  enemyHit = false; int hitDmg = 0;
    bool  playerHit = false; int playerDmg = 0;
};

static void drawBattleScreen(const BattleUI& ui) {
    std::ostringstream buf;
    buf << "\033[2J\033[H";

    bool isBoss = (ui.enemyGlyph == 'K' || ui.enemyGlyph == 'Q'
        || ui.enemyGlyph == 'J' || ui.enemyGlyph == '*');
    if (isBoss)
        buf << Color::BOLD << Color::YELLOW
        << "=================================================\n"
        << "           *** BOSS BATTLE ***\n"
        << "=================================================\n" << Color::RESET << "\n";
    else
        buf << Color::BOLD
        << "=================================================\n"
        << "              CARD BATTLE\n"
        << "=================================================\n" << Color::RESET << "\n";

    buf << Color::RED << Color::BOLD << "  " << ui.enemyName << Color::RESET
        << " " << suitSym(ui.enemySuit)
        << "  HP " << makeHpBar(ui.enemyHp, ui.enemyMaxHp)
        << "  " << ui.enemyHp << "/" << ui.enemyMaxHp
        << "  ATK:" << ui.enemyAtk << "  DEF:" << ui.enemyDef << "\n\n";

    auto art = getEnemyArt(ui.enemyGlyph, ui.enemyHit);
    for (const auto& line : art)
        buf << "  " << line << "\n";
    if (ui.enemyHit && ui.hitDmg > 0)
        buf << Color::BOLD << Color::YELLOW << "     [ -" << ui.hitDmg << " HP! ]\n" << Color::RESET;
    else
        buf << "\n";

    buf << "\n" << Color::DIM
        << "- - - - - - - - - - - - - - - - - - - - - - - -\n"
        << Color::RESET;

    printPlayerArt(buf, ui.playerHit, ui.playerDmg);

    buf << "\n" << Color::CYAN << Color::BOLD
        << "  [\xeb\x82\x98] " << Color::RESET  // [나]
        << suitSym(ui.playerSuit)
        << "  HP " << makeHpBar(ui.playerHp, ui.playerMaxHp)
        << "  " << ui.playerHp << "/" << ui.playerMaxHp
        << "  ATK:" << ui.playerAtk << "  DEF:" << ui.playerDef << "\n"
        << "  \xea\xb3\xa0\xec\xa0\x95\xec\x8a\xac\xeb\xa1\xaf: "  // 고정슬롯:
        << ui.permCount << "/" << ui.permMax << "\n";

    buf << "\n" << Color::DIM
        << "=================================================\n"
        << "  [1-5] \xec\x84\xa0\xed\x83\x9d/\xed\x95\xb4\xec\xa0\x9c"  // 선택/해제
        << "  [Enter] \xec\x82\xac\xec\x9a\xa9"                           // 사용
        << "  [B] \xea\xb0\x80\xeb\xb0\xa9"                               // 가방
        << "  [S] \xed\x84\xb4\xeb\x84\x98\xea\xb8\xb0\n"                 // 턴넘기기
        << "-------------------------------------------------\n"
        << Color::RESET;

    if (ui.hand) {
        for (int i = 0; i < (int)ui.hand->size(); ++i) {
            const Card& c = (*ui.hand)[i];
            bool sel = ui.selected &&
                std::find(ui.selected->begin(), ui.selected->end(), i) != ui.selected->end();
            buf << (sel ? (std::string(Color::YELLOW) + Color::BOLD + "  > ") : "    ");
            buf << (i + 1) << ": ";
            buf << cardColor(c.effect) << cardDisplay(c) << Color::RESET;
            if (c.isPermanent)
                buf << Color::CYAN << " \xe2\x98\x85\xea\xb3\xa0\xec\xa0\x95" << Color::RESET; // ★고정
            buf << "\n";
        }
        if (ui.selected && !ui.selected->empty()) {
            std::vector<Card> preview;
            for (int idx : *ui.selected)
                if (idx >= 0 && idx < (int)ui.hand->size())
                    preview.push_back((*ui.hand)[idx]);
            PokerHand ph = evaluateHand(preview);
            if (ph != PokerHand::None)
                buf << "\n  " << Color::BOLD << Color::YELLOW
                << "\xe2\x98\x85 " << pokerHandToString(ph)
                << "  x" << pokerHandMultiplier(ph)
                << Color::RESET << "\n";
        }
    }

    buf << Color::DIM << "-------------------------------------------------\n" << Color::RESET;
    buf << "  " << ui.log << "\n";
    buf << "  \xeb\x8d\xb1:" << ui.deckSz << "  \xeb\xb2\x84\xeb\xa6\xbc:" << ui.discardSz << "\n"; // 덱: 버림:
    buf << Color::BOLD << "=================================================\n" << Color::RESET;
    std::cout << buf.str() << std::flush;
}

static void animPlayerAttack(BattleUI ui, int dmg) {
    ui.log = std::string(Color::YELLOW) + Color::BOLD
        + "\xeb\x82\x98  ---->  " + ui.enemyName + "  !!!" + Color::RESET; // 나
    ui.enemyHit = false; drawBattleScreen(ui); sleepMs(160);
    for (int i = 0; i < 2; ++i) {
        ui.enemyHit = true; ui.hitDmg = dmg; drawBattleScreen(ui); sleepMs(170);
        ui.enemyHit = false;                  drawBattleScreen(ui); sleepMs(80);
    }
    ui.enemyHit = true; ui.hitDmg = dmg; drawBattleScreen(ui); sleepMs(260);
}

static void animEnemyAttack(BattleUI ui, int dmg) {
    ui.log = std::string(Color::RED) + Color::BOLD
        + ui.enemyName + "  ---->  \xeb\x82\x98  !!!" + Color::RESET; // 나
    ui.playerHit = false; drawBattleScreen(ui); sleepMs(160);
    for (int i = 0; i < 2; ++i) {
        ui.playerHit = true; ui.playerDmg = dmg; drawBattleScreen(ui); sleepMs(170);
        ui.playerHit = false;                      drawBattleScreen(ui); sleepMs(80);
    }
    ui.playerHit = true; ui.playerDmg = dmg; drawBattleScreen(ui); sleepMs(260);
}

static BattleUI makeUI(const Player& player, const MonsterBase& monster,
    const std::string& log, const std::vector<int>* sel = nullptr)
{
    BattleUI ui;
    ui.enemyName = monster.name();
    ui.enemyGlyph = monster.glyph();
    ui.enemyHp = monster.getHp();
    ui.enemyMaxHp = monster.getMaxHp();
    ui.enemyAtk = monster.getAttack();
    ui.enemyDef = monster.getDefense();
    ui.enemySuit = monster.getSuit();
    ui.playerHp = player.getHp();
    ui.playerMaxHp = player.getMaxHp();
    ui.playerAtk = player.getAttack();
    ui.playerDef = player.getDefense();
    ui.playerSuit = player.getSuit();
    ui.hand = &player.getDeck().getHand();
    ui.selected = sel;
    ui.deckSz = player.getDeck().deckSize();
    ui.discardSz = player.getDeck().discardSize();
    ui.permCount = player.getDeck().permCount();
    ui.permMax = Deck::MAX_PERM;
    ui.log = log;
    return ui;
}

// ===========================================================================
// 카드 보상
// ===========================================================================
static void cardRewardScreen(Player& player, const CardReward& reward, std::mt19937&) {
    int cursor = 0;
    while (true) {
        clearScreen();
        std::cout << Color::BOLD
            << "=================================================\n"
            << "    *** \xec\xa0\x84\xed\x88\xac \xec\x8a\xb9\xeb\xa6\xac! \xec\xb9\xb4\xeb\x93\x9c \xeb\xb3\xb4\xec\x83\x81 ***\n" // 전투 승리! 카드 보상
            << "=================================================\n"
            << Color::RESET << "\n"
            << "  " << Color::DIM
            << "W/S: \xec\x9d\xb4\xeb\x8f\x99   [D] \xeb\x8d\xb1\xec\xb6\x94\xea\xb0\x80   [F] \xea\xb3\xa0\xec\xa0\x95\xec\x9e\xa5\xec\xb0\xa9\n" // W/S: 이동   [D] 덱추가   [F] 고정장착
            << Color::RESET << "\n";

        for (int i = 0; i < (int)reward.choices.size(); ++i) {
            const Card& c = reward.choices[i];
            bool s = (cursor == i);
            std::cout << (s ? (std::string(Color::YELLOW) + Color::BOLD + "  > ") : "    ");
            std::cout << cardColor(c.effect) << cardDisplay(c) << Color::RESET;
            std::cout << "   (";
            switch (c.effect) {
            case CardEffect::Attack:    std::cout << "\xea\xb3\xb5\xea\xb2\xa9 " << c.value << " \xeb\x8d\xb0\xeb\xaf\xb8\xec\xa7\x80"; break; // 공격 N 데미지
            case CardEffect::Defend:    std::cout << "\xeb\xb0\xa9\xec\x96\xb4 +" << c.value; break;                                          // 방어 +N
            case CardEffect::Heal:      std::cout << "\xed\x9a\x8c\xeb\xb3\xb5 +" << c.value; break;                                          // 회복 +N
            case CardEffect::DrawCard:  std::cout << "\xeb\x93\x9c\xeb\xa1\x9c\xec\x9a\xb0 +" << c.value << "\xec\x9e\xa5"; break;             // 드로우 +N장
            case CardEffect::DoubleAtk: std::cout << "\xeb\x8b\xa4\xec\x9d\x8c \xea\xb3\xb5\xea\xb2\xa9 2\xeb\xb0\xb0"; break;               // 다음 공격 2배
            }
            std::cout << " | " << suitToString(c.suit) << ")\n";
        }
        bool passSel = (cursor == (int)reward.choices.size());
        std::cout << (passSel
            ? (std::string(Color::DIM) + Color::BOLD + "  > \xed\x8c\xa8\xec\x8a\xa4\n") // 패스
            : "    \xed\x8c\xa8\xec\x8a\xa4\n") << Color::RESET;

        std::cout << "\n  " << Color::CYAN
            << "\xea\xb3\xa0\xec\xa0\x95 \xec\x8a\xac\xeb\xa1\xaf: " // 고정 슬롯:
            << player.getDeck().permCount() << "/" << Deck::MAX_PERM << Color::RESET << "\n";

        int key = readKey();
        if (key == KEY_UP) { cursor = (cursor == 0) ? (int)reward.choices.size() : cursor - 1; continue; }
        if (key == KEY_DOWN) { cursor = (cursor >= (int)reward.choices.size()) ? 0 : cursor + 1; continue; }
        if ((key == KEY_ENTER && passSel) || key == '0') return;

        int ci = cursor;
        if (ci < 0 || ci >= (int)reward.choices.size()) continue;

        if (key == KEY_ENTER || key == 'd' || key == 'D') {
            player.receiveCard(reward.choices[ci]);
            clearScreen();
            std::cout << Color::GREEN << Color::BOLD
                << "\n  \xec\xb9\xb4\xeb\x93\x9c \xed\x9a\x8d\xeb\x93\x97: " // 카드 획득:
                << cardDisplay(reward.choices[ci]) << " -> \xeb\x8d\xb1 \xec\xb6\x94\xea\xb0\x80\n" << Color::RESET; // -> 덱 추가
            sleepMs(900); return;
        }
        if (key == 'f' || key == 'F') {
            if (!player.getDeck().canEquipMore()) {
                std::cout << Color::RED << "\n  \xea\xb3\xa0\xec\xa0\x95 \xec\x8a\xac\xeb\xa1\xaf \xea\xb0\x80\xeb\x93\x9d!\n" << Color::RESET; // 고정 슬롯 가득!
                sleepMs(1000); continue;
            }
            player.getDeck().equipPermanent(reward.choices[ci]);
            clearScreen();
            std::cout << Color::CYAN << Color::BOLD
                << "\n  \xea\xb3\xa0\xec\xa0\x95 \xec\x9e\xa5\xec\xb0\xa9: " // 고정 장착:
                << cardDisplay(reward.choices[ci]) << "\n" << Color::RESET;
            sleepMs(900); return;
        }
    }
}

// ===========================================================================
// 몬스터 클래스 (main.cpp 전용)
// ===========================================================================

// 강화 고블린
class StrongGoblin final : public MonsterBase {
public:
    StrongGoblin(int x, int y, Suit suit = Suit::None)
        : MonsterBase(x, y, suit, 18, 7, 2, 15) {
    }
    std::string name()  const override { return "Goblin+"; }
    char        glyph() const override { return 'G'; }
    void onTurn(Player& player, Map& map) override;
};

// 보스 종류
enum class BossKind { Jack, Queen, King, Joker };

class BossMonster final : public MonsterBase {
public:
    BossMonster(int x, int y, BossKind kind, Suit suit)
        : MonsterBase(x, y, suit, bossHp(kind), bossAtk(kind), bossDef(kind), bossExp(kind))
        , kind_(kind) {
    }

    std::string name() const override {
        switch (kind_) {
        case BossKind::King:  return "K King";
        case BossKind::Queen: return "Q Queen";
        case BossKind::Jack:  return "J Jack";
        default:              return "JOKER";
        }
    }
    char glyph() const override {
        switch (kind_) {
        case BossKind::King:  return 'K';
        case BossKind::Queen: return 'Q';
        case BossKind::Jack:  return 'J';
        default:              return '*';
        }
    }
    void onTurn(Player& player, Map& map) override {
        if (!isAlive()) return;
        int px = player.getX(), py = player.getY();
        if (std::abs(getX() - px) + std::abs(getY() - py) == 1) {
            attackPlayer(player); return;
        }
        int dx = (px > getX()) ? 1 : (px < getX() ? -1 : 0);
        int dy = (py > getY()) ? 1 : (py < getY() ? -1 : 0);
        if (dx != 0 && tryMove(getX() + dx, getY(), map)) return;
        if (dy != 0 && tryMove(getX(), getY() + dy, map)) return;
    }
private:
    BossKind kind_;
    static int bossHp(BossKind k) { switch (k) { case BossKind::King:return 45; case BossKind::Queen:return 60; case BossKind::Jack:return 80; default:return 120; } }
    static int bossAtk(BossKind k) { switch (k) { case BossKind::King:return 12; case BossKind::Queen:return 15; case BossKind::Jack:return 18; default:return 22; } }
    static int bossDef(BossKind k) { switch (k) { case BossKind::King:return 5;  case BossKind::Queen:return 7;  case BossKind::Jack:return 9;  default:return 12; } }
    static int bossExp(BossKind k) { switch (k) { case BossKind::King:return 30; case BossKind::Queen:return 50; case BossKind::Jack:return 70; default:return 150; } }
};

void StrongGoblin::onTurn(Player& player, Map& map) {
    if (!isAlive()) return;
    int px = player.getX(), py = player.getY();
    if (std::abs(x_ - px) + std::abs(y_ - py) == 1) { attackPlayer(player); return; }
    static std::mt19937 rng{ std::random_device{}() };
    int dirs[4][2] = { {0,-1},{0,1},{-1,0},{1,0} };
    std::shuffle(std::begin(dirs), std::end(dirs), rng);
    for (const auto& d : dirs) {
        int nx = x_ + d[0], ny = y_ + d[1];
        if (nx == px && ny == py) continue;
        if (tryMove(nx, ny, map)) break;
    }
}

// ===========================================================================
// 보스 패턴 데미지
// ===========================================================================
static int calcBossDamage(MonsterBase& m, const Player& p, int turn, std::string& note) {
    int dmg = m.calcDamage(p.getDefense(), p.getSuit());
    char g = m.glyph();
    if (g == 'K') {
        if (turn % 3 == 0) { dmg += 8; note = " [\xec\x99\x95\xec\x9d\x98 \xec\x9d\xbc\xea\xb2\xa9]"; } // 왕의 일격
    }
    else if (g == 'Q') {
        if (m.getHp() <= m.getMaxHp() / 2 && turn % 3 == 1) { m.heal(8); note = " [\xed\x9a\x8c\xeb\xb3\xb5]"; } // 회복
        else dmg += 3;
    }
    else if (g == 'J') {
        if (turn % 2 == 0) { dmg += std::max(1, dmg / 2); note = " [\xec\x97\xb0\xec\x86\x8d\xea\xb3\xb5\xea\xb2\xa9]"; } // 연속공격
    }
    else if (g == '*') {
        int ph = turn % 4;
        if (ph == 0) { m.heal(10); note = " [Joker\xed\x9a\x8c\xeb\xb3\xb5]"; }            // Joker회복
        else if (ph == 1) { dmg += 6;   note = " [\xec\x9a\xb0\xeb\x9e\x91\xed\x83\x80]"; }     // 우랑타
        else if (ph == 2) { dmg += std::max(1, dmg / 2); note = " [\xec\x9d\xb4\xec\xa4\x91\xed\x83\x80]"; } // 이중타
        else { dmg += 12;  note = " [\xec\x98\xb5\xec\x82\xac\xec\x9d\x98\xeb\x82\xac\xeb\x8f\x99]"; } // 옵사의난동
    }
    return std::max(1, dmg);
}

// ===========================================================================
// 가방 화면: 드로우+버림더미에서 원하는 카드를 골라 핸드로 교체
//   반환값: true  = 카드 교체 완료 (턴 소모)
//           false = 취소 (턴 소모 없음)
// ===========================================================================
// ===========================================================================
// 가방 화면 (두 패널)
//   패널 0 = 가방(드로우+버림),  패널 1 = 현재 핸드
//   Tab       : 패널 전환
//   W/S       : 커서 이동
//   Space     : 카드 마크/해제
//   Enter     : 교체 실행
//     - 가방 1장 + 핸드 1장 마크 → 1:1 교체 (턴 소모 없이 반복 가능)
//     - 가방 N장 + 핸드 마크 없음 → 전체 교체 (턴 소모)
//   Q         : 나가기 (변경 없으면 턴 소모 없음, 있으면 소모)
// ===========================================================================
static bool bagPickScreen(Player& player) {

    // Space 를 KEY_ENTER 로 변환하지 않는 전용 키 읽기
    auto readBagKey = []() -> int {
        int ch = _getch();
        if (ch == 0 || ch == 224) {
            int a = _getch();
            if (a == 72) return KEY_UP;
            if (a == 80) return KEY_DOWN;
            return ch;
        }
        if (ch == 'w' || ch == 'W') return KEY_UP;
        if (ch == 's' || ch == 'S') return KEY_DOWN;
        return ch; // Space(32), Enter(13) 그대로
        };

    int  activePanel = 0;   // 0=가방, 1=핸드
    int  bagCursor = 0;
    int  handCursor = 0;
    int  markBag = -1;  // 1:1 교체용 가방 마크 (-1=없음)
    int  markHand = -1;  // 1:1 교체용 핸드 마크 (-1=없음)
    std::vector<int> fullSel;// 전체교체용 다중 선택
    bool anySwapped = false;
    std::string statusMsg;

    while (true) {
        const auto& dp = player.getDeck().drawPile();
        const auto& dsc = player.getDeck().discardPile();
        const auto& hand = player.getDeck().getHand();
        const int   bagTotal = (int)(dp.size() + dsc.size());
        const int   handTotal = (int)hand.size();
        // 고정 슬롯 제외 핸드 최대 교체 수
        const int   maxFull = Deck::HAND_SIZE - player.getDeck().permCount();

        // 커서 범위 보정
        if (bagTotal == 0) bagCursor = 0;
        else if (bagCursor >= bagTotal)  bagCursor = bagTotal - 1;
        if (handTotal == 0) handCursor = 0;
        else if (handCursor >= handTotal) handCursor = handTotal - 1;
        // 마크 범위 보정
        if (markBag >= bagTotal)  markBag = -1;
        if (markHand >= handTotal) markHand = -1;

        // ── 화면 그리기 ───────────────────────────────────────────────
        clearScreen();

        // 헤더
        std::cout << Color::BOLD
            << "==================================================\n"
            << "  [\xea\xb0\x80\xeb\xb0\xa9]"                              // [가방]
            << "  Tab:\xed\x8c\xa8\xeb\x84\x90\xec\xa0\x84\xed\x99\x98"    // Tab:패널전환
            << "  W/S:\xec\x9d\xb4\xeb\x8f\x99"                            // W/S:이동
            << "  Space:\xeb\xa7\x88\xed\x81\xac"                          // Space:마크
            << "  Enter:\xea\xb5\x90\xec\xb2\xb4"                          // Enter:교체
            << "  Q:\xeb\x82\x98\xea\xb0\x80\xea\xb8\xb0\n"               // Q:나가기
            << "==================================================\n"
            << Color::RESET;

        // 모드 힌트
        std::cout << Color::DIM
            << "  \xea\xb0\x80\xeb\xb0\xa9 1\xec\x9e\xa5 + \xed\x95\xb8\xeb\x93\x9c 1\xec\x9e\xa5 \xeb\xa7\x88\xed\x81\xac \xed\x9b\x84 Enter"
            // 가방 1장 + 핸드 1장 마크 후 Enter
            << " = \033[32m1:1 \xea\xb5\x90\xec\xb2\xb4\033[0m"            // = 1:1 교체
            << Color::DIM
            << "  /  \xea\xb0\x80\xeb\xb0\xa9\xeb\xa7\x8c N\xec\x9e\xa5 \xec\x84\xa0\xed\x83\x9d \xed\x9b\x84 Enter"
            // 가방만 N장 선택 후 Enter
            << " = \033[33m\xec\xb8\xa0\xec\xb2\xb4 \xea\xb5\x90\xec\xb2\xb4\033[0m\n"  // 전체 교체
            << Color::RESET;

        if (!statusMsg.empty()) {
            std::cout << "  " << Color::YELLOW << statusMsg << Color::RESET << "\n";
            statusMsg.clear();
        }

        // ── 핸드 패널 ────────────────────────────────────────────────
        bool handActive = (activePanel == 1);
        std::cout << "\n"
            << (handActive ? std::string("\033[1;36m") : std::string(Color::DIM))
            << "[ \xed\x98\x84\xec\x9e\xac \xed\x95\xb8\xeb\x93\x9c ]"    // [ 현재 핸드 ]
            << (handActive ? " <<" : "")
            << Color::RESET << "\n";

        if (handTotal == 0) {
            std::cout << "  (\xeb\xb9\x84\xec\x96\xb4\xec\x9e\x88\xec\x9d\x8c)\n"; // (비어있음)
        }
        else {
            for (int i = 0; i < handTotal; ++i) {
                const Card& c = hand[i];
                bool isCur = handActive && (i == handCursor);
                bool isMark = (i == markHand);
                bool isPerm = c.isPermanent;

                if (isCur && isMark)
                    std::cout << "\033[1;33m  >[H] ";
                else if (isCur)
                    std::cout << "\033[1;36m  >    ";
                else if (isMark)
                    std::cout << "\033[1;33m  [H]  ";
                else
                    std::cout << "       ";

                std::cout << (i + 1) << ": "
                    << cardColor(c.effect) << cardDisplay(c) << Color::RESET;
                if (isPerm)
                    std::cout << Color::CYAN << " \xe2\x98\x85\xea\xb3\xa0\xec\xa0\x95" << Color::RESET; // ★고정
                if (isMark)
                    std::cout << Color::YELLOW << " \xe2\x86\x90\xea\xb5\x90\xec\xb2\xb4\xeb\x8c\x80\xec\x83\x81" << Color::RESET; // ←교체대상
                std::cout << Color::RESET << "\n";
            }
        }

        // ── 가방 패널 ────────────────────────────────────────────────
        bool bagActive = (activePanel == 0);
        std::cout << Color::DIM
            << "--------------------------------------------------\n"
            << Color::RESET
            << (bagActive ? std::string("\033[1;33m") : std::string(Color::DIM))
            << "[ \xea\xb0\x80\xeb\xb0\xa9 ]  "                            // [ 가방 ]
            << "\xeb\x93\x9c\xeb\xa1\x9c\xec\x9a\xb0: " << (int)dp.size()  // 드로우:
            << "  \xeb\xb2\x84\xeb\xa6\xbc: " << (int)dsc.size()           // 버림:
            << (bagActive ? " <<" : "")
            << Color::RESET << "\n";

        if (bagTotal == 0) {
            std::cout << "  (\xec\xb9\xb4\xeb\x93\x9c \xec\x97\x86\xec\x9d\x8c)\n"; // (카드 없음)
        }
        else {
            for (int i = 0; i < bagTotal; ++i) {
                const Card& c = (i < (int)dp.size())
                    ? dp[i] : dsc[i - (int)dp.size()];
                bool isCur = bagActive && (i == bagCursor);
                bool isMark1 = (i == markBag);
                bool isFullSel = std::find(fullSel.begin(), fullSel.end(), i) != fullSel.end();

                if (isCur && isMark1)
                    std::cout << "\033[1;33m  >[B] ";
                else if (isCur && isFullSel)
                    std::cout << "\033[1;32m  >[V] ";
                else if (isCur)
                    std::cout << "\033[1;33m  >    ";
                else if (isMark1)
                    std::cout << "\033[1;33m  [B]  ";
                else if (isFullSel)
                    std::cout << "\033[1;32m  [V]  ";
                else
                    std::cout << "       ";

                std::cout << (i + 1) << ": "
                    << cardColor(c.effect) << cardDisplay(c) << Color::RESET;

                if (i < (int)dp.size())
                    std::cout << Color::DIM << " (\xeb\x93\x9c\xeb\xa1\x9c\xec\x9a\xb0)" << Color::RESET; // (드로우)
                else
                    std::cout << Color::DIM << " (\xeb\xb2\x84\xeb\xa6\xbc)" << Color::RESET; // (버림)
                if (isMark1)
                    std::cout << Color::YELLOW << " \xe2\x86\x92\xea\xb5\x90\xec\xb2\xb4\xeb\x8c\x80\xec\x83\x81" << Color::RESET; // →교체대상
                std::cout << Color::RESET << "\n";
            }
        }

        // 전체교체 선택 현황
        if (!fullSel.empty())
            std::cout << "\n  " << Color::GREEN
            << "\xec\xa0\x84\xec\xb2\xb4\xea\xb5\x90\xec\xb2\xb4 \xec\x84\xa0\xed\x83\x9d: " // 전체교체 선택:
            << (int)fullSel.size() << "/" << maxFull << "\xec\x9e\xa5"  // N/M장
            << Color::RESET << "\n";

        // ── 입력 처리 ─────────────────────────────────────────────────
        int key = readBagKey();

        // Q: 나가기
        if (key == 'q' || key == 'Q') return anySwapped;

        // Tab: 패널 전환
        if (key == '\t') { activePanel ^= 1; continue; }

        // W/S: 커서 이동
        if (key == KEY_UP) {
            if (activePanel == 0 && bagTotal > 0) bagCursor = (bagCursor == 0) ? bagTotal - 1 : bagCursor - 1;
            if (activePanel == 1 && handTotal > 0) handCursor = (handCursor == 0) ? handTotal - 1 : handCursor - 1;
            continue;
        }
        if (key == KEY_DOWN) {
            if (activePanel == 0 && bagTotal > 0) bagCursor = (bagCursor + 1) % bagTotal;
            if (activePanel == 1 && handTotal > 0) handCursor = (handCursor + 1) % handTotal;
            continue;
        }

        // Space: 마크
        if (key == ' ') {
            if (activePanel == 0 && bagTotal > 0) {
                // 가방 패널: 1:1 마크 (markBag) 와 전체선택 (fullSel) 중 현재 상황에 맞게
                // markHand 가 이미 있으면 → 1:1 마크 모드
                if (markHand != -1) {
                    markBag = (markBag == bagCursor) ? -1 : bagCursor;
                }
                else {
                    // fullSel 토글
                    auto it = std::find(fullSel.begin(), fullSel.end(), bagCursor);
                    if (it != fullSel.end()) {
                        fullSel.erase(it);
                        if (markBag == bagCursor) markBag = -1;
                    }
                    else if ((int)fullSel.size() < maxFull) {
                        fullSel.push_back(bagCursor);
                        markBag = bagCursor; // 마지막 선택 = 1:1 후보
                    }
                }
            }
            else if (activePanel == 1 && handTotal > 0) {
                // 핸드 패널
                const Card& hc = hand[handCursor];
                if (!hc.isPermanent)
                    markHand = (markHand == handCursor) ? -1 : handCursor;
                else
                    statusMsg = "\xea\xb3\xa0\xec\xa0\x95 \xec\xb9\xb4\xeb\x93\x9c\xeb\x8a\x94 \xea\xb5\x90\xec\xb2\xb4\xed\x95\xa0 \xec\x88\x98 \xec\x97\x86\xec\x8a\xb5\xeb\x8b\x88\xeb\x8b\xa4."; // 고정 카드는 교체할 수 없습니다.
            }
            continue;
        }

        // Enter: 교체 실행
        if (key == 13 || key == 10) {
            // ① 1:1 교체: 가방 마크 + 핸드 마크 둘 다 있을 때
            if (markBag != -1 && markHand != -1) {
                if (player.getDeck().swapOneFromBag(markHand, markBag)) {
                    anySwapped = true;
                    statusMsg = "\033[32m1:1 \xea\xb5\x90\xec\xb2\xb4 \xec\x99\x84\xeb\xa3\x8c!\033[0m"; // 1:1 교체 완료!
                    // 선택 초기화 (계속 교체 가능)
                    fullSel.erase(std::find_if(fullSel.begin(), fullSel.end(),
                        [&](int v) { return v == markBag; }), fullSel.end());
                    markBag = -1;
                    markHand = -1;
                }
                continue;
            }
            // ② 전체 교체: fullSel 에 카드가 있을 때 (핸드 마크 없음)
            if (!fullSel.empty() && markHand == -1) {
                player.getDeck().swapHandFromBag(fullSel);
                return true; // 전체 교체는 바로 나감 (턴 소모)
            }
            // ③ 아무것도 선택 안 됨
            statusMsg = "\xea\xb5\x90\xec\xb2\xb4\xed\x95\xa0 \xec\xb9\xb4\xeb\x93\x9c\xeb\xa5\xbc \xeb\xa7\x88\xed\x81\xac\xed\x95\x98\xec\x84\xb8\xec\x9a\x94."; // 교체할 카드를 마크하세요.
            continue;
        }
    }
}


// ===========================================================================
// 엔딩 시네마틱: JOKER 처치 → 던전 탈출 → 야외 장면 → 크레딧
// ===========================================================================
static void showEndingCinematic(int finalLevel, int finalExp) {

    // ── Scene 1: JOKER DEFEATED 빅 배너 (깜빡 플래시) ────────────────────
    for (int fi = 0; fi < 7; ++fi) {
        clearScreen();
        if (fi % 2 == 0) {
            std::cout
                << "\n\n\n"
                << "\033[1;33m  +====================================================+\n"
                << "  |                                                    |\n"
                << "  |   \xe2\x98\x85\xe2\x98\x85\xe2\x98\x85  J O K E R   D E F E A T E D  \xe2\x98\x85\xe2\x98\x85\xe2\x98\x85    |\n"
                << "  |                                                    |\n"
                << "  |         DUNGEON  CONQUEST  COMPLETE!               |\n"
                << "  |                                                    |\n"
                << "  +====================================================+\033[0m\n";
        }
        sleepMs(fi < 6 ? 240 : 700);
    }

    // ── Scene 2: 던전 복도 탈출 애니메이션 ───────────────────────────────
    //   내부 너비 IW=50, 횃불 위치 T1=4 / T2=45, 탈출구 오른쪽 끝
    //   각 프레임: 플레이어 위치 + 횃불 깜빡 + 출구 빛(점점 밝아짐)
    static const int IW = 50;     // 복도 내부 너비
    static const int T1 = 4;     // 왼쪽 횃불 열
    static const int T2 = 45;     // 오른쪽 횃불 열

    struct CFrame {
        int         px;           // 플레이어 열 (IW 이상 = 탈출 완료)
        int         glowFrom;     // 이 열부터 출구 빛 시작 (-1 = 없음)
        const char* cap;
        int         ms;
    };
    static const CFrame CF[] = {
        {  1, -1,
           "...JOKER\xeb\xa5\xbc \xec\xa0\x9c\xec\x95\x95\xed\x96\x88\xeb\x8b\xa4...",          560 },
        {  9, -1,
           "\xeb\xb9\x9b\xec\x9d\xb4 \xeb\xb3\xb4\xec\x9d\xb8\xeb\x8b\xa4...!",                520 },
        { 18, 45,
           "\xec\xb6\x9c\xea\xb5\xac\xea\xb0\x80 \xea\xb0\x80\xea\xb9\x8c\xec\x9b\xa8\xec\xa7\x84\xeb\x8b\xa4...", 480 },
        { 28, 42,
           "\xec\xb6\x9c\xea\xb5\xac\xea\xb0\x80 \xea\xb0\x00\xea\xb9\x8c\xec\x9b\xa8\xec\xa7\x84\xeb\x8b\xa4!",   450 },
        { 38, 38,
           "\xec\xa1\xb0\xea\xb8\x88\xeb\xb0\x96\xec\x9d\xb4\xeb\x8b\xa4!",                     420 },
        { IW, 30,
           "\xec\xa7\x80\xea\xb8\x88\xec\x9d\xb4\xec\x95\xbc!!!",                               800 },
    };
    // caption 인덱스 3 수정 (broken byte 수정)
    static const char* CAP3 =
        "\xec\xb6\x9c\xea\xb5\xac\xea\xb0\x80 \xea\xb0\x80\xea\xb9\x8c\xec\x9b\xa8\xec\xa7\x84\xeb\x8b\xa4!"; // 출구가 가까워진다!

    for (int f = 0; f < 6; ++f) {
        const CFrame& fr = CF[f];
        bool torchLit = (f % 2 == 0);   // 횃불 깜빡

        clearScreen();
        std::cout << "\n  \033[2m[ DUNGEON  B10F  -  ESCAPE  ROUTE ]\033[0m\n\n";

        // ─ 복도 상단 벽 ─
        std::cout << "  +" << std::string(IW, '=') << "\033[1;33m>\033[0m\n";

        // ─ 복도 내부 3행 ─
        for (int row = 0; row < 3; ++row) {
            // 각 행을 char 배열로 구성 후 색상 적용하여 출력
            // 기호: ' '=공백  '@'=플레이어  'F'=횃불켜짐  'f'=횃불꺼짐
            //        '.'=약한빛  ':'=중간빛  '*'=강한빛
            std::string row_s(IW, ' ');

            // 횃불 배치 (row 1 = 몸통, row 0 = 불꽃, row 2 = 기둥)
            if (row == 0) {
                row_s[T1] = torchLit ? '^' : ' ';
                row_s[T2] = torchLit ? '^' : ' ';
            }
            else if (row == 1) {
                row_s[T1] = torchLit ? 'F' : 'f';
                row_s[T2] = torchLit ? 'F' : 'f';
            }
            else {
                row_s[T1] = '|';
                row_s[T2] = '|';
            }

            // 출구 빛 (오른쪽에서 왼쪽으로 퍼짐)
            if (fr.glowFrom >= 0) {
                for (int c = fr.glowFrom; c < IW; ++c) {
                    int d = IW - 1 - c;            // 0 = 출구 바로 옆
                    char g = (d <= 1) ? '*' : (d <= 3 ? ':' : '.');
                    if (row_s[c] == ' ') row_s[c] = g;
                }
            }

            // 플레이어 (row 1)
            if (row == 1 && fr.px < IW) row_s[fr.px] = '@';

            // 출력
            std::cout << "  |";
            for (int c = 0; c < IW; ++c) {
                char ch = row_s[c];
                switch (ch) {
                case '@': std::cout << "\033[1;36m@\033[0m";  break;
                case 'F': std::cout << "\033[1;33mf\033[0m";  break; // 밝은 횃불
                case 'f': std::cout << "\033[2;33mi\033[0m";  break; // 어두운 횃불
                case '^': std::cout << "\033[1;33m^\033[0m";  break;
                case '|': std::cout << "\033[33m|\033[0m";    break;
                case '*': std::cout << "\033[1;33m*\033[0m";  break;
                case ':': std::cout << "\033[33m:\033[0m";    break;
                case '.': std::cout << "\033[2;33m.\033[0m";  break;
                default:  std::cout << ' ';
                }
            }
            // 오른쪽 출구기둥 / 탈출한 플레이어
            if (row == 1 && fr.px >= IW)
                std::cout << "\033[1;36m@\033[0m\n";
            else
                std::cout << "\033[1;33m>\033[0m\n";
        }

        // ─ 복도 하단 벽 ─
        std::cout << "  +" << std::string(IW, '=') << "\033[1;33m>\033[0m\n";
        std::cout << "\n  \033[1;33m" << (f == 3 ? CAP3 : fr.cap) << "\033[0m\n";
        sleepMs(fr.ms);
    }

    // ── Scene 3: 탈출 순간 화이트 플래시 ────────────────────────────────
    for (int i = 0; i < 6; ++i) {
        clearScreen();
        if (i % 2 == 0) {
            for (int bl = 0; bl < 12; ++bl) std::cout << "\n";
            std::cout
                << "             \033[1;37m*  *  *  *  *  *  *  *  *\033[0m\n"
                << "          \033[1;37m*                             *\033[0m\n"
                << "          \033[1;37m*    \xed\x83\x88\xec\xb6\x9c \xec\x84\xb1\xea\xb3\xb5!    *\033[0m\n" // 탈출 성공!
                << "          \033[1;37m*                             *\033[0m\n"
                << "             \033[1;37m*  *  *  *  *  *  *  *  *\033[0m\n";
        }
        sleepMs(170);
    }
    sleepMs(350);

    // ── Scene 4: 던전 밖 장면 (2프레임 — 플레이어 살짝 이동) ─────────────
    // 구성: 하늘/별 → 태양 → 산 → 나무+초원 → [던전입구] & [플레이어] → 대사

    auto printOutdoorScene = [](int playerShift) {
        clearScreen();
        std::cout
            // 하늘
            << "\n"
            << "   \033[1;33m*  \xc2\xb7   *    \xc2\xb7    *   \xc2\xb7    *   \xc2\xb7    *   \xc2\xb7   *\033[0m\n"
            << " \033[1;33m\xc2\xb7    *     \xc2\xb7       *     \xc2\xb7      *     \xc2\xb7      *\033[0m\n"
            << "\n"
            // 태양
            << "                 \033[1;33m\\   |   /\033[0m\n"
            << "              \033[1;33m~~~~(\xe2\x98\x80)~~~~\033[0m\n"   // ☀
            << "                 \033[1;33m/   |   \\\033[0m\n"
            << "\n"
            // 산
            << "  \033[1;34m ^    ^        ^    ^  ^         ^     ^    ^\033[0m\n"
            << " \033[1;34m^^^ ^^^^  ^^^  ^^^  ^^^  ^^^  ^^^   ^^^  ^^^  ^^^\033[0m\n"
            // 초원
            << " \033[32m~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\033[0m\n"
            << " \033[32m~ ~ ~ ~ ~ ~ ~  ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~\033[0m\n"
            << "\n";

        // 던전 입구 (왼쪽) + 플레이어 (오른쪽) 같은 줄
        //   던전입구: col 3~16  플레이어: col 40+playerShift
        int ps = 40 + playerShift;
        std::string sp(ps > 16 ? ps - 16 : 1, ' ');

        std::cout
            << "    \033[2m___________\033[0m\n"
            << "   \033[2m|\033[0m \033[1;34mD U N G E O N\033[0m \033[2m|\033[0m" << sp << "\033[1;36mO\033[0m\n"
            << "   \033[2m|  E N T R Y  |\033[0m" << std::string(sp.size(), ' ') << "\033[1;36m/|\\\033[0m\n"
            << "   \033[2m|_____________|\033[0m" << std::string(sp.size(), ' ') << "\033[1;36m/ \\\033[0m\n"
            << "   \033[2m|  [ EXIT ] |\033[0m\n"
            // 지면
            << " \033[32m====================================================\033[0m\n"
            << "\n"
            // 대사
            << "     \033[1;37m\"\xeb\x93\x9c\xeb\x94\x94\xec\x96\xb4... \xec\x9e\x90\xec\x9c\xa0\xeb\x8b\xa4.\"\033[0m\n\n"; // 드디어... 자유다.
        };

    // 4프레임 — 플레이어가 던전 입구에서 조금씩 멀어짐
    for (int w = 0; w < 4; ++w) {
        printOutdoorScene(w * 2);
        sleepMs(700);
    }
    sleepMs(800);

    // ── Scene 5: 엔딩 크레딧 ─────────────────────────────────────────────
    clearScreen();
    // 크레딧 배경용 조용한 외경
    std::cout
        << "\n"
        << "   \033[1;33m*  \xc2\xb7   *    \xc2\xb7    *   \xc2\xb7    *   \xc2\xb7    *   \xc2\xb7   *\033[0m\n"
        << " \033[1;33m\xc2\xb7    *     \xc2\xb7       *     \xc2\xb7      *     \xc2\xb7      *\033[0m\n"
        << "\n"
        << "              \033[1;33m~~~~(\xe2\x98\x80)~~~~\033[0m\n\n"
        << " \033[1;34m^^^  ^^^  ^^^  ^^^  ^^^   ^^^  ^^^  ^^^  ^^^\033[0m\n"
        << " \033[32m~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\033[0m\n"
        << "\n"
        // 크레딧 박스
        << "   \033[1;33m+================================================+\033[0m\n"
        << "   \033[1;33m|\033[0m                                                \033[1;33m|\033[0m\n"
        << "   \033[1;33m|\033[0m       \033[1;33mD U N G E O N  C A R D  Q U E S T\033[0m       \033[1;33m|\033[0m\n"
        << "   \033[1;33m|\033[0m                                                \033[1;33m|\033[0m\n"
        << "   \033[1;33m|\033[0m               \033[1;32mC O M P L E T E !\033[0m               \033[1;33m|\033[0m\n"
        << "   \033[1;33m|\033[0m                                                \033[1;33m|\033[0m\n"
        << "   \033[1;33m|\033[0m   \xec\xb5\x9c\xec\xa2\x85 \xeb\xa0\x88\xeb\xb2\xa8  : \033[1;32m" << finalLevel
        << "\033[0m                               \033[1;33m|\033[0m\n"   // 최종 레벨
        << "   \033[1;33m|\033[0m   \xec\xb4\x9d \xea\xb2\xbd\xed\x97\x98\xec\xb9\x98 : \033[1;32m" << finalExp
        << "\033[0m                             \033[1;33m|\033[0m\n"     // 총 경험치
        << "   \033[1;33m|\033[0m                                                \033[1;33m|\033[0m\n"
        << "   \033[1;33m+================================================+\033[0m\n"
        << "\n   \xec\x95\x84\xeb\xac\xb4 \xed\x82\xa4\xeb\x82\x98...\n"; // 아무 키나...

    readKey();
}


static bool runCardBattle(Player& player, MonsterBase& monster, std::mt19937& rng) {
    player.drawHand();
    std::string battleLog = monster.name()
        + std::string(Color::RED) + " \xeb\x93\xb1\xec\x9e\xa5!" + Color::RESET; // 등장!
    std::vector<int> selCards;
    int battleTurn = 1;

    { auto ui = makeUI(player, monster, battleLog); drawBattleScreen(ui); sleepMs(800); }

    while (player.isAlive() && monster.isAlive()) {
        { auto ui = makeUI(player, monster, battleLog, &selCards); drawBattleScreen(ui); }
        battleLog.clear();
        int key = readKey();

        if (key == 'b' || key == 'B') {
            bool used = bagPickScreen(player);
            if (used && monster.isAlive()) {
                // 가방 교체 = 턴 소모 → 적 반격
                std::string note;
                bool isBoss = (monster.glyph() == 'K' || monster.glyph() == 'Q'
                    || monster.glyph() == 'J' || monster.glyph() == '*');
                int monDmg = isBoss
                    ? calcBossDamage(monster, player, battleTurn, note)
                    : std::max(1, monster.calcDamage(player.getDefense(), player.getSuit()));
                player.takeDamage(monDmg);
                { auto ui = makeUI(player, monster, ""); animEnemyAttack(ui, monDmg); }
                battleLog = std::string(Color::CYAN)
                    + "[\xea\xb0\x80\xeb\xb0\xa9] \xed\x95\xb8\xeb\x93\x9c \xea\xb5\x90\xec\xb2\xb4!" // [가방] 핸드 교체!
                    + Color::RESET + "\n"
                    + std::string(Color::RED) + monster.name()
                    + " \xeb\xb0\x98\xea\xb2\xa9! " // 반격!
                    + std::to_string(monDmg) + " \xeb\x8d\xb0\xeb\xaf\xb8\xec\xa7\x80" // 데미지
                    + note + Color::RESET;
                ++battleTurn;
            }
            continue;
        }

        if (key >= '1' && key <= '5') {
            int idx = key - '1';
            const auto& hand = player.getDeck().getHand();
            if (idx < (int)hand.size()) {
                auto it = std::find(selCards.begin(), selCards.end(), idx);
                if (it != selCards.end()) selCards.erase(it);
                else selCards.push_back(idx);
            }
            continue;
        }

        if (key == KEY_ENTER) {
            if (selCards.empty()) {
                battleLog = "\xec\xb9\xb4\xeb\x93\x9c\xeb\xa5\xbc \xec\x84\xa0\xed\x83\x9d\xed\x95\x98\xec\x84\xb8\xec\x9a\x94."; // 카드를 선택하세요.
                continue;
            }
            std::string pokerResult;
            std::string effectLog = BattleSystem::playCards(selCards, player, monster, player.getDeck(), pokerResult);

            bool hasAtk = effectLog.find("\xea\xb3\xb5\xea\xb2\xa9!") != std::string::npos; // 공격!
            if (hasAtk && monster.isAlive()) {
                auto ui = makeUI(player, monster, effectLog);
                animPlayerAttack(ui, 0);
            }
            if (!pokerResult.empty() && pokerResult != "\xec\x97\x86\xec\x9d\x8c") // 없음
                effectLog = std::string(Color::BOLD) + Color::YELLOW
                + "\xe2\x98\x85 " + pokerResult + " \xeb\xb0\x9c\xeb\x8f\x99!\n" // 발동!
                + Color::RESET + effectLog;
            battleLog = effectLog;
            selCards.clear();

            if (monster.isAlive()) {
                std::string note;
                bool isBoss = (monster.glyph() == 'K' || monster.glyph() == 'Q'
                    || monster.glyph() == 'J' || monster.glyph() == '*');
                int monDmg = isBoss
                    ? calcBossDamage(monster, player, battleTurn, note)
                    : std::max(1, monster.calcDamage(player.getDefense(), player.getSuit()));
                player.takeDamage(monDmg);
                { auto ui = makeUI(player, monster, battleLog); animEnemyAttack(ui, monDmg); }
                battleLog += std::string(Color::RED) + monster.name()
                    + " \xeb\xb0\x98\xea\xb2\xa9! " // 반격!
                    + std::to_string(monDmg) + " \xeb\x8d\xb0\xeb\xaf\xb8\xec\xa7\x80" // 데미지
                    + note + Color::RESET;
                ++battleTurn;
            }
            if (player.isAlive() && player.getDeck().getHand().empty()) {
                player.drawHand();
                battleLog += "\n(\xec\x83\x88 \xed\x95\xb8\xeb\x93\x9c \xeb\x93\x9c\xeb\xa1\x9c\xec\x9a\xb0)"; // 새 핸드 드로우
            }
            continue;
        }

        if (key == 's' || key == 'S' || key == KEY_DOWN) {
            player.getDeck().discardAll();
            player.drawHand();
            std::string note;
            bool isBoss = (monster.glyph() == 'K' || monster.glyph() == 'Q'
                || monster.glyph() == 'J' || monster.glyph() == '*');
            int monDmg = isBoss
                ? calcBossDamage(monster, player, battleTurn, note)
                : std::max(1, monster.calcDamage(player.getDefense(), player.getSuit()));
            player.takeDamage(monDmg);
            { auto ui = makeUI(player, monster, ""); animEnemyAttack(ui, monDmg); }
            battleLog = "\xed\x84\xb4 \xeb\x84\x98\xea\xb9\x80. " // 턴 넘김.
                + std::string(Color::RED) + monster.name()
                + " \xea\xb3\xb5\xea\xb2\xa9! " // 공격!
                + std::to_string(monDmg) + " \xeb\x8d\xb0\xeb\xaf\xb8\xec\xa7\x80" // 데미지
                + note + Color::RESET;
            ++battleTurn; selCards.clear(); continue;
        }
    }

    player.getDeck().discardAll();
    { auto ui = makeUI(player, monster, battleLog); drawBattleScreen(ui); }

    if (!player.isAlive()) {
        std::cout << "\n" << Color::RED << Color::BOLD
            << "  === \xec\xa0\x84\xed\x88\xac \xed\x8c\xa8\xeb\xb0\xb0 ===\n" << Color::RESET; // 전투 패배
        sleepMs(300);
        std::cout << "  \xec\x95\x84\xeb\xac\xb4 \xed\x82\xa4\xeb\x82\x98..."; readKey(); // 아무 키나...
        return false;
    }
    std::cout << "\n" << Color::GREEN << Color::BOLD
        << "  === " << monster.name() << " \xec\xb2\x98\xec\xb9\x98! ===\n" << Color::RESET; // 처치!
    sleepMs(300);
    std::cout << "  \xec\x95\x84\xeb\xac\xb4 \xed\x82\xa4\xeb\x82\x98..."; readKey();

    CardReward reward = generateCardReward(1 + player.getLevel() / 2, rng);
    cardRewardScreen(player, reward, rng);
    return true;
}

// ===========================================================================
// 스테이지 시스템
// ===========================================================================
static constexpr int MAP_W = 50;
static constexpr int MAP_H = 22;

struct StageConfig {
    int         normalCount;
    int         strongCount;
    int         bossType;    // 0=없음 1=Jack 2=Queen 3=King 4=Joker
    const char* name;
};

static const StageConfig STAGES[10] = {
    { 3, 0, 0, "\xeb\x8d\xa9\xea\xb5\xb4 \xec\x9e\x85\xea\xb5\xac"                          }, // 던전 입구
    { 0, 4, 0, "\xea\xb9\x8a\xec\x96\xb4\xec\xa7\x80\xeb\x8a\x94 \xec\x9c\x84\xed\x97\x98"  }, // 깊어지는 위험
    { 2, 0, 1, "Jack\xec\x9d\x98 \xed\x8a\xb8\xeb\xa6\xad"                                   }, // Jack의 트릭
    { 0, 4, 0, "\xec\xb9\xa8\xcb\x96\xed\x95\x9c \xed\x8c\x90\xea\xb5\x90"                   }, // 침울한 판교
    { 0, 5, 0, "\xec\xa0\x81\xec\x9d\x98 \xec\x98\x81\xec\x97\xad"                           }, // 적의 영역
    { 2, 0, 2, "Queen\xec\x9d\x98 \xea\xb6\x81\xec\xa0\x84"                                  }, // Queen의 궁전
    { 0, 4, 0, "\xeb\xb6\x88\xed\x83\x80\xeb\x8a\x94 \xea\xb2\x80\xec\x9d\x80 \xec\x88\xb2" }, // 불타는 검은 숲
    { 0, 5, 0, "\xec\xa0\x80\xec\x9a\xb4 \xec\xb9\xa8\xeb\xb3\xb5"                           }, // 저운 침묵
    { 2, 0, 3, "King\xec\x9d\x98 \xec\x9e\xac\xed\x8c\x90"                                   }, // King의 재판
    { 0, 0, 4, "JOKER\xec\x9d\x98 \xec\x9e\xac\xed\x8c\x90"                                  }, // JOKER의 재판
};

static bool isBossStage(int idx) { return STAGES[idx].bossType > 0; }
static bool isFinalStage(int idx) { return idx == 9; }

static void bossCutscene(const MonsterBase& boss) {
    clearScreen();
    char g = boss.glyph();
    std::cout << "\n\n";
    std::cout << Color::BOLD << Color::RED
        << "  +============================================+\n";
    if (g == '*')
        std::cout << Color::BOLD << Color::WHITE
        << "  |   *** \xe2\x98\x85\xe2\x98\x85\xe2\x98\x85 FINAL BOSS \xe2\x98\x85\xe2\x98\x85\xe2\x98\x85 ***       |\n"; // ★★★ FINAL BOSS ★★★
    else
        std::cout << Color::BOLD << Color::YELLOW
        << "  |              BOSS BATTLE!                |\n";
    std::cout << Color::BOLD << Color::RED
        << "  |--------------------------------------------|" << Color::RESET << "\n";

    auto art = getEnemyArt(g, false);
    for (const auto& line : art)
        std::cout << "  " << line << "\n";

    std::cout << "\n  " << Color::BOLD << Color::YELLOW << boss.name() << Color::RESET << "\n";
    std::cout << "  \xec\x86\x8d\xec\x84\xb1: " << suitToString(boss.getSuit()) << "\n"; // 속성:
    std::cout << "  HP:" << boss.getHp()
        << "  ATK:" << boss.getAttack()
        << "  DEF:" << boss.getDefense() << "\n";
    std::cout << Color::BOLD << Color::RED
        << "  +============================================+\n" << Color::RESET;
    std::cout << "\n  \xec\x95\x84\xeb\xac\xb4 \xed\x82\xa4\xeb\x82\x98...\n"; // 아무 키나...
    readKey();
}

static bool stageClearScreen(int idx) {
    clearScreen();
    bool isLast = isFinalStage(idx);
    std::cout << "\n\n  \033[1;32m+==========================================+\033[0m\n";
    if (isLast) {
        std::cout << "  \033[1;33m|   \xe2\x98\x85\xe2\x98\x85\xe2\x98\x85   GAME  CLEAR!   \xe2\x98\x85\xe2\x98\x85\xe2\x98\x85       |\033[0m\n";
        std::cout << "  \033[1;36m|   Joker\xeb\xa5\xbc \xec\xa0\x95\xeb\xb3\xb5\xed\x96\x88\xec\x8a\xb5\xeb\x8b\x88\xeb\x8b\xa4!               |\033[0m\n"; // Joker를 정복했습니다!
    }
    else {
        std::cout << "  \033[1;32m|   STAGE " << (idx + 1) << " CLEAR!                        |\033[0m\n";
        std::cout << "  \033[1;36m|   " << STAGES[idx].name << "  \xed\x81\xb4\xeb\xa6\xac\xec\x96\xb4!              |\033[0m\n"; // 클리어!
    }
    std::cout << "  \033[1;32m+==========================================+\033[0m\n\n";

    if (isLast) {
        std::cout << "  \xec\x95\x84\xeb\xac\xb4 \xed\x82\xa4\xeb\x82\x98...\n"; readKey(); return false; // 아무 키나...
    }
    std::cout << "  Next: Stage " << (idx + 2) << " - " << STAGES[idx + 1].name << "\n\n";
    std::cout << "  [Enter] \xeb\x8b\xa4\xec\x9d\x8c \xec\x8a\xa4\xed\x85\x8c\xec\x9d\xb4\xec\xa7\x80  |  [Q] \xec\xa2\x85\xeb\xa3\x8c\n\n"; // 다음 스테이지 | 종료
    while (true) {
        char k = static_cast<char>(_getch());
        if (k == '\r' || k == '\n' || k == ' ') return true;
        if (k == 'q' || k == 'Q')              return false;
    }
}

static void renderMap(const Map& map, const Player& player,
    const std::vector<std::unique_ptr<MonsterBase>>& monsters,
    int stageIdx, const std::string& msg)
{
    clearScreen();
    bool boss = isBossStage(stageIdx);
    std::cout << Color::BOLD << (boss ? Color::YELLOW : Color::GREEN)
        << "=== Stage " << (stageIdx + 1) << ": " << STAGES[stageIdx].name
        << (boss ? "  [BOSS]" : "") << " ===\n" << Color::RESET;

    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            if (player.getX() == x && player.getY() == y) {
                std::cout << Color::CYAN << Color::BOLD << '@' << Color::RESET; continue;
            }
            bool drawn = false;
            for (const auto& m : monsters) {
                if (!m->isAlive() || m->getX() != x || m->getY() != y) continue;
                char g = m->glyph();
                if (g == 'K' || g == 'Q' || g == 'J' || g == '*')
                    std::cout << Color::BOLD << Color::YELLOW << g << Color::RESET;
                else if (g == 'G')
                    std::cout << Color::RED << g << Color::RESET;
                else
                    std::cout << Color::DIM << Color::RED << g << Color::RESET;
                drawn = true; break;
            }
            if (!drawn) {
                char ch = map.at(x, y).toChar();
                if (ch == '.') std::cout << Color::DIM << '.' << Color::RESET;
                else           std::cout << ch;
            }
        }
        std::cout << '\n';
    }

    int alive = 0; bool bossAlive = false;
    for (const auto& m : monsters) {
        if (!m->isAlive()) continue; ++alive;
        char g = m->glyph();
        if (g == 'K' || g == 'Q' || g == 'J' || g == '*') bossAlive = true;
    }

    const auto& deck = player.getDeck();
    std::cout << "\n"
        << Color::CYAN << "[" << suitSym(player.getSuit()) << "]" << Color::RESET
        << " Lv." << player.getLevel()
        << "  HP:" << Color::GREEN << player.getHp() << "/" << player.getMaxHp() << Color::RESET
        << "  ATK:" << player.getAttack() << "  DEF:" << player.getDefense()
        << "  EXP:" << player.getExp()
        << "  \xea\xb3\xa0\xec\xa0\x95:" << deck.permCount() << "/" << Deck::MAX_PERM  // 고정:
        << "  \xeb\x8d\xb1:" << deck.deckSize() << "  \xeb\xb2\x84\xeb\xa6\xbc:" << deck.discardSize() << "\n"; // 덱: 버림:

    std::cout << Color::DIM
        << "\xec\x9d\xb4\xeb\x8f\x99: WASD   \xeb\x8d\xb1: V   \xec\xa2\x85\xeb\xa3\x8c: Q\n" << Color::RESET; // 이동: WASD   덱: V   종료: Q
    std::cout << Color::DIM << "Legend: " << Color::RESET
        << Color::CYAN << "@" << Color::RESET << Color::DIM << "=\xed\x94\x8c\xeb\xa0\x88\xec\x9d\xb4\xec\x96\xb4  " << Color::RESET  // 플레이어
        << Color::RED << "g" << Color::RESET << Color::DIM << "/G=\xea\xb3\xa0\xeb\xb8\x94\xeb\xa6\xb0  " << Color::RESET             // 고블린
        << Color::YELLOW << "J/Q/K/*" << Color::RESET << Color::DIM << "=\xeb\xb3\xb4\xec\x8a\xa4\n" << Color::RESET; // 보스

    std::cout << "\xeb\xaa\xac\xec\x8a\xa4\xed\x84\xb0: " << alive; // 몬스터:
    if (bossAlive) std::cout << Color::BOLD << Color::YELLOW
        << "  [BOSS \xec\xa1\xb4\xec\x9e\xac!]" << Color::RESET; // BOSS 존재!
    std::cout << "\n";

    if (!msg.empty()) std::cout << Color::YELLOW << "  " << msg << Color::RESET << "\n";
}

static void viewDeck(const Player& player) {
    clearScreen();
    const auto& deck = player.getDeck();
    std::cout << Color::BOLD << Color::YELLOW
        << "=== \xeb\x8d\xb1 \xeb\xaa\xa9\xeb\xa1\xa9 (\xec\xb4\x9d " << deck.totalCards() << "\xec\x9e\xa5) ===\n\n" << Color::RESET; // 덱 목록 (총 N장)
    std::cout << Color::CYAN << Color::BOLD
        << "[\xe2\x98\x85 \xea\xb3\xa0\xec\xa0\x95 " << deck.permCount() << "/" << Deck::MAX_PERM << "]\n" << Color::RESET; // [★ 고정 N/3]
    if (deck.permanentSlots().empty())
        std::cout << "  (\xeb\xb9\x84\xec\x96\xb4\xec\x9e\x88\xec\x9d\x8c)\n"; // (비어있음)
    for (const auto& c : deck.permanentSlots())
        std::cout << "  " << cardColor(c.effect) << cardDisplay(c) << Color::RESET << "\n";
    std::cout << "\n" << Color::BOLD
        << "[\xeb\x93\x9c\xeb\xa1\x9c\xec\x9a\xb0 " << deck.deckSize() << "\xec\x9e\xa5]\n" << Color::RESET; // [드로우 N장]
    for (const auto& c : deck.drawPile())
        std::cout << "  " << cardColor(c.effect) << cardDisplay(c) << Color::RESET << "\n";
    std::cout << "\n" << Color::DIM
        << "[\xeb\xb2\x84\xeb\xa6\xbc " << deck.discardSize() << "\xec\x9e\xa5]\n" << Color::RESET; // [버림 N장]
    for (const auto& c : deck.discardPile())
        std::cout << "  " << Color::DIM << cardDisplay(c) << Color::RESET << "\n";
    std::cout << "\n\xec\x95\x84\xeb\xac\xb4 \xed\x82\xa4\xeb\x82\x98..."; readKey(); // 아무 키나...
}

static std::pair<int, int> uniqueFloor(const Map& map, std::mt19937& rng,
    const std::vector<std::pair<int, int>>& used)
{
    for (int i = 0; i < 1000; ++i) {
        auto pos = map.getRandomFloor(rng);
        if (std::none_of(used.begin(), used.end(),
            [&](const std::pair<int, int>& p) { return p == pos; }))
            return pos;
    }
    return map.getRandomFloor(rng);
}

static void loadStage(int idx, std::mt19937& rng, Map& map,
    std::vector<std::unique_ptr<MonsterBase>>& monsters, Player& player)
{
    makeGenerator(GeneratorType::BSP, rng)->generate(map);
    monsters.clear();
    map.clearMonsters();

    std::vector<std::pair<int, int>> placed;
    auto pp = uniqueFloor(map, rng, placed);
    placed.push_back(pp);
    player.setPosition(pp.first, pp.second);

    const StageConfig& sc = STAGES[idx];
    Suit suits[] = { Suit::Spade, Suit::Diamond, Suit::Club, Suit::Heart };
    std::uniform_int_distribution<int> sd(0, 3);

    for (int i = 0; i < sc.normalCount; ++i) {
        auto mp = uniqueFloor(map, rng, placed); placed.push_back(mp);
        monsters.push_back(std::make_unique<BasicMonster>(mp.first, mp.second, suits[sd(rng)]));
        map.addMonsterPos(mp.first, mp.second);
    }
    for (int i = 0; i < sc.strongCount; ++i) {
        auto mp = uniqueFloor(map, rng, placed); placed.push_back(mp);
        monsters.push_back(std::make_unique<StrongGoblin>(mp.first, mp.second, suits[sd(rng)]));
        map.addMonsterPos(mp.first, mp.second);
    }
    if (sc.bossType > 0) {
        auto bp = uniqueFloor(map, rng, placed); placed.push_back(bp);
        BossKind bk; Suit bs;
        switch (sc.bossType) {
        case 1: bk = BossKind::Jack;  bs = Suit::Spade;   break;
        case 2: bk = BossKind::Queen; bs = Suit::Heart;   break;
        case 3: bk = BossKind::King;  bs = Suit::Diamond; break;
        default:bk = BossKind::Joker; bs = Suit::Club;    break;
        }
        monsters.push_back(std::make_unique<BossMonster>(bp.first, bp.second, bk, bs));
        map.addMonsterPos(bp.first, bp.second);
    }
}

static Suit selectSuit() {
    static const char* labels[4] = {
        "\xe2\x99\xa5 Heart   | \xea\xb0\x95: Diamond\xe2\x99\xa6 | \xec\x95\xbd: Spade\xe2\x99\xa0",
        "\xe2\x99\xa6 Diamond | \xea\xb0\x95: Club   \xe2\x99\xa3 | \xec\x95\xbd: Heart\xe2\x99\xa5",
        "\xe2\x99\xa3 Club    | \xea\xb0\x95: Spade  \xe2\x99\xa0 | \xec\x95\xbd: Diamond\xe2\x99\xa6",
        "\xe2\x99\xa0 Spade   | \xea\xb0\x95: Heart  \xe2\x99\xa5 | \xec\x95\xbd: Club\xe2\x99\xa3"
    };
    static const Suit vals[4] = { Suit::Heart, Suit::Diamond, Suit::Club, Suit::Spade };
    int sel = 0;
    while (true) {
        clearScreen();
        std::cout << "================================================\n"
            << "   \xec\x86\x8d\xec\x84\xb1 \xec\x84\xa0\xed\x83\x9d (W/S: \xec\x9d\xb4\xeb\x8f\x99 | Enter: \xed\x99\x95\xec\xa0\x95)\n" // 속성 선택 (W/S: 이동 | Enter: 확정)
            << "================================================\n\n"
            << "  \xec\x83\x81\xec\x84\xb1: \xe2\x99\xa5>\xe2\x99\xa6>\xe2\x99\xa3>\xe2\x99\xa0>\xe2\x99\xa5 (x1.5)  \xec\x97\xad\xec\x83\x81\xec\x84\xb1: x0.75\n\n"; // 상성: | 역상성:
        for (int i = 0; i < 4; ++i) {
            if (i == sel) std::cout << "  \033[1;33m>> " << labels[i] << " <<\033[0m\n";
            else          std::cout << "     " << labels[i] << "\n";
        }
        std::cout << "\n================================================\n";
        char k = static_cast<char>(_getch());
        if (k == 'w' || k == 'W') { if (--sel < 0) sel = 3; }
        else if (k == 's' || k == 'S') { if (++sel > 3) sel = 0; }
        else if (k == '\r' || k == '\n' || k == ' ') break;
    }
    clearScreen();
    std::cout << "\xec\x84\xa0\xed\x83\x9d: \033[1;33m" << labels[sel] << "\033[0m\n\n" // 선택:
        << "\xea\xb2\x8c\xec\x9e\x84 \xec\x8b\x9c\xec\x9e\x91! \xec\x95\x84\xeb\xac\xb4 \xed\x82\xa4\xeb\x82\x98...\n"; // 게임 시작! 아무 키나...
    (void)_getch();
    return vals[sel];
}

// ===========================================================================
// main
// ===========================================================================
int main() {
    initConsole();
    std::mt19937 rng{ std::random_device{}() };

    Suit chosen = selectSuit();

    int    stageIdx = 0;
    Map    map(MAP_W, MAP_H);
    Player player(0, 0, chosen);
    std::vector<std::unique_ptr<MonsterBase>> monsters;

    loadStage(stageIdx, rng, map, monsters, player);
    std::string lastMsg = "Stage 1 \xec\x8b\x9c\xec\x9e\x91! " + std::string(STAGES[0].name); // 시작!

    while (player.isAlive()) {
        bool allDead = std::all_of(monsters.begin(), monsters.end(),
            [](const auto& m) { return !m->isAlive(); });

        if (allDead) {
            bool cont = stageClearScreen(stageIdx);
            if (!cont) {
                // 최종 스테이지 클리어 → 엔딩 시네마틱
                if (isFinalStage(stageIdx))
                    showEndingCinematic(player.getLevel(), player.getExp());
                break;
            }
            ++stageIdx;
            if (stageIdx >= 10) break;
            loadStage(stageIdx, rng, map, monsters, player);
            lastMsg = "Stage " + std::to_string(stageIdx + 1)
                + " \xec\x8b\x9c\xec\x9e\x91!  " + STAGES[stageIdx].name; // 시작!
            continue;
        }

        renderMap(map, player, monsters, stageIdx, lastMsg);
        lastMsg.clear();

        int key = readKey();
        if (key == 'q' || key == 'Q') break;
        if (key == 'v' || key == 'V') { viewDeck(player); continue; }

        // =================================================================
        // [DEBUG] 스테이지 스킵 키: 백틱(`) → 현재 스테이지 몬스터 전멸
        //  테스트 영상 촬영 후 아래 블록 전체를 삭제하고 제출하세요.
        // =================================================================
        if (key == '`') {
            for (auto& m : monsters)
                if (m->isAlive()) m->takeDamage(9999);
            map.clearMonsters();
            lastMsg = std::string(Color::YELLOW)
                + "[DEBUG] \xec\x8a\xa4\xed\x85\x8c\xec\x9d\xb4\xec\xa7\x80 \xec\x8a\xa4\xed\x82\xb5!" // 스테이지 스킵!
                + Color::RESET;
            continue;
        }
        // =================================================================
        // [DEBUG END]
        // =================================================================

        int dx = 0, dy = 0;
        if (key == 'w' || key == 'W' || key == KEY_UP)   dy = -1;
        else if (key == 's' || key == 'S' || key == KEY_DOWN)  dy = 1;
        else if (key == 'a' || key == 'A')                     dx = -1;
        else if (key == 'd' || key == 'D')                     dx = 1;
        else continue;

        int nx = player.getX() + dx, ny = player.getY() + dy;
        if (!map.isWalkable(nx, ny)) continue;

        bool attacked = false;
        for (auto& m : monsters) {
            if (!m->isAlive() || m->getX() != nx || m->getY() != ny) continue;
            attacked = true;
            bool isBoss = (m->glyph() == 'K' || m->glyph() == 'Q'
                || m->glyph() == 'J' || m->glyph() == '*');
            if (isBoss) bossCutscene(*m);
            runCardBattle(player, *m, rng);
            if (!player.isAlive()) break;
            if (!m->isAlive()) {
                player.gainExp(m->getExpReward());
                lastMsg = m->name() + " \xec\xb2\x98\xec\xb9\x98! +" // 처치! +
                    + std::to_string(m->getExpReward()) + " EXP";
                auto idx = map.monsterIndexAt(m->getX(), m->getY());
                if (idx.has_value()) map.removeMonsterAt(*idx);
            }
            break;
        }

        if (!attacked) player.setPosition(nx, ny);

        if (player.isAlive())
            for (auto& m : monsters)
                if (m->isAlive()) m->onTurn(player, map);
    }

    if (!player.isAlive()) {
        clearScreen();
        std::cout << "\n\n  \033[1;31m"
            << "+===========================================+\n"
            << "  |             GAME  OVER...                 |\n"
            << "  +===========================================+\033[0m\n\n"
            << "  \xec\x95\x84\xeb\xac\xb4 \xed\x82\xa4\xeb\x82\x98...\n"; // 아무 키나...
        (void)_getch();
    }

    std::cout << "\n\xec\xa2\x85\xeb\xa3\x8c. \xec\x95\x84\xeb\xac\xb4 \xed\x82\xa4\xeb\x82\x98...\n"; // 종료. 아무 키나...
    readKey();
    return 0;
}
