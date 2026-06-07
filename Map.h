#pragma once
// Map.h
#ifndef ROGUELIKE_MAP_H
#define ROGUELIKE_MAP_H

#include <vector>
#include <stdexcept>
#include <random>
#include <utility>
#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace Roguelike {

    // ---------------------------------------------------------------------------
    // TileType
    // ---------------------------------------------------------------------------
    enum class TileType : uint8_t {
        Wall = 0,
        Floor = 1,
        Door = 2,
        Void = 255,
    };

    // ---------------------------------------------------------------------------
    // Tile
    // ---------------------------------------------------------------------------
    struct Tile {
        TileType type{ TileType::Wall };
        bool     visited{ false };

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
    // OptionalIndex: std::optional 없이 쓰는 선택형 인덱스 (C++14 호환)
    // ---------------------------------------------------------------------------
    struct OptionalIndex {
        bool   valid;
        size_t value;

        OptionalIndex() noexcept : valid(false), value(0) {}
        explicit OptionalIndex(size_t v) noexcept : valid(true), value(v) {}

        bool   has_value()              const noexcept { return valid; }
        explicit operator bool()        const noexcept { return valid; }
        size_t   operator*()            const noexcept { return value; }
    };

    // ---------------------------------------------------------------------------
    // Point
    // ---------------------------------------------------------------------------
    struct Point {
        int x{ 0 };
        int y{ 0 };

        constexpr bool operator==(const Point& o) const noexcept {
            return x == o.x && y == o.y;
        }
        constexpr Point operator+(const Point& o) const noexcept {
            return { x + o.x, y + o.y };
        }
    };

    // ---------------------------------------------------------------------------
    // Map
    // ---------------------------------------------------------------------------
    class MapGeneratorBase;

    class Map {
    public:
        Map(int width, int height);

        [[nodiscard]] int  width()  const noexcept { return width_; }
        [[nodiscard]] int  height() const noexcept { return height_; }

        [[nodiscard]] const Tile& at(int x, int y) const noexcept {
            if (!inBounds(x, y)) return voidTile();
            return grid_[static_cast<size_t>(y)][static_cast<size_t>(x)];
        }

        [[nodiscard]] bool inBounds(int x, int y) const noexcept {
            return x >= 0 && x < width_ && y >= 0 && y < height_;
        }

        [[nodiscard]] bool isWalkable(int x, int y) const noexcept {
            return inBounds(x, y) &&
                grid_[static_cast<size_t>(y)][static_cast<size_t>(x)].isWalkable();
        }

        // --- 플레이어 ---
        void setPlayerPos(int x, int y) {
            if (!inBounds(x, y))
                throw std::out_of_range("Player position out of map bounds.");
            playerPos_ = Point{ x, y };
            hasPlayer_ = true;
        }
        [[nodiscard]] bool  hasPlayer()    const noexcept { return hasPlayer_; }
        [[nodiscard]] Point getPlayerPos() const {
            if (!hasPlayer_) throw std::runtime_error("Player position not set.");
            return playerPos_;
        }
        void clearPlayer() noexcept { hasPlayer_ = false; }

        // --- 몬스터 ---
        void addMonsterPos(int x, int y) {
            if (!inBounds(x, y))
                throw std::out_of_range("Monster position out of map bounds.");
            monsterPositions_.push_back(Point{ x, y });
        }
        void setMonsterPos(size_t idx, int x, int y) {
            if (idx >= monsterPositions_.size())
                throw std::out_of_range("Monster index out of range.");
            if (!inBounds(x, y))
                throw std::out_of_range("Monster position out of map bounds.");
            monsterPositions_[idx] = Point{ x, y };
        }
        void removeMonsterAt(size_t idx) {
            if (idx >= monsterPositions_.size())
                throw std::out_of_range("Monster index out of range.");
            monsterPositions_.erase(monsterPositions_.begin() + static_cast<std::ptrdiff_t>(idx));
        }
        [[nodiscard]] const std::vector<Point>& getMonsterPositions() const noexcept {
            return monsterPositions_;
        }
        [[nodiscard]] size_t monsterCount() const noexcept { return monsterPositions_.size(); }

        [[nodiscard]] bool hasMonsterAt(int x, int y) const noexcept {
            return std::any_of(monsterPositions_.begin(), monsterPositions_.end(),
                [x, y](const Point& p) { return p.x == x && p.y == y; });
        }
        [[nodiscard]] OptionalIndex monsterIndexAt(int x, int y) const noexcept {
            for (size_t i = 0; i < monsterPositions_.size(); ++i)
                if (monsterPositions_[i].x == x && monsterPositions_[i].y == y)
                    return OptionalIndex(i);
            return OptionalIndex();
        }
        void clearMonsters() noexcept { monsterPositions_.clear(); }

        [[nodiscard]] std::pair<int, int> getRandomFloor(std::mt19937& rng) const;

    private:
        friend class MapGeneratorBase;

        [[nodiscard]] Tile& atMut(int x, int y) {
            return grid_[static_cast<size_t>(y)][static_cast<size_t>(x)];
        }

        int                            width_;
        int                            height_;
        std::vector<std::vector<Tile>> grid_;

        bool               hasPlayer_ = false;
        Point              playerPos_{ 0, 0 };
        std::vector<Point> monsterPositions_;

        static const Tile& voidTile() noexcept {
            static const Tile t{ TileType::Void, false };
            return t;
        }
    };

} // namespace Roguelike

#endif // ROGUELIKE_MAP_H