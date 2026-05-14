#include "Map.h"

#include <stdexcept>
#include <random>
#include <vector>
#include <utility>

namespace Roguelike {

// ---------------------------------------------------------------------------
// Map 생성자
// ---------------------------------------------------------------------------
namespace {
    int requirePositive(int v) {
        if (v <= 0) throw std::invalid_argument("Map dimensions must be positive.");
        return v;
    }
}

Map::Map(int width, int height)
    : width_(requirePositive(width))
    , height_(requirePositive(height))
    , grid_(static_cast<size_t>(height_),
            std::vector<Tile>(static_cast<size_t>(width_)))
{}

// ---------------------------------------------------------------------------
// Map::getRandomFloor
//
// 복잡도: O(W*H) — 전체 타일 순회 불가피
// 개선 여지: Map이 Floor 좌표 목록을 캐싱하면 O(1)로 단축 가능.
//            단, 타일 변경 시마다 캐시 무효화 로직이 필요하므로
//            현재 생성-후-읽기 패턴에서는 이 구현으로 충분.
// ---------------------------------------------------------------------------
std::pair<int, int> Map::getRandomFloor(std::mt19937& rng) const {
    std::vector<std::pair<int, int>> floors;
    floors.reserve(static_cast<size_t>(width_ * height_) / 4); // 휴리스틱 예약

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            if (grid_[static_cast<size_t>(y)][static_cast<size_t>(x)].isWalkable()) {
                floors.emplace_back(x, y);
            }
        }
    }

    if (floors.empty()) {
        throw std::runtime_error("No walkable tiles found on the map.");
    }

    std::uniform_int_distribution<size_t> dist(0, floors.size() - 1);
    return floors[dist(rng)];
}

} // namespace Roguelike
