# baraja_cpp

트럼프 카드 속성을 기반으로 한 로그라이크 게임 (C++17, 개발 중)

---

## 프로젝트 구조

```
baraja_cpp/
├── Suit.h              # 카드 속성 시스템 (헤더 전용)
├── Map.h / Map.cpp     # 맵 타일 컨테이너
├── MapGenerator.h/.cpp # 맵 자동 생성 (RandomWalk, BSP)
├── Monster.h/.cpp      # 몬스터 기반 클래스 및 구현체
├── player.h / player.cpp # 플레이어 캐릭터
├── main.cpp            # 게임 루프 및 렌더링
```

모든 코드는 `Roguelike` 네임스페이스 안에 있습니다.

---

## 빌드

빌드 시스템 파일은 아직 없으며, C++17 컴파일러로 직접 컴파일합니다.

```bash
# g++ (Windows CMD / PowerShell)
g++ -std=c++17 -Wall -Wextra -o baraja Map.cpp MapGenerator.cpp Monster.cpp player.cpp main.cpp

# g++ (Linux / WSL) — conio.h, windows.h 의존성으로 현재 미지원
g++ -std=c++17 -Wall -Wextra -o baraja Map.cpp MapGenerator.cpp Monster.cpp player.cpp main.cpp
```

> Windows 환경에서 개발 및 실행을 권장합니다. (`conio.h`, `windows.h` 사용)

---

## 시스템 설명

### 속성 시스템 (`Suit.h`)

트럼프 카드 4가지 속성(♥ ♦ ♣ ♠)을 기반으로 한 상성 시스템입니다.

```
상성 순환: ♦ > ♥ > ♠ > ♣ > ♦
```

| 관계       | 배율   |
|-----------|-------|
| 유리 (상성)  | 1.5배  |
| 불리 (역상성) | 0.75배 |
| 중립 / 무속성 | 1.0배  |

`getSuitMultiplier(attacker, defender)` 함수가 모든 전투 계산의 단일 기준점입니다.

---

### 맵 (`Map.h` / `Map.cpp`)

타일(`Wall`, `Floor`, `Void`) 데이터를 관리하는 컨테이너입니다.

- 타일 쓰기(`atMut`)는 `friend`를 통해 `MapGeneratorBase` 계층에만 허용됩니다.
- 플레이어와 몬스터의 좌표를 맵에서 중앙 관리합니다.
- `getRandomFloor()`: 걷기 가능한 타일 중 무작위 좌표 반환 (맵 생성 시 배치용).

---

### 맵 생성 (`MapGenerator.h` / `MapGenerator.cpp`)

두 가지 알고리즘을 제공합니다.

| 생성기 | 알고리즘 | 특징 |
|------|---------|------|
| `RandomWalkGenerator` | Drunkard's Walk | 유기적인 동굴형 맵 |
| `BSPGenerator` | Binary Space Partitioning | 반듯한 방과 복도 구조 |

`makeGenerator(GeneratorType, rng)` 팩토리 함수로 생성합니다.

```cpp
std::mt19937 rng(seed);
auto gen = Roguelike::makeGenerator(Roguelike::GeneratorType::BSP, rng);
gen->generate(map);
```

---

### 전투 엔티티 (`player.h/.cpp`, `Monster.h/.cpp`)

`Player`와 `MonsterBase` 모두 동일한 2중 오버로드 피격 패턴을 사용합니다.

```cpp
// raw 데미지: 상성 배율 + 방어력 적용
entity.takeDamage(rawDamage, attackerSuit);

// final 데미지: 이미 계산된 값을 그대로 적용
entity.takeDamage(finalDamage);
```

전투 시 `attackPlayer()` / `attackMonster()`를 직접 호출하면 계산과 적용이 한 번에 처리됩니다.

#### 몬스터 목록

| 클래스 | 이름 | HP | 공격 | 방어 | 경험치 | 특징 |
|-------|------|----|------|------|--------|------|
| `BasicMonster` | Goblin (`g`) | 10 | 4 | 1 | 8 | 기본 근접 공격 |

---

### 레벨 시스템

`Player::gainExp()`로 경험치를 획득하고, 조건 충족 시 자동으로 레벨업합니다.

- 레벨업 시: 최대 HP +5 (전체 회복), 공격 +2, 방어 +1
- 다음 레벨 경험치 임계값: 직전 임계값 × 1.5배
- 한 번에 여러 레벨 상승도 처리됩니다.

---

### 게임 루프 (`main.cpp`)

50×22 크기의 BSP 맵에서 플레이어와 고블린 3마리가 배치된 기본 게임 루프입니다.

| 기호 | 의미 |
|------|------|
| `@` | 플레이어 |
| `g` | 고블린 (Goblin) |
| `.` | 바닥 (Floor) |
| `#` | 벽 (Wall) |

- 이동: WASD / 종료: Q
- 이동 목표 칸에 몬스터가 있으면 자동 공격
- 모든 몬스터 처치 시 승리 / HP 0이면 게임 오버

---

## 개발 현황

- [x] 카드 속성 상성 시스템
- [x] 타일 맵 컨테이너
- [x] 맵 자동 생성 (RandomWalk, BSP)
- [x] 플레이어 / 몬스터 전투 시스템
- [x] 레벨업 시스템
- [x] 게임 루프 / `main.cpp`
- [x] 몬스터 이동 AI
- [x] 배틀 UI (별도 전투 화면)
- [x] 덱빌딩 시스템
- [x] 아이템 및 카드 시스템
- [ ] 스테이지 진행 시스템
