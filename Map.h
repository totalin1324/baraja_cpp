#ifndef ROGUELIKE_MAP_H
#define ROGUELIKE_MAP_H

#include <vector>
#include <stdexcept>
#include <random>
#include <utility>
#include <optional>
#include <algorithm>

namespace Roguelike {

// ---------------------------------------------------------------------------
// TileType: 타일 종류 열거형
// ---------------------------------------------------------------------------
enum class TileType : uint8_t {
    Wall  = 0,
    Floor = 1,
    Door  = 2,
    Void  = 255,
};

// ---------------------------------------------------------------------------
// Tile: 맵의 최소 단위
// 단순 값 타입이므로 모든 메서드를 헤더에 인라인 정의 (성능 이점)
// ---------------------------------------------------------------------------
struct Tile {
    TileType type    { TileType::Wall };
    bool     visited { false };

    [[nodiscard]] constexpr bool isWalkable() const noexcept {
        return type == TileType::Floor || type == TileType::Door;
    }

    [[nodiscard]] constexpr char toChar() const noexcept {
        switch (type) {
            case TileType::Floor: return '.';
            case TileType::Door:  return '+';
            case TileType::Void:  return ' ';
            default:              return '#';
        }
    }
};

// ---------------------------------------------------------------------------
// Point: 2D 좌표 값 타입
// 연산자 오버로드는 constexpr이므로 헤더에 유지
// ---------------------------------------------------------------------------
struct Point {
    int x { 0 };
    int y { 0 };

    constexpr bool operator==(const Point& o) const noexcept {
        return x == o.x && y == o.y;
    }

    constexpr Point operator+(const Point& o) const noexcept {
        return { x + o.x, y + o.y };
    }
};

// ---------------------------------------------------------------------------
// Map: 타일 데이터 컨테이너
//
// 설계 원칙:
//   - 읽기 접근자(at, inBounds, isWalkable)는 헤더 인라인 유지
//   - 쓰기 접근자(atMut)는 friend를 통해 Generator 계층에만 노출
//   - 비용이 있는 연산(getRandomFloor)은 .cpp에서 구현
// ---------------------------------------------------------------------------
class MapGeneratorBase;  // 전방 선언 — friend 선언을 위해 필요

class Map {
public:
    Map(int width, int height);

    // --- 읽기 전용 접근자 (인라인) ---

    [[nodiscard]] int width()  const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }

    [[nodiscard]] const Tile& at(int x, int y) const noexcept {
        if (!inBounds(x, y)) return voidTile_;
        return grid_[static_cast<size_t>(y)][static_cast<size_t>(x)];
    }

    [[nodiscard]] bool inBounds(int x, int y) const noexcept {
        return x >= 0 && x < width_ && y >= 0 && y < height_;
    }

    [[nodiscard]] bool isWalkable(int x, int y) const noexcept {
        return inBounds(x, y) &&
               grid_[static_cast<size_t>(y)][static_cast<size_t>(x)].isWalkable();
    }

    // -----------------------------------------------------------------------
    // 엔티티 좌표 관리
    // -----------------------------------------------------------------------

    // --- 플레이어 ---
    void setPlayerPos(int x, int y) {
        if (!inBounds(x, y))
            throw std::out_of_range("Player position out of map bounds.");
        playerPos_ = Point{ x, y };
    }

    [[nodiscard]] bool hasPlayer() const noexcept { return playerPos_.has_value(); }

    [[nodiscard]] Point getPlayerPos() const {
        if (!playerPos_.has_value())
            throw std::runtime_error("Player position not set.");
        return *playerPos_;
    }

    void clearPlayer() noexcept { playerPos_.reset(); }

    // --- 몬스터 ---
    // 등록: 같은 좌표가 이미 있으면 덮어쓰지 않고 추가 (몬스터 ID로 구분 필요 시 확장)
    void addMonsterPos(int x, int y) {
        if (!inBounds(x, y))
            throw std::out_of_range("Monster position out of map bounds.");
        monsterPositions_.push_back(Point{ x, y });
    }

    // 특정 인덱스의 몬스터 위치 갱신 (이동 시)
    void setMonsterPos(size_t idx, int x, int y) {
        if (idx >= monsterPositions_.size())
            throw std::out_of_range("Monster index out of range.");
        if (!inBounds(x, y))
            throw std::out_of_range("Monster position out of map bounds.");
        monsterPositions_[idx] = Point{ x, y };
    }

    // 특정 인덱스 몬스터 제거 (사망 처리)
    // 주의: 제거 후 뒤쪽 인덱스가 앞으로 당겨지므로,
    //        이전에 monsterIndexAt()으로 구한 인덱스는 즉시 무효화됨.
    void removeMonsterAt(size_t idx) {
        if (idx >= monsterPositions_.size())
            throw std::out_of_range("Monster index out of range.");
        monsterPositions_.erase(monsterPositions_.begin() + static_cast<std::ptrdiff_t>(idx));
    }

    [[nodiscard]] const std::vector<Point>& getMonsterPositions() const noexcept {
        return monsterPositions_;
    }

    [[nodiscard]] size_t monsterCount() const noexcept { return monsterPositions_.size(); }

    // 특정 좌표에 몬스터가 있는지 확인 — 충돌 판정용
    [[nodiscard]] bool hasMonsterAt(int x, int y) const noexcept {
        return std::any_of(monsterPositions_.begin(), monsterPositions_.end(),
            [x, y](const Point& p){ return p.x == x && p.y == y; });
    }

    // 특정 좌표의 몬스터 인덱스 반환 (없으면 nullopt)
    [[nodiscard]] std::optional<size_t> monsterIndexAt(int x, int y) const noexcept {
        for (size_t i = 0; i < monsterPositions_.size(); ++i) {
            if (monsterPositions_[i].x == x && monsterPositions_[i].y == y)
                return i;
        }
        return std::nullopt;
    }

    void clearMonsters() noexcept { monsterPositions_.clear(); }

    // -----------------------------------------------------------------------
    // 비용 있는 연산 (.cpp 구현)

    /// 걷기 가능한 타일 중 무작위 좌표 반환.
    /// walkable 타일이 없으면 std::runtime_error 발생.
    [[nodiscard]] std::pair<int, int> getRandomFloor(std::mt19937& rng) const;

private:
    // MapGeneratorBase 계층에만 쓰기 권한 부여
    friend class MapGeneratorBase;

    [[nodiscard]] Tile& atMut(int x, int y) {
        return grid_[static_cast<size_t>(y)][static_cast<size_t>(x)];
    }

    int                            width_;
    int                            height_;
    std::vector<std::vector<Tile>> grid_;

    std::optional<Point>           playerPos_;        // 플레이어 좌표 (없으면 미배치)
    std::vector<Point>             monsterPositions_; // 몬스터 좌표 목록

    inline static const Tile voidTile_ { TileType::Void, false };
};

} // namespace Roguelike

#endif // ROGUELIKE_MAP_H
