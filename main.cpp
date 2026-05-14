// baraja_deckbuilding - 로그라이크 덱빌딩
// Battledemo.cpp의 ASCII 아트 / ANSI 색상 UI 통합
// 고정 슬롯: 전투 보상에서 "덱에 추가" vs "고정 장착" 선택 가능

#include "Map.h"
#include "MapGenerator.h"
#include "Monster.h"
#include "player.h"
#include "Card.h"
#include "BattleSystem.h"
#include "Suit.h"

#include <algorithm>
#include <chrono>
#include <conio.h>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#define NOMINMAX
#include <windows.h>

using namespace Roguelike;

// ===========================================================================
// ANSI 색상 (Battledemo.cpp 동일)
// ===========================================================================
namespace Color {
    constexpr const char* RESET  = "\033[0m";
    constexpr const char* BOLD   = "\033[1m";
    constexpr const char* DIM    = "\033[2m";
    constexpr const char* RED    = "\033[31m";
    constexpr const char* GREEN  = "\033[32m";
    constexpr const char* YELLOW = "\033[33m";
    constexpr const char* BLUE   = "\033[34m";
    constexpr const char* CYAN   = "\033[36m";
    constexpr const char* WHITE  = "\033[37m";
}

// ===========================================================================
// 콘솔 초기화 (Battledemo.cpp의 initConsole)
// ===========================================================================
static void initConsole() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode  = 0;
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

// 화면 지우기 (ANSI 방식, Battledemo.cpp 동일)
static void clearScreen() {
    std::cout << "\033[2J\033[H" << std::flush;
}

// HP 바 (Battledemo.cpp makeHpBar)
static std::string makeHpBar(int hp, int maxHp, int width = 20) {
    if (maxHp <= 0) maxHp = 1;
    int filled = std::max(0, std::min(hp * width / maxHp, width));
    std::string bar = "[";
    for (int i = 0; i < width; ++i) bar += (i < filled ? '#' : '-');
    bar += "]";
    return bar;
}

// 키 입력 (Battledemo.cpp readKey)
static constexpr int KEY_UP    = 300;
static constexpr int KEY_DOWN  = 301;
static constexpr int KEY_ENTER = 302;

static int readKey() {
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
}

// ===========================================================================
// ASCII 아트 (Battledemo.cpp drawEnemyArt / drawPlayerArt)
// ===========================================================================
static void printEnemyArt(std::ostringstream& buf, int offset, bool hit, int dmgNum) {
    std::string pad(offset, ' ');
    buf << (hit ? (std::string(Color::BOLD) + Color::RED) : Color::GREEN);
    buf << pad << "     /\\   /\\\n"
        << pad << "    /  \\_/  \\\n"
        << pad << "   ( o     o )\n"
        << pad << "   |    ^    |\n"
        << pad << "   |  \\___/  |\n"
        << pad << "    \\_______/\n"
        << pad << "    /|     |\\\n"
        << pad << "   (_|_____|_)\n"
        << Color::RESET;
    if (hit && dmgNum > 0)
        buf << Color::BOLD << Color::YELLOW
            << std::string(offset + 3, ' ') << "[ -" << dmgNum << " HP! ]\n"
            << Color::RESET;
    else
        buf << "\n";
}

static void printPlayerArt(std::ostringstream& buf, bool hit, int dmgNum) {
    buf << (hit ? (std::string(Color::BOLD) + Color::RED) : Color::CYAN);
    buf << "            O\n"
        << "           /|\\\n"
        << "           / \\\n"
        << Color::RESET;
    if (hit && dmgNum > 0)
        buf << Color::BOLD << Color::YELLOW
            << "          [ -" << dmgNum << " HP! ]\n"
            << Color::RESET;
    else
        buf << "\n";
}

