#pragma once
// MapGenerator.h
#ifndef ROGUELIKE_MAP_GENERATOR_H
#define ROGUELIKE_MAP_GENERATOR_H

#include "Map.h"

#include <random>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

namespace Roguelike {

    // ---------------------------------------------------------------------------
    // MapGeneratorBase
    // ---------------------------------------------------------------------------
    class MapGeneratorBase {
    public:
        explicit MapGeneratorBase(std::mt19937& rng);
        virtual ~MapGeneratorBase() = default;

        MapGeneratorBase(const MapGeneratorBase&) = delete;
        MapGeneratorBase& operator=(const MapGeneratorBase&) = delete;
        MapGeneratorBase(MapGeneratorBase&&) = default;
        MapGeneratorBase& operator=(MapGeneratorBase&&) = default;

        virtual void        generate(Map& map) = 0;
        [[nodiscard]] virtual std::string name() const = 0;

    protected:
        void setTile(Map& map, int x, int y, TileType type) const;
        void fill(Map& map, TileType type) const;
        void carveRoom(Map& map, int x1, int y1, int x2, int y2) const;
        void carveHCorridor(Map& map, int x1, int x2, int y) const;
        void carveVCorridor(Map& map, int x, int y1, int y2) const;

        [[nodiscard]] int randInt(int lo, int hi);

        std::mt19937& rng_;
    };

    // ---------------------------------------------------------------------------
    // RandomWalkGenerator
    // ---------------------------------------------------------------------------
    struct RandomWalkConfig {
        float targetFillRatio{ 0.45f };
        int   maxSteps{ 50000 };
    };

    class RandomWalkGenerator final : public MapGeneratorBase {
    public:
        using Config = RandomWalkConfig;

        explicit RandomWalkGenerator(std::mt19937& rng, Config cfg = {});

        void        generate(Map& map) override;
        [[nodiscard]] std::string name() const override;

    private:
        Config cfg_;
    };

    // ---------------------------------------------------------------------------
    // BSPGenerator
    // ---------------------------------------------------------------------------
    struct BSPConfig {
        int minLeafSize{ 6 };
        int minRoomSize{ 3 };
        int padding{ 1 };
    };

    class BSPGenerator final : public MapGeneratorBase {
    public:
        using Config = BSPConfig;

        explicit BSPGenerator(std::mt19937& rng, Config cfg = {});

        void        generate(Map& map) override;
        [[nodiscard]] std::string name() const override;

    private:
        struct Room { int x1, y1, x2, y2; };

        [[nodiscard]] static Point center(const Room& r) noexcept;

        void splitAndCarve(Map& map, int x1, int y1, int x2, int y2);
        void placeRoom(Map& map, int x1, int y1, int x2, int y2);
        void connectRooms(Map& map);

        Config            cfg_;
        std::vector<Room> rooms_;
    };

    // ---------------------------------------------------------------------------
    // Factory
    // ---------------------------------------------------------------------------
    enum class GeneratorType { RandomWalk, BSP };

    [[nodiscard]] std::unique_ptr<MapGeneratorBase>
        makeGenerator(GeneratorType type, std::mt19937& rng);

} // namespace Roguelike

#endif // ROGUELIKE_MAP_GENERATOR_H