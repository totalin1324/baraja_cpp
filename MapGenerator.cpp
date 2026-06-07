// MapGenerator.cpp
#include "MapGenerator.h"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace Roguelike {

    // ===========================================================================
    // MapGeneratorBase
    // ===========================================================================

    MapGeneratorBase::MapGeneratorBase(std::mt19937& rng)
        : rng_(rng)
    {
    }

    void MapGeneratorBase::setTile(Map& map, int x, int y, TileType type) const {
        if (!map.inBounds(x, y)) return;
        map.atMut(x, y).type = type;
    }

    void MapGeneratorBase::fill(Map& map, TileType type) const {
        for (int y = 0; y < map.height(); ++y)
            for (int x = 0; x < map.width(); ++x)
                setTile(map, x, y, type);
    }

    void MapGeneratorBase::carveRoom(Map& map, int x1, int y1, int x2, int y2) const {
        for (int y = y1; y <= y2; ++y)
            for (int x = x1; x <= x2; ++x)
                setTile(map, x, y, TileType::Floor);
    }

    void MapGeneratorBase::carveHCorridor(Map& map, int x1, int x2, int y) const {
        const int lo = std::min(x1, x2);
        const int hi = std::max(x1, x2);
        for (int x = lo; x <= hi; ++x)
            setTile(map, x, y, TileType::Floor);
    }

    void MapGeneratorBase::carveVCorridor(Map& map, int x, int y1, int y2) const {
        const int lo = std::min(y1, y2);
        const int hi = std::max(y1, y2);
        for (int y = lo; y <= hi; ++y)
            setTile(map, x, y, TileType::Floor);
    }

    int MapGeneratorBase::randInt(int lo, int hi) {
        std::uniform_int_distribution<int> dist(lo, hi);
        return dist(rng_);
    }

    // ===========================================================================
    // RandomWalkGenerator
    // ===========================================================================

    RandomWalkGenerator::RandomWalkGenerator(std::mt19937& rng, Config cfg)
        : MapGeneratorBase(rng), cfg_(cfg)
    {
    }

    void RandomWalkGenerator::generate(Map& map) {
        fill(map, TileType::Wall);

        int cx = map.width() / 2;
        int cy = map.height() / 2;

        const int totalCells = map.width() * map.height();
        const int targetFloors = static_cast<int>(
            static_cast<float>(totalCells) * cfg_.targetFillRatio);
        int floorCount = 0;
        int steps = 0;

        constexpr std::array<Point, 4> dirs{ {
            Point{ 0, -1}, Point{ 0,  1},
            Point{-1,  0}, Point{ 1,  0}
        } };

        while (floorCount < targetFloors && steps < cfg_.maxSteps) {
            if (map.at(cx, cy).type != TileType::Floor) {
                setTile(map, cx, cy, TileType::Floor);
                ++floorCount;
            }

            const auto& dir = dirs[static_cast<size_t>(randInt(0, 3))];
            const int nx = cx + dir.x;
            const int ny = cy + dir.y;

            if (map.inBounds(nx, ny)
                && nx > 0 && ny > 0
                && nx < map.width() - 1
                && ny < map.height() - 1)
            {
                cx = nx;
                cy = ny;
            }
            ++steps;
        }
    }

    std::string RandomWalkGenerator::name() const {
        return "RandomWalk (Drunkard)";
    }

    // ===========================================================================
    // BSPGenerator
    // ===========================================================================

    BSPGenerator::BSPGenerator(std::mt19937& rng, Config cfg)
        : MapGeneratorBase(rng), cfg_(cfg)
    {
    }

    void BSPGenerator::generate(Map& map) {
        fill(map, TileType::Wall);
        rooms_.clear();

        splitAndCarve(map, 1, 1, map.width() - 2, map.height() - 2);
        connectRooms(map);
    }

    std::string BSPGenerator::name() const {
        return "BSP (Binary Space Partitioning)";
    }

    Point BSPGenerator::center(const Room& r) noexcept {
        return { (r.x1 + r.x2) / 2, (r.y1 + r.y2) / 2 };
    }

    void BSPGenerator::splitAndCarve(Map& map, int x1, int y1, int x2, int y2) {
        const int w = x2 - x1;
        const int h = y2 - y1;

        if (w < cfg_.minLeafSize * 2 && h < cfg_.minLeafSize * 2) {
            placeRoom(map, x1, y1, x2, y2);
            return;
        }

        const bool splitH = (h > w) ? true
            : (w > h) ? false
            : (randInt(0, 1) == 0);

        if (splitH && h >= cfg_.minLeafSize * 2) {
            const int split = randInt(y1 + cfg_.minLeafSize, y2 - cfg_.minLeafSize);
            splitAndCarve(map, x1, y1, x2, split);
            splitAndCarve(map, x1, split, x2, y2);
        }
        else if (!splitH && w >= cfg_.minLeafSize * 2) {
            const int split = randInt(x1 + cfg_.minLeafSize, x2 - cfg_.minLeafSize);
            splitAndCarve(map, x1, y1, split, y2);
            splitAndCarve(map, split, y1, x2, y2);
        }
        else {
            placeRoom(map, x1, y1, x2, y2);
        }
    }

    void BSPGenerator::placeRoom(Map& map, int x1, int y1, int x2, int y2) {
        const int p = cfg_.padding;
        const int half = cfg_.minRoomSize / 2;

        const int rx1 = randInt(x1 + p, x1 + p + half);
        const int ry1 = randInt(y1 + p, y1 + p + half);
        const int rx2 = randInt(x2 - p - half, x2 - p);
        const int ry2 = randInt(y2 - p - half, y2 - p);

        if (rx2 <= rx1 || ry2 <= ry1) return;

        carveRoom(map, rx1, ry1, rx2, ry2);
        rooms_.push_back({ rx1, ry1, rx2, ry2 });
    }

    void BSPGenerator::connectRooms(Map& map) {
        for (size_t i = 1; i < rooms_.size(); ++i) {
            const Point p1 = center(rooms_[i - 1]);
            const Point p2 = center(rooms_[i]);

            if (randInt(0, 1) == 0) {
                carveHCorridor(map, p1.x, p2.x, p1.y);
                carveVCorridor(map, p2.x, p1.y, p2.y);
            }
            else {
                carveVCorridor(map, p1.x, p1.y, p2.y);
                carveHCorridor(map, p1.x, p2.x, p2.y);
            }
        }
    }

    // ===========================================================================
    // Factory
    // ===========================================================================

    std::unique_ptr<MapGeneratorBase>
        makeGenerator(GeneratorType type, std::mt19937& rng) {
        switch (type) {
        case GeneratorType::RandomWalk:
            return std::unique_ptr<MapGeneratorBase>(new RandomWalkGenerator(rng));
        case GeneratorType::BSP:
            return std::unique_ptr<MapGeneratorBase>(new BSPGenerator(rng));
        default:
            throw std::invalid_argument("Unknown GeneratorType");
        }
    }

} // namespace Roguelike