// ===========================================================================
// 카드 컬러 (효과별)
// ===========================================================================
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
// 배틀 UI 메인 렌더러 (Battledemo.cpp drawBattleUI 확장)
// ===========================================================================
struct BattleUI {
    // 적 이름 / 스탯
    std::string enemyName;
    int enemyHp, enemyMaxHp, enemyAtk, enemyDef;
    Suit enemySuit;
    // 플레이어 스탯
    int playerHp, playerMaxHp, playerAtk, playerDef;
    Suit playerSuit;
    // 핸드
    const std::vector<Card>* hand = nullptr;
    const std::vector<int>*  selected = nullptr;
    // 덱 정보
    int deckSz, discardSz, permCount, permMax;
    // 로그
    std::string log;
    // 애니메이션 플래그
    bool enemyHit  = false; int hitDmg    = 0;
    bool playerHit = false; int playerDmg = 0;
};

static void drawBattleScreen(const BattleUI& ui) {
    std::ostringstream buf;
    buf << "\033[2J\033[H"; // clear

    // 헤더
    buf << Color::BOLD
        << "=================================================\n"
        << "              CARD BATTLE\n"
        << "=================================================\n"
        << Color::RESET << "\n";

    // 적 HP
    buf << Color::RED << Color::BOLD << "  " << ui.enemyName << Color::RESET
        << " " << suitToSymbol(ui.enemySuit)
        << "  HP " << makeHpBar(ui.enemyHp, ui.enemyMaxHp)
        << "  " << ui.enemyHp << "/" << ui.enemyMaxHp
        << "  ATK:" << ui.enemyAtk << "  DEF:" << ui.enemyDef << "\n\n";

    // 적 ASCII 아트
    int eOffset = ui.enemyHit ? 5 : 2;
    printEnemyArt(buf, eOffset, ui.enemyHit, ui.hitDmg);

    // 구분선
    buf << "\n" << Color::DIM
        << "- - - - - - - - - - - - - - - - - - - - - - - -\n"
        << Color::RESET;

    // 플레이어 ASCII 아트
    printPlayerArt(buf, ui.playerHit, ui.playerDmg);

    // 플레이어 HP
    buf << "\n" << Color::CYAN << Color::BOLD << "  [나] " << Color::RESET
        << suitToSymbol(ui.playerSuit)
        << "  HP " << makeHpBar(ui.playerHp, ui.playerMaxHp)
        << "  " << ui.playerHp << "/" << ui.playerMaxHp
        << "  ATK:" << ui.playerAtk << "  DEF:" << ui.playerDef << "\n"
        << "  고정슬롯: " << ui.permCount << "/" << ui.permMax << "\n";

    // 카드 핸드
    buf << "\n" << Color::DIM
        << "=================================================\n"
        << "  [1-5] 선택/해제  [Enter] 사용  [S] 턴넘기기\n"
        << "-------------------------------------------------\n"
        << Color::RESET;

    if (ui.hand) {
        for (int i = 0; i < (int)ui.hand->size(); ++i) {
            const Card& c = (*ui.hand)[i];
            bool sel = ui.selected &&
                       std::find(ui.selected->begin(), ui.selected->end(), i)
                       != ui.selected->end();

            buf << (sel ? (std::string(Color::YELLOW) + Color::BOLD + "  > ")
                        : "    ");
            buf << (i + 1) << ": ";
            buf << cardColor(c.effect) << c.toString() << Color::RESET;
            if (c.isPermanent)
                buf << Color::CYAN << " ★고정" << Color::RESET;
            buf << "\n";
        }

        // 족보 미리보기
        if (ui.selected && !ui.selected->empty()) {
            std::vector<Card> preview;
            for (int idx : *ui.selected)
                if (idx >= 0 && idx < (int)ui.hand->size())
                    preview.push_back((*ui.hand)[idx]);
            PokerHand ph = evaluateHand(preview);
            if (ph != PokerHand::None)
                buf << "\n  " << Color::BOLD << Color::YELLOW
                    << "★ " << pokerHandToString(ph)
                    << "  x" << pokerHandMultiplier(ph)
                    << Color::RESET << "\n";
        }
    }

    buf << Color::DIM << "-------------------------------------------------\n" << Color::RESET;
    buf << "  " << ui.log << "\n";
    buf << "  덱:" << ui.deckSz << "  버림:" << ui.discardSz << "\n";
    buf << Color::BOLD << "=================================================\n" << Color::RESET;

    std::cout << buf.str() << std::flush;
}

