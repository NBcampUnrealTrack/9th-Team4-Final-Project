# Player Pawn 부재 상태 → LifeComponent 판정 연동 계약

## 목적

팀원의 Pawn이 없는 이유를 구분하여 `UNF_PlayerLifeComponent::CheckAliveTeammate()`가 올바른 결과를 반환하도록 한다.

Pawn이 없다는 사실만으로는 다음 두 상황을 구분할 수 없다.

- 최초 Pawn 생성 전: 아직 생존 여부를 판단할 정보가 부족하다.
- 사망 후 리스폰 대기 또는 영구 탈락: 살아 있는 팀원으로 판단하면 안 된다.

Core는 `ANF_GameMode::GetPlayerPawnState()`를 통해 구분된 상태를 제공하고, LifeComponent는 팀원 Pawn이 없을 때 해당 API를 사용한다.

## 담당 범위

### Core 담당

- 플레이어별 `ENF_PlayerPawnState`를 서버에서 관리한다.
- `ANF_GameMode::GetPlayerPawnState()` 조회 API를 유지한다.
- 최초 스폰, 사망, 리스폰 예약, 새 Pawn 생성, 탈락 시점에 상태를 갱신한다.
- 내부 구현이 `UNF_PlayerRespawnComponent`로 분리되더라도 GameMode 조회 API의 동작을 유지한다.

### 지우 담당 — `UNF_PlayerLifeComponent`

- 팀원 Pawn이 없을 때 `ANF_GameMode::GetPlayerPawnState()`를 조회한다.
- 최초 생성 전과 사망 후 리스폰 대기 상태를 서로 다르게 판정한다.
- 기존 Pawn이 존재하는 경우의 LifeState, ASC, Avatar 검사는 유지한다.

## Core 제공 상태

```cpp
UENUM(BlueprintType)
enum class ENF_PlayerPawnState : uint8
{
	Unknown,
	InitialSpawnPending,
	Active,
	RespawnPending,
	Eliminated
};
```

| 상태 | 의미 | LifeComponent 판정 |
|---|---|---|
| `Unknown` | 등록되지 않았거나 상태를 확인할 수 없음 | `Unknown` |
| `InitialSpawnPending` | 최초 Pawn 생성 전 | `Unknown` |
| `Active` | 정상적으로 Pawn을 보유해야 하는 상태 | Pawn이 없다면 `Unknown` |
| `RespawnPending` | 사망 후 리스폰 대기 | `NoAliveTeammate` |
| `Eliminated` | 리스폰 불가 또는 영구 탈락 | `NoAliveTeammate` |

## 수정 대상

`UNF_PlayerLifeComponent::CheckAliveTeammate()`의 팀원 Pawn 부재 처리 부분이다.

### 기존 코드

```cpp
APawn* TeammatePawn = TeammateState->GetPawn();

if (!IsValid(TeammatePawn))
{
	return ENFTeammateCheckResult::Unknown;
}
```

현재 구현은 Pawn이 없는 모든 경우를 `Unknown`으로 처리하므로, 사망 후 리스폰을 기다리는 팀원을 생존 여부 미확정 상태로 잘못 취급한다.

### 변경 코드

```cpp
APawn* TeammatePawn = TeammateState->GetPawn();

if (!IsValid(TeammatePawn))
{
	const ANF_GameMode* NFGameMode =
		World->GetAuthGameMode<ANF_GameMode>();

	if (!IsValid(NFGameMode))
	{
		return ENFTeammateCheckResult::Unknown;
	}

	const ENF_PlayerPawnState PawnState =
		NFGameMode->GetPlayerPawnState(TeammateState);

	switch (PawnState)
	{
	case ENF_PlayerPawnState::InitialSpawnPending:
	case ENF_PlayerPawnState::Unknown:
		// 최초 Pawn 생성 전이거나 상태 정보가 없으므로 판정을 보류한다.
		return ENFTeammateCheckResult::Unknown;

	case ENF_PlayerPawnState::RespawnPending:
	case ENF_PlayerPawnState::Eliminated:
		// 현재 살아 있는 팀원이 아니다.
		return ENFTeammateCheckResult::NoAliveTeammate;

	case ENF_PlayerPawnState::Active:
	default:
		// Active인데 Pawn이 없는 것은 정상적인 상태가 아니므로 안전하게 판정을 보류한다.
		return ENFTeammateCheckResult::Unknown;
	}
}
```

