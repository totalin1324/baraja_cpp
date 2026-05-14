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
// MapGeneratorBase: 추상 인터페이스 + 공통 유틸리티 레이어
// ---------------------------------------------------------------------------
class MapGeneratorBase {
public:
    explicit MapGeneratorBase(std::mt19937& rng);
    virtual ~MapGeneratorBase() = default;

    MapGeneratorBase(const MapGeneratorBase&)            = delete;
    MapGeneratorBase& operator=(const MapGeneratorBase&) = delete;
    MapGeneratorBase(MapGeneratorBase&&)                 = default;
    MapGeneratorBase& operator=(MapGeneratorBase&&)      = default;

    virtual void generate(Map& map) = 0;
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
// [구현체 1] RandomWalkGenerator (Drunkard's Walk)
//
// Note: Config를 클래스 외부에 선언.
//   중첩 struct의 기본 멤버 이니셜라이저는 같은 클래스 선언 내의
//   기본 인수(= Config{})에서 참조 불가 — C++ 표준 제약(CWG 325).
//   네임스페이스 스코프로 올리면 완전히 정의된 타입이 되어 해결됨.
// ---------------------------------------------------------------------------
struct RandomWalkConfig {
    float targetFillRatio { 0.45f };
    int   maxSteps        { 50000 };
};

class RandomWalkGenerator final : public MapGeneratorBase {
public:
    using Config = RandomWalkConfig;

    explicit RandomWalkGenerator(std::mt19937& rng, Config cfg = {});

    void generate(Map& map) override;
    [[nodiscard]] std::string name() const override;

private:
    Config cfg_;
};

// ---------------------------------------------------------------------------
// [구현체 2] BSPGenerator (Binary Space Partitioning)
// ---------------------------------------------------------------------------
struct BSPConfig {
    int minLeafSize { 6 };
    int minRoomSize { 3 };
    int padding     { 1 };
};

class BSPGenerator final : public MapGeneratorBase {
public:
    using Config = BSPConfig;

    explicit BSPGenerator(std::mt19937& rng, Config cfg = {});

    void generate(Map& map) override;
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
// MapGeneratorFactory
// ---------------------------------------------------------------------------
enum class GeneratorType { RandomWalk, BSP };

[[nodiscard]] std::unique_ptr<MapGeneratorBase>
makeGenerator(GeneratorType type, std::mt19937& rng);

} // namespace Roguelike

#endif // ROGUELIKE_MAP_GENERATOR_H