// 애니메이션 유틸
static void animPlayerAttack(BattleUI ui, int dmg) {
    std::string chargeLog = std::string(Color::YELLOW) + Color::BOLD
                          + "나  ---->  " + ui.enemyName + "  !!!" + Color::RESET;
    ui.log = chargeLog; ui.enemyHit = false;
    drawBattleScreen(ui); sleepMs(160);

    for (int i = 0; i < 2; ++i) {
        ui.enemyHit = true; ui.hitDmg = dmg;
        drawBattleScreen(ui); sleepMs(170);
        ui.enemyHit = false;
        drawBattleScreen(ui); sleepMs(80);
    }
    ui.enemyHit = true; ui.hitDmg = dmg;
    drawBattleScreen(ui); sleepMs(260);
}

static void animEnemyAttack(BattleUI ui, int dmg) {
    std::string chargeLog = std::string(Color::RED) + Color::BOLD
                          + ui.enemyName + "  ---->  나  !!!" + Color::RESET;
    ui.log = chargeLog; ui.playerHit = false;
    drawBattleScreen(ui); sleepMs(160);

    for (int i = 0; i < 2; ++i) {
        ui.playerHit = true; ui.playerDmg = dmg;
        drawBattleScreen(ui); sleepMs(170);
        ui.playerHit = false;
        drawBattleScreen(ui); sleepMs(80);
    }
    ui.playerHit = true; ui.playerDmg = dmg;
    drawBattleScreen(ui); sleepMs(260);
}

// BattleUI 헬퍼 빌더
static BattleUI makeUI(const Player& player, const MonsterBase& monster,
                       const std::string& log,
                       const std::vector<int>* selected = nullptr) {
    BattleUI ui;
    ui.enemyName   = monster.name();
    ui.enemyHp     = monster.getHp();
    ui.enemyMaxHp  = monster.getMaxHp();
    ui.enemyAtk    = monster.getAttack();
    ui.enemyDef    = monster.getDefense();
    ui.enemySuit   = monster.getSuit();
    ui.playerHp    = player.getHp();
    ui.playerMaxHp = player.getMaxHp();
    ui.playerAtk   = player.getAttack();
    ui.playerDef   = player.getDefense();
    ui.playerSuit  = player.getSuit();
    ui.hand        = &player.getDeck().getHand();
    ui.selected    = selected;
    ui.deckSz      = player.getDeck().deckSize();
    ui.discardSz   = player.getDeck().discardSize();
    ui.permCount   = player.getDeck().permCount();
    ui.permMax     = Deck::MAX_PERM;
    ui.log         = log;
    return ui;
}

