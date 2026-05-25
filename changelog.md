# Changelog

## [2026-05-25] 덱빌딩 사용자 친화성 개선 (팀원 베이스 위 통합)

### 변경 내용
- **덱 큐레이션** (`Card.h`): `removeLibraryCard`(최소 덱 가드), `unequipPermanent`(라이브러리 복귀), `countByEffect` 추가.
- **덱 관리 화면** (`main.cpp`): `viewDeck` → `deckManagerScreen`으로 교체. 문양→랭크 정렬, 효과별 요약, R 제거 / U 고정 해제. 팀원의 카드 상한(N/30) 표시 흡수.
- **전투 예측** (`BattleSystem.h`/`main.cpp`): 비파괴 `previewSelection` + `SelectionPreview` 추가. 선택 조합의 예상 공격/회복/방어/드로우 + 족보를 커밋 전에 표시(`BattleUI::selectionInfo`).
- **도움말** (`main.cpp`): `helpScreen`(상성 순환·카드 효과·족보 배수·조작) 추가, 맵/전투에서 H키, 맵 푸터 안내 갱신.
- 전투 종료 시 `discardAll()` 호출 — 전투 사이 덱 상태 정합성 유지(덱 관리 화면이 정확한 보유 카드 표시).

### 이유
- 카드를 추가만 하던 단방향 덱빌딩에 제거/해제 큐레이션, 정렬·요약, 전투 예측, 도움말을 더해 사용자 친화성 개선.
- 팀원 커밋(카드 상한·드로우 처리·카드소실 수정) 위에 충돌 없이 재통합(중복된 카드소실 수정은 제외).

## [2026-05-25] README 개발현황 최신화

### 변경 내용
- 프로젝트 구조에 `BossMonster.h/.cpp`, `Card.h`, `BattleSystem.h` 추가.
- 빌드 명령에 `BossMonster.cpp` 컴파일 대상 추가.
- 몬스터 목록에 보스 4종(Jack/Queen/King/Joker) 스탯·특징 추가.
- 게임 루프 설명을 단일 맵 → 스테이지 시스템(보스 스폰·카드 전투)으로 갱신.
- 개발 현황 체크리스트에 "보스 몬스터 및 특수기", "스테이지 진행 시스템" 완료 반영.

### 이유
- 보스 전투 통합·스테이지 시스템 추가 후 README가 실제 구현과 어긋나 있어 최신화.

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