`NF_PlayerLifeComponent.cpp`에는 이미 다음 include가 있으므로 추가 include는 필요하지 않다.

```cpp
#include "Core/NF_GameMode.h"
```

`NF_GameMode.h`가 `NF_PlayerPawnState.h`를 포함하므로 현재 구조에서는 enum을 위한 별도 include도 필요하지 않다.

## 판정 흐름

```text
팀원 PlayerState 확인
    ↓
팀원 Pawn 존재 여부 확인
    ├─ Pawn 존재
    │    └─ 기존 LifeState → ASC → Avatar 판정 유지
    │
    └─ Pawn 없음
         ↓
       GameMode의 PawnState 조회
         ├─ InitialSpawnPending / Unknown → Unknown
         ├─ RespawnPending / Eliminated  → NoAliveTeammate
         └─ Active 또는 예상 밖 상태     → Unknown
```

## 서버 조건

- `CheckAliveTeammate()`는 기존과 같이 서버 권한에서 실행한다.
- `ANF_GameMode`는 서버에만 존재하므로 `GetAuthGameMode<ANF_GameMode>()`를 사용한다.
- 클라이언트 UI에서 이 API를 직접 조회하지 않는다.
- GameMode 또는 PlayerState가 유효하지 않으면 확정 판정을 내리지 않고 `Unknown`을 반환한다.

## 기존 로직 유지 사항

Pawn이 존재하는 경우에는 다음 기존 검사를 그대로 유지한다.

- Pawn이 제거 중인지 검사
- 팀원 `UNF_PlayerLifeComponent` 존재 여부
- 팀원의 팀 배정 확인 여부
- 팀원 LifeState가 `Alive`인지 검사
- 팀원 ASC 존재 여부
- ASC Avatar가 현재 Pawn과 일치하는지 검사
- 실제 Health가 0보다 큰지 검사

이 계약은 Pawn이 없는 분기만 변경한다.

## 완료 확인 항목

- 최초 Pawn 생성 전 팀원의 상태는 `Unknown`으로 판정된다.
- 팀원이 사망하여 `RespawnPending`인 경우 `NoAliveTeammate`로 판정된다.
- 보스 처치 후 `Eliminated`인 경우 `NoAliveTeammate`로 판정된다.
- 팀원의 새 Pawn이 생성되고 상태가 `Active`가 되면 기존 Pawn 기반 검사를 수행한다.
- Core 조회에 실패한 경우 임의로 사망 확정하지 않고 `Unknown`을 반환한다.
- 기존 Pawn 존재 분기의 LifeState, ASC, Avatar, Health 검사가 변경되지 않는다.
- Dedicated Server에서 팀원이 리스폰 대기 중일 때 남은 플레이어의 Health 0 판정이 무기한 보류되지 않는다.

## 지우 담당 요청문

> `UNF_PlayerLifeComponent::CheckAliveTeammate()`에서 팀원 Pawn이 없을 때 무조건 `Unknown`을 반환하지 말고, `ANF_GameMode::GetPlayerPawnState()`를 조회해 주세요. `InitialSpawnPending`과 `Unknown`은 `Unknown`, `RespawnPending`과 `Eliminated`는 `NoAliveTeammate`로 판정하고, Pawn이 존재하는 기존 LifeState·ASC·Avatar·Health 검사는 그대로 유지해 주세요.