// ===========================================================================
// 카드 보상 화면 (덱 추가 or 고정 슬롯 장착 선택)
// ===========================================================================
static void cardRewardScreen(Player& player, const CardReward& reward, std::mt19937&) {
    int cursor = 0; // 0~2: 카드선택, 3: 패스

    while (true) {
        clearScreen();
        std::cout << Color::BOLD
                  << "=================================================\n"
                  << "       *** 전투 승리! 카드 보상 ***\n"
                  << "=================================================\n"
                  << Color::RESET << "\n"
                  << "  " << Color::DIM
                  << "W/S 또는 방향키: 이동   Enter/Space: 선택\n"
                  << Color::RESET << "\n";

        for (int i = 0; i < (int)reward.choices.size(); ++i) {
            const Card& c = reward.choices[i];
            bool sel = (cursor == i);
            std::cout << (sel ? (std::string(Color::YELLOW) + Color::BOLD + "  > ") : "    ");
            std::cout << cardColor(c.effect) << c.toString() << Color::RESET;

            // 효과 설명
            std::cout << "   (";
            switch (c.effect) {
                case CardEffect::Attack:    std::cout << "공격 " << c.value << " 데미지"; break;
                case CardEffect::Defend:    std::cout << "방어 +" << c.value; break;
                case CardEffect::Heal:      std::cout << "회복 +" << c.value; break;
                case CardEffect::DrawCard:  std::cout << "드로우 +" << c.value << "장"; break;
                case CardEffect::DoubleAtk: std::cout << "다음 공격 2배"; break;
            }
            std::cout << " | " << suitToString(c.suit) << ")\n";
        }

        // 패스 옵션
        bool passSel = (cursor == (int)reward.choices.size());
        std::cout << (passSel ? (std::string(Color::DIM) + Color::BOLD + "  > 패스 (카드 없이 계속)\n")
                              : "    패스 (카드 없이 계속)\n")
                  << Color::RESET;

        // 고정 슬롯 안내
        std::cout << "\n  " << Color::CYAN
                  << "고정 슬롯: " << player.getDeck().permCount()
                  << "/" << Deck::MAX_PERM
                  << "  (카드 선택 후 [D]덱추가 / [F]고정장착)\n"
                  << Color::RESET
                  << Color::DIM
                  << "  고정 카드는 매 전투 핸드에 항상 포함, 사용해도 소모 안 됨\n"
                  << Color::RESET;

        int key = readKey();

        if (key == KEY_UP) {
            cursor = (cursor == 0) ? (int)reward.choices.size() : cursor - 1;
            continue;
        }
        if (key == KEY_DOWN) {
            cursor = (cursor >= (int)reward.choices.size()) ? 0 : cursor + 1;
            continue;
        }

        // 패스 선택
        if (key == KEY_ENTER && passSel) return;
        if (key == '0') return;

        // 카드 선택된 상태 — D: 덱, F: 고정
        int cardIdx = cursor; // 0~2
        if (cardIdx < 0 || cardIdx >= (int)reward.choices.size()) continue;

        if (key == KEY_ENTER || key == 'd' || key == 'D') {
            // 덱에 추가
            player.receiveCard(reward.choices[cardIdx]);
            clearScreen();
            std::cout << Color::GREEN << Color::BOLD
                      << "\n  카드 획득: " << reward.choices[cardIdx].toString()
                      << " → 덱에 추가됨\n" << Color::RESET;
            sleepMs(900);
            return;
        }
        if (key == 'f' || key == 'F') {
            // 고정 슬롯 장착
            if (!player.getDeck().canEquipMore()) {
                std::cout << Color::RED
                          << "\n  고정 슬롯이 꽉 찼습니다! (최대 " << Deck::MAX_PERM << "개)\n"
                          << Color::RESET;
                sleepMs(1000);
                continue;
            }
            player.getDeck().equipPermanent(reward.choices[cardIdx]);
            clearScreen();
            std::cout << Color::CYAN << Color::BOLD
                      << "\n  카드 장착: " << reward.choices[cardIdx].toString()
                      << " → 고정 슬롯에 장착! (매 전투 재사용)\n" << Color::RESET;
            sleepMs(900);
            return;
        }
    }
}

