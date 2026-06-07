// Map.cpp
#include "Map.h"

#include <stdexcept>
#include <random>
#include <vector>
#include <utility>

namespace Roguelike {

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
    {
    }

    std::pair<int, int> Map::getRandomFloor(std::mt19937& rng) const {
        std::vector<std::pair<int, int>> floors;
        floors.reserve(static_cast<size_t>(width_ * height_) / 4);

        for (int y = 0; y < height_; ++y)
            for (int x = 0; x < width_; ++x)
                if (grid_[static_cast<size_t>(y)][static_cast<size_t>(x)].isWalkable())
                    floors.emplace_back(x, y);

        if (floors.empty())
            throw std::runtime_error("No walkable tiles found on the map.");

        std::uniform_int_distribution<size_t> dist(0, floors.size() - 1);
        return floors[dist(rng)];
    }

} // namespace Roguelike