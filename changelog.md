# Changelog

## [2026-05-25] 보스 특수기를 카드 전투에 통합 + 스테이지 시스템 추가

### 변경 내용
- **전투 다형성 도입** (`Monster.h`/`Monster.cpp`)
  - `struct EnemyAction { int damage; std::string log; }` 추가 — 적 한 턴의 결과(데미지·로그) 전달용.
  - `virtual EnemyAction battleTurn(Player&, std::mt19937&)` 추가. 기본 구현은 단순 반격.
  - 반격 데미지 공식을 `computeBattleDamage()` 보호 헬퍼로 집중 (기존 runCardBattle 인라인 공식 유지).
- **보스 구현** (`BossMonster.h`/`BossMonster.cpp`, 신규)
  - JBoss(연속 공격 30% 콤보), QBoss(3턴마다 방어력 -1 디버프), KBoss(HP 절반 시 1회 광폭화), JokerBoss(랜덤 공격/회복/속성변경)의 특수기를 `battleTurn`으로 구현.
  - 보스 `onTurn`은 정지형(빈 구현)으로 유지 → 맵에서 거리 무관 피격 버그 제거.
- **플레이어** (`player.h`/`player.cpp`)
  - `setDefense(int)` 추가 (0 미만 클램프) — QBoss 디버프용 + 기존 컴파일 에러 해결.
- **카드 전투 연동** (`main.cpp`)
  - `runCardBattle`의 적 반격 인라인 계산 2곳을 `monster.battleTurn(player, rng)` 호출로 교체. `damage==0`(회복·속성변경) 턴은 공격 애니메이션 생략.
- **스테이지 시스템** (`main.cpp`)
  - `runStage(stage, player, rng)` 추출 + `bossForStage(stage)`(3/6/9/12 보스) 추가. `main()`은 스테이지 루프 + 결과 처리만 담당.
  - 일반 스테이지 고블린 `3 + stage/2`(상한 8), 보스 스테이지는 보스 1 + 고블린 2.
- **난수원 통일 / 죽은 코드 제거**
  - 보스의 C `rand()`를 게임 전역 `std::mt19937 rng`로 통일 (`uniform_int_distribution` 사용). 시드 미설정 문제 및 rand/mt19937 혼용 해소.
  - `BossMonster.cpp`의 `<cstdlib>`/`<ctime>` include 제거.
  - 사용되지 않던 `JokerBoss::turnCount_` 멤버 제거.

### 이유
- 외부 기여자의 보스 코드는 "몬스터가 onTurn에서 직접 공격"하는 전통 로그라이크를 전제했으나, baraja의 실제 전투는 카드 미니게임(`runCardBattle`)이라 보스 특수기가 전혀 실행되지 않는 죽은 코드였음. 전투 다형성(`battleTurn`)을 도입해 특수기를 실제 발동시킴.
- 보스를 스폰할 진입점이 없어 스테이지 시스템으로 연결.
- 난수원이 보스만 C `rand()`(시드 미설정)였고 `turnCount_`가 미사용이라 일관성·정리를 위해 통일·제거.