// ===========================================================================
// 카드 배틀
// ===========================================================================
static bool runCardBattle(Player& player, MonsterBase& monster, std::mt19937& rng) {
    player.drawHand();
    std::string battleLog = monster.name() + std::string(Color::RED) + " 등장!" + Color::RESET;
    std::vector<int> selectedCards;

    // 등장 연출
    {
        BattleUI ui = makeUI(player, monster, battleLog);
        drawBattleScreen(ui);
        sleepMs(800);
    }

    while (player.isAlive() && monster.isAlive()) {
        {
            BattleUI ui = makeUI(player, monster, battleLog, &selectedCards);
            drawBattleScreen(ui);
        }
        battleLog.clear();

        int key = readKey();

        // 1~5: 카드 선택/해제
        if (key >= '1' && key <= '5') {
            int idx = key - '1';
            const auto& hand = player.getDeck().getHand();
            if (idx < (int)hand.size()) {
                auto it = std::find(selectedCards.begin(), selectedCards.end(), idx);
                if (it != selectedCards.end()) selectedCards.erase(it);
                else selectedCards.push_back(idx);
            }
            continue;
        }

        // Enter: 카드 사용
        if (key == KEY_ENTER) {
            if (selectedCards.empty()) { battleLog = "카드를 선택해주세요."; continue; }

            std::string pokerResult;
            std::string effectLog = BattleSystem::playCards(
                selectedCards, player, monster, player.getDeck(), pokerResult);

            // 공격 애니메이션 (Attack 카드가 있으면)
            bool hasAtk = false;
            for (int idx : selectedCards) {
                const auto& h = player.getDeck().getHand(); // 이미 사용 후 핸드 변경됨
                (void)h;
                // 원본 카드 확인은 effectLog로 대체
                (void)idx;
            }
            // effectLog에 "공격!" 포함되면 애니메이션
            if (effectLog.find("공격!") != std::string::npos) {
                hasAtk = true;
            }

            if (hasAtk && monster.isAlive()) {
                BattleUI ui = makeUI(player, monster, effectLog);
                animPlayerAttack(ui, 0); // 데미지 수치는 로그에 포함
            }

            if (!pokerResult.empty() && pokerResult != "없음")
                effectLog = std::string(Color::BOLD) + Color::YELLOW
                          + "★ " + pokerResult + " 발동!\n"
                          + Color::RESET + effectLog;

            battleLog = effectLog;
            selectedCards.clear();

            // 몬스터 반격
            if (monster.isAlive()) {
                float mult   = getSuitMultiplier(monster.getSuit(), player.getSuit());
                int   monDmg = std::max(1, static_cast<int>(
                    (monster.getAttack() - player.getDefense()) * mult));
                player.takeDamage(monDmg);

                BattleUI ui = makeUI(player, monster, battleLog);
                animEnemyAttack(ui, monDmg);

                battleLog += std::string(Color::RED) + monster.name()
                           + " 반격! " + std::to_string(monDmg) + " 데미지"
                           + Color::RESET;
            }

            // 핸드 소진 시 재드로우
            if (player.isAlive() && player.getDeck().getHand().empty()) {
                player.drawHand();
                battleLog += "\n(새 핸드 드로우)";
            }
            continue;
        }

        // S / KEY_DOWN: 턴 넘기기
        if (key == 's' || key == 'S' || key == KEY_DOWN) {
            player.getDeck().discardAll();
            player.drawHand();
            float mult   = getSuitMultiplier(monster.getSuit(), player.getSuit());
            int   monDmg = std::max(1, static_cast<int>(
                (monster.getAttack() - player.getDefense()) * mult));
            player.takeDamage(monDmg);

            BattleUI ui = makeUI(player, monster, "");
            animEnemyAttack(ui, monDmg);

            battleLog = "턴 넘김. "
                      + std::string(Color::RED) + monster.name()
                      + " 공격! " + std::to_string(monDmg) + " 데미지" + Color::RESET;
            selectedCards.clear();
            continue;
        }
    }

    // 결과
    {
        BattleUI ui = makeUI(player, monster, battleLog);
        drawBattleScreen(ui);
    }

    if (!player.isAlive()) {
        std::cout << "\n" << Color::RED << Color::BOLD
                  << "  === 전투 패배 ===\n" << Color::RESET;
        sleepMs(300);
        std::cout << "  아무 키나...";
        readKey();
        return false;
    }

    std::cout << "\n" << Color::GREEN << Color::BOLD
              << "  === " << monster.name() << " 처치! ===\n" << Color::RESET;
    sleepMs(300);
    std::cout << "  아무 키나...";
    readKey();

    // 카드 보상
    CardReward reward = generateCardReward(1 + player.getLevel() / 2, rng);
    cardRewardScreen(player, reward, rng);
    return true;
}

// ===========================================================================
// 맵 렌더링
// ===========================================================================
constexpr int MAP_W = 50;
constexpr int MAP_H = 22;

static void renderMap(const Map& map,
                      const Player& player,
                      const std::vector<std::unique_ptr<MonsterBase>>& monsters,
                      const std::string& msg)
{
    clearScreen();

    // 타이틀
    std::cout << Color::BOLD << Color::YELLOW
              << "=== BARAJA DECKBUILDING ROGUELIKE ===\n"
              << Color::RESET;

    // 맵
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            if (player.getX() == x && player.getY() == y) {
                std::cout << Color::CYAN << Color::BOLD << '@' << Color::RESET;
                continue;
            }
            bool printed = false;
            for (const auto& m : monsters) {
                if (m->isAlive() && m->getX() == x && m->getY() == y) {
                    std::cout << Color::RED << m->glyph() << Color::RESET;
                    printed = true; break;
                }
            }
            if (!printed) {
                char ch = map.at(x, y).toChar();
                if (ch == '.') std::cout << Color::DIM << ch << Color::RESET;
                else           std::cout << ch;
            }
        }
        std::cout << '\n';
    }

    // HUD
    const auto& deck = player.getDeck();
    std::cout << "\n"
              << Color::CYAN << "[" << suitToSymbol(player.getSuit()) << "]" << Color::RESET
              << " Lv." << player.getLevel()
              << "  HP:" << Color::GREEN << player.getHp() << "/" << player.getMaxHp() << Color::RESET
              << "  ATK:" << player.getAttack()
              << "  DEF:" << player.getDefense()
              << "  EXP:" << player.getExp()
              << "  " << Color::CYAN << "고정:" << deck.permCount() << "/" << Deck::MAX_PERM << Color::RESET
              << "  덱:" << deck.deckSize()
              << "  버림:" << deck.discardSize()
              << "\n"
              << Color::DIM << "이동: WASD   덱보기: V   종료: Q\n" << Color::RESET;

    if (!msg.empty())
        std::cout << Color::YELLOW << "  " << msg << Color::RESET << "\n";
}

// ===========================================================================
// 덱 보기
// ===========================================================================
static void viewDeck(const Player& player) {
    clearScreen();
    const auto& deck = player.getDeck();

    std::cout << Color::BOLD << Color::YELLOW
              << "=== 덱 목록 (총 " << deck.totalCards() << "장) ===\n\n"
              << Color::RESET;

    // 고정 슬롯
    std::cout << Color::CYAN << Color::BOLD
              << "[★ 고정 슬롯 " << deck.permCount() << "/" << Deck::MAX_PERM << "]\n"
              << Color::RESET;
    if (deck.permanentSlots().empty())
        std::cout << "  (비어있음)\n";
    for (const auto& c : deck.permanentSlots())
        std::cout << "  " << cardColor(c.effect) << c.toString() << Color::RESET << "\n";

    // 드로우 파일
    std::cout << "\n" << Color::BOLD
              << "[드로우 파일 " << deck.deckSize() << "장]\n" << Color::RESET;
    for (const auto& c : deck.drawPile())
        std::cout << "  " << cardColor(c.effect) << c.toString() << Color::RESET << "\n";

    // 버림 파일
    std::cout << "\n" << Color::DIM
              << "[버림 파일 " << deck.discardSize() << "장]\n" << Color::RESET;
    for (const auto& c : deck.discardPile())
        std::cout << "  " << Color::DIM << c.toString() << Color::RESET << "\n";

    std::cout << "\n아무 키나...";
    readKey();
}

// ===========================================================================
// 유니크 좌표 생성
// ===========================================================================
static std::pair<int,int> uniqueFloor(
    const Map& map, std::mt19937& rng,
    const std::vector<std::pair<int,int>>& used)
{
    for (int attempt = 0; attempt < 1000; ++attempt) {
        auto pos = map.getRandomFloor(rng);
        bool dup = std::any_of(used.begin(), used.end(),
            [&](const auto& p){ return p == pos; });
        if (!dup) return pos;
    }
    return map.getRandomFloor(rng);
}

// ===========================================================================
// main
// ===========================================================================
int main() {
    initConsole();

    std::mt19937 rng{std::random_device{}()};

    Map map(MAP_W, MAP_H);
    makeGenerator(GeneratorType::BSP, rng)->generate(map);

    std::vector<std::pair<int,int>> placed;
    auto [px, py] = uniqueFloor(map, rng, placed);
    placed.push_back({px, py});
    Player player(px, py, Suit::Heart);

    Suit monSuits[] = {Suit::Spade, Suit::Diamond, Suit::Club, Suit::Heart, Suit::Spade};
    std::vector<std::unique_ptr<MonsterBase>> monsters;
    for (int i = 0; i < 5; ++i) {
        auto [mx, my] = uniqueFloor(map, rng, placed);
        placed.push_back({mx, my});
        monsters.push_back(std::make_unique<BasicMonster>(mx, my, monSuits[i % 4]));
        map.addMonsterPos(mx, my);
    }

    std::string lastMsg;

    while (player.isAlive()) {
        bool allDead = std::all_of(monsters.begin(), monsters.end(),
            [](const auto& m){ return !m->isAlive(); });
        if (allDead) {
            renderMap(map, player, monsters, "");
            std::cout << "\n" << Color::GREEN << Color::BOLD
                      << "  === 모든 몬스터 처치! 승리! ===\n" << Color::RESET;
            break;
        }

        renderMap(map, player, monsters, lastMsg);
        lastMsg.clear();

        int key = readKey();
        if (key == 'q' || key == 'Q') break;
        if (key == 'v' || key == 'V') { viewDeck(player); continue; }

        int dx = 0, dy = 0;
        if      (key == 'w' || key == 'W' || key == KEY_UP)   dy = -1;
        else if (key == 's' || key == 'S' || key == KEY_DOWN)  dy =  1;
        else if (key == 'a' || key == 'A')                     dx = -1;
        else if (key == 'd' || key == 'D')                     dx =  1;
        else continue;

        const int nx = player.getX() + dx;
        const int ny = player.getY() + dy;
        if (!map.isWalkable(nx, ny)) continue;

        bool attacked = false;
        for (auto& m : monsters) {
            if (!m->isAlive() || m->getX() != nx || m->getY() != ny) continue;

            runCardBattle(player, *m, rng);
            if (!player.isAlive()) break;

            if (!m->isAlive()) {
                player.gainExp(m->getExpReward());
                lastMsg = m->name() + " 처치! +" + std::to_string(m->getExpReward()) + " EXP";
                auto idx = map.monsterIndexAt(m->getX(), m->getY());
                if (idx.has_value()) map.removeMonsterAt(*idx);
            }
            attacked = true;
            break;
        }

        if (!attacked) player.setPosition(nx, ny);

        for (auto& m : monsters)
            if (m->isAlive()) m->onTurn(player, map);
    }

    if (!player.isAlive()) {
        renderMap(map, player, monsters, "");
        std::cout << "\n" << Color::RED << Color::BOLD
                  << "  === 게임 오버 ===\n" << Color::RESET;
    }

    std::cout << "\n종료하려면 아무 키나...";
    readKey();
    return 0;
}
