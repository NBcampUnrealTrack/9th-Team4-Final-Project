# NF Model-ViewModel 연동 가이드

## 목적

이 문서는 각 시스템 담당자가 Model 데이터를 ViewModel과 연동하기 위해 Model 측에서 제공해야 하는 사항을 정리한다.

Widget과 View Binding 구성은 이 문서의 범위에 포함하지 않는다.

핵심 원칙은 다음과 같다.

> Model이 ViewModel을 직접 호출하거나 참조하지 않는다. Model은 현재 상태를 조회할 Getter와 상태 변경을 알리는 Delegate, OnRep 또는 GAS 이벤트를 제공하고, ViewModel이 해당 Model을 찾아 구독한다.

## 전체 데이터 흐름

```text
서버에서 권한 있는 상태 변경
    -> GameplayEffect, 서버 함수 또는 Server RPC 처리
    -> 복제 프로퍼티 또는 GAS 데이터 변경
    -> 클라이언트 OnRep 또는 GAS 변경 이벤트 발생
    -> Model 변경 Delegate 발생
    -> ViewModel이 새 상태 조회
    -> ViewModel FieldNotify 갱신
```

ViewModel은 네트워크 복제 대상이 아니다. 서버 권한 Model이 복제한 데이터를 각 로컬 클라이언트의 ViewModel이 읽는 구조로 구현한다.

## 공통 Model 연동 규칙

### 1. 현재 상태를 조회할 Getter 제공

ViewModel은 처음 생성되거나 리스폰 후 Model에 다시 연결될 때 초기 상태를 읽을 수 있어야 한다.

```cpp
int32 GetCurrentLevel() const;
int32 GetCurrentExperience() const;
const FNF_BuildValidation& GetValidation() const;
```

변경 Delegate만 있고 Getter가 없으면 ViewModel이 변경 이벤트 발생 이후에 생성되었을 때 현재 상태를 복원할 수 없다.

복합 상태라면 여러 Getter 대신 하나의 Snapshot 구조체를 반환해도 된다.

```cpp
FNF_ConsumableSlotState GetConsumableSlotState() const;
```

### 2. 상태 변경 Delegate 제공

Model 상태가 변경될 때 ViewModel이 알 수 있어야 한다.

```cpp
DECLARE_MULTICAST_DELEGATE_TwoParams(
    FNF_OnExperienceChanged,
    int32,
    int32);

FNF_OnExperienceChanged OnExperienceChanged;
```

Blueprint 구독이 필요하면 Dynamic Multicast Delegate를 사용하고, C++ 전용이면 일반 Multicast Delegate를 사용한다.

Delegate에는 다음 두 방식 중 하나를 적용한다.

1. 변경된 값을 인자로 전달한다.
2. 변경 사실만 알리고 ViewModel이 Getter로 최신 Snapshot을 다시 읽게 한다.

여러 값이 서로 연관되어 있다면 두 번째 방식이 상태 불일치를 줄이기 쉽다.

### 3. 네트워크 복제와 OnRep 처리

이 프로젝트는 Dedicated Server 방식이므로 서버에서 바뀐 값이 클라이언트에 도착한 시점에도 변경 알림이 발생해야 한다.

```cpp
UPROPERTY(ReplicatedUsing = OnRep_Experience)
int32 Experience = 0;

void ANF_PlayerState::OnRep_Experience()
{
    OnExperienceChanged.Broadcast(Experience, RequiredExperience);
}
```

서버에서 값을 변경하는 함수에서도 필요한 Delegate를 발생시킨다.

```cpp
void ANF_PlayerState::AuthoritySetExperience(int32 NewExperience)
{
    if (!HasAuthority())
    {
        return;
    }

    Experience = NewExperience;
    OnExperienceChanged.Broadcast(Experience, RequiredExperience);
    ForceNetUpdate();
}
```

기본 흐름은 다음과 같다.

```text
서버 값 변경
    -> 서버 Delegate
    -> 프로퍼티 복제
    -> 클라이언트 OnRep
    -> 클라이언트 Delegate
    -> ViewModel 갱신
```

서버와 클라이언트에서 서로 다른 Delegate를 사용하지 말고 동일한 상태 변경 경로를 제공하는 것이 좋다.

### 4. 구독 해제가 가능한 Delegate 사용

ViewModel은 다음 상황에서 기존 Model과의 연결을 해제하고 다시 연결할 수 있다.

- Pawn 리스폰
- PlayerController 또는 PlayerState 교체
- Seamless Travel
- 관찰 중인 몬스터 변경
- ASC 변경
- ViewModel 종료

따라서 Model Delegate는 `AddUObject`, `AddDynamic`, `Remove`, `RemoveDynamic` 등을 통해 정상적으로 등록 및 해제할 수 있어야 한다.

### 5. Model은 UI 계층을 참조하지 않음

Model에서 다음 타입을 직접 참조하지 않는다.

- `UNF_*ViewModel`
- `UUserWidget`
- `UTextBlock`, `UProgressBar` 등의 Widget 클래스
- Widget Visibility
- UI 색상과 애니메이션

Model은 게임 상태만 제공하고, 텍스트 변환과 표시 여부 판단은 ViewModel에서 담당한다.

예를 들어 Model은 `FText("보스 전투")` 대신 `ENF_MatchPhase::Boss`를 제공한다.

## GAS 데이터 연동 원칙

GAS Attribute와 Gameplay Tag는 이미 변경 이벤트를 제공하므로 별도의 UI 전용 Delegate를 만들 필요가 없다.

ViewModel은 다음 API로 직접 구독한다.

```cpp
ASC->GetGameplayAttributeValueChangeDelegate(Attribute);
ASC->RegisterGameplayTagEvent(Tag);
```

GAS 담당자가 보장해야 하는 사항은 다음과 같다.

- AttributeSet이 ASC에 정상 등록되어 있어야 한다.
- Attribute와 Gameplay Tag가 필요한 클라이언트에 복제되어야 한다.
- 가능한 상태 변경은 GameplayEffect를 통해 처리한다.
- Attribute 직접 대입으로 GAS 변경 알림을 우회하지 않는다.
- GameplayEffect 제거 시 Granted 또는 Owning Tag Count가 정상 감소해야 한다.
- 지속시간 UI가 필요한 GameplayEffect는 ASC에서 남은 시간을 조회할 수 있어야 한다.
- Ability의 Cost, Cooldown, 차단 Tag가 `CanActivateAbility()`에 반영되어야 한다.

## 담당자별 Model 계약

## 명현: 플레이어와 소모품

### 플레이어 체력과 실드

현재 `UNF_PlayerStatusViewModel`은 다음 GAS Attribute를 직접 구독한다.

- `Health`
- `MaxHealth`
- `Shield`
- `MaxShield`

따라서 플레이어 시스템에서는 별도의 UI Delegate를 만들 필요가 없다.

다음 데이터 흐름이 정상적으로 유지되면 된다.

```text
GameplayEffect
    -> PlayerState ASC
    -> UNF_PlayerStatusAttribute
    -> GAS Attribute Change Delegate
    -> UNF_PlayerStatusViewModel
```

추가로 보장해야 하는 사항은 다음과 같다.

- 최대 체력 또는 최대 실드가 감소할 때 현재 값 Clamp 처리
- 사망 이후 체력 처리 규칙 통일
- 리스폰 시 Attribute 초기화
- PlayerState의 ASC ActorInfo 초기화
- Owner Client에서 필요한 Attribute 복제

### 레벨과 경험치

현재 `UNF_PlayerStatusViewModel`에는 다음 수동 갱신 함수만 존재한다.

```cpp
void UpdateProgression(
    int32 NewLevel,
    float NewExperience,
    float NewRequiredExperience);
```

실제 Model 연동을 위해 다음 방식 중 하나가 필요하다.

#### 권장 방식: GAS Attribute

레벨과 경험치를 GAS에서 관리한다면 별도의 AttributeSet을 제공한다.

```text
Level
Experience
RequiredExperience
```

ViewModel은 해당 Attribute 변경 Delegate를 구독한다.

#### 일반 복제 방식

GAS로 관리하지 않는다면 PlayerState 또는 전용 Component에서 다음 계약을 제공한다.

```cpp
int32 GetLevel() const;
float GetExperience() const;
float GetRequiredExperience() const;

FNF_OnProgressionChanged OnProgressionChanged;
```

레벨과 경험치는 플레이어 개인 정보이므로 필요에 따라 Owner Only 복제를 사용한다.

### 소모품

소모품 시스템은 최소한 다음 상태를 제공해야 한다.

```cpp
FName GetSelectedItemId() const;
int32 GetSelectedItemQuantity() const;
bool CanUseSelectedItem() const;
FKey GetConsumableInputKey() const;
```

필요한 변경 알림 예시는 다음과 같다.

```text
OnSelectedConsumableChanged
OnConsumableQuantityChanged
OnConsumableUsabilityChanged
OnConsumableCooldownChanged
```

여러 Delegate 대신 전체 상태 Snapshot 하나를 전달해도 된다.

```cpp
DECLARE_MULTICAST_DELEGATE_OneParam(
    FNF_OnConsumableSlotChanged,
    const FNF_ConsumableSlotState&);
```

사용 가능 여부는 최소한 다음 조건을 반영해야 한다.

```text
아이템이 선택됨
AND 수량이 1개 이상
AND 쿨다운 중이 아님
AND State.Dead 없음
AND State.Stunned 없음
AND 소모품 사용 금지 Tag 없음
```

쿨다운과 사용 불가 상태가 GAS에 있다면 소모품 Model은 아이템 및 수량만 제공하고 ViewModel이 ASC Tag를 함께 구독할 수 있다.

## 정준: 스킬

`UNF_SkillSlotViewModel`은 다음 데이터를 함께 사용한다.

- `UNF_BuildComponent`: 현재 장착된 스킬
- `UNF_BuildInputComponent`: 슬롯 단축키
- ASC의 `FGameplayAbilitySpec`: 쿨다운과 활성화 가능 여부

스킬 시스템에서는 다음 사항을 보장해야 한다.

- 장착된 스킬의 AbilitySpec이 ASC에 존재한다.
- 액티브 슬롯 0은 GAS `InputID 1`에 대응한다.
- 액티브 슬롯 1은 GAS `InputID 2`에 대응한다.
- `GetCooldownTimeRemainingAndDuration()`으로 쿨다운을 조회할 수 있다.
- 쿨다운 GameplayEffect가 정상 적용 및 제거된다.
- `CanActivateAbility()`가 Cost, Cooldown 및 차단 Tag를 정확하게 판단한다.
- 스킬 교체 및 제거 시 기존 AbilitySpec을 정리한다.
- 스킬 이름, 아이콘, 설명의 데이터 소유자를 명확히 한다.

추천 스킬 데이터 계약은 다음과 같다.

```cpp
FName GetSkillId() const;
FText GetDisplayName() const;
TSoftObjectPtr<UTexture2D> GetIcon() const;
FText GetDescription() const;
float GetCastTime() const;
```

표시 데이터는 GameplayAbility 여러 곳에 나누기보다 스킬 DataAsset에 모으는 것을 권장한다.

쿨다운 시작과 종료는 GameplayEffect Tag로 표현하면 ViewModel이 Tag 변경 이벤트를 구독할 수 있다.

```text
CoolDown.Skill.Grenade
CoolDown.Skill.Barrier
```

Gameplay Tag는 프로젝트 태그 규칙에 따라 도메인별 h/cpp에서 선언 및 정의하고 작성자, 날짜, 목적을 기록한다.

## 준현: 빌드 시스템

빌드 시스템에는 현재 다음 변경 Delegate가 존재한다.

- `OnDraftChanged`
- `OnBuildChanged`
- `OnBuildShopChanged`
- `OnBuildRequestCompleted`
- `OnSlotKeysChanged`

ViewModel이 전체 상태를 다시 구성할 수 있도록 다음 Getter를 유지해야 한다.

```cpp
const TArray<FNF_BuildSelection>& GetCurrentBuild() const;
const TArray<FNF_BuildSelection>& GetDraftBuild() const;
const UNF_BuildRules* GetRules() const;
int32 GetPlayerLevel() const;
bool IsAwaitingBuildResult() const;
FName GetPendingActiveSkill() const;
ANF_BuildShopZone* GetCurrentShop() const;
const UNF_BuildItemDefinition* FindItemDefinition(FName ItemId) const;
FNF_BuildValidation PreviewBuild(...) const;
```

Delegate 발생 기준은 다음과 같이 통일한다.

- Draft가 변경되면 `OnDraftChanged`
- 실제 적용 빌드가 변경되면 `OnBuildChanged`
- 상점에 입장하거나 퇴장하면 `OnBuildShopChanged`
- 서버 요청이 성공하거나 실패하면 `OnBuildRequestCompleted`
- 슬롯 키가 변경되면 `OnSlotKeysChanged`
- 복제된 빌드가 클라이언트에 도착한 OnRep에서도 해당 Delegate 발생

빌드 적용의 권장 흐름은 다음과 같다.

```text
ViewModel의 ApplyBuild 요청
    -> BuildComponent 공개 함수
    -> Server RPC
    -> 서버 검증
    -> CurrentBuild 복제
    -> 클라이언트 OnRep
    -> OnBuildChanged
    -> ViewModel 갱신
```

`UNF_BuildItemDefinition` 또는 연결된 DataAsset에는 다음 표시 데이터가 있어야 한다.

- `ItemId`
- `DisplayName`
- `Icon`
- `Description`
- `Cost`
- `Tier`
- `ItemType`
- 무기, 방어구 또는 스킬 상세 데이터 참조

현재 일부 Build Item ViewModel은 Icon과 Description을 별도 인자로 받고 있으므로 해당 데이터의 실제 소유자를 확정해야 한다.

## 경환: 일반 몬스터

일반 몬스터 체력은 `UNF_MonsterAttributeSet`을 통해 조회한다.

몬스터 시스템에서 보장해야 하는 사항은 다음과 같다.

- 몬스터가 `IAbilitySystemInterface`를 구현하거나 ASC 접근 함수를 제공한다.
- `Health`, `MaxHealth` AttributeSet이 ASC에 등록되어 있다.
- 필요한 Attribute가 클라이언트에 복제된다.
- 데미지는 GameplayEffect로 적용한다.
- 사망 상태를 Gameplay Tag 또는 명확한 Delegate로 제공한다.
- UI에 사용할 표시 이름 Getter를 제공한다.

권장 최소 계약은 다음과 같다.

```cpp
UAbilitySystemComponent* GetAbilitySystemComponent() const;
FText GetDisplayName() const;
```

타게팅 시스템이나 머리 위 체력바 시스템이 대상 ASC를 찾은 뒤 UI 계층에서 다음 형태로 관찰 대상을 지정한다.

```cpp
MonsterStatusViewModel->SetObservedUnit(
    MonsterASC,
    MonsterDisplayName);
```

몬스터 Model이 `UNF_MonsterStatusViewModel`을 직접 참조하면 안 된다.

## 명우: 보스와 매치메이킹

### 보스

보스도 일반 몬스터와 같은 GAS 계약을 제공한다.

- 보스 ASC 접근 함수
- `UNF_BossAttributeSet::Health`
- `UNF_BossAttributeSet::MaxHealth`
- 표시 이름
- 보스 등장 및 퇴장 알림
- 별도 보스 페이즈가 있다면 페이즈 Getter와 Delegate

예시는 다음과 같다.

```cpp
DECLARE_MULTICAST_DELEGATE_OneParam(
    FNF_OnBossActivated,
    ANF_Boss*);

DECLARE_MULTICAST_DELEGATE(
    FNF_OnBossDeactivated);
```

`UNF_MonsterStatusViewModel`은 현재 일반 몬스터 AttributeSet과 보스 AttributeSet을 판별할 수 있다.

### 매치메이킹

현재 매치메이킹 시스템은 다음 계약을 제공한다.

- `GetMatchState()`
- `GetQueuedPlayerCount()`
- `GetQueueElapsedSeconds()`
- `GetCurrentQueueId()`
- `GetAvailableMatchQueues()`
- `OnMatchStateChanged`
- `OnQueueCountChanged`
- `OnMatchmakingFailed`

담당자는 매치메이킹 상태를 변경할 때 해당 Delegate가 누락 없이 발생하도록 보장해야 한다.

```text
Ended
Queueing
LobbyWaiting
ServerPreparing
ServerConnecting
InProgress
Canceled
```

Model은 상태 Enum과 원본 데이터만 제공한다. 한글 상태 문구, 버튼 활성화 여부 및 Visibility는 ViewModel이 결정한다.

## 지우: GameplayEffect, 사망 및 기절

UI와 연동되는 효과는 Gameplay Tag 계약이 중요하다.

필요한 상태 예시는 다음과 같다.

```text
State.Dead
State.Stunned
State.KeyHolder
State.MaxShieldPenalty
CoolDown.*
```

GameplayEffect 시스템에서 보장해야 하는 사항은 다음과 같다.

- 효과 활성 중 필요한 Granted 또는 Owning Tag가 ASC에 존재한다.
- 효과 제거 시 Tag Count가 정상 감소한다.
- 지속 시간이 필요한 효과는 ASC에서 남은 시간을 조회할 수 있다.
- 사망 및 기절 Tag가 Ability 차단 규칙에 반영된다.
- 최대 실드 감소가 `MaxShield` Attribute 변화에 반영된다.
- 열쇠를 드롭하면 열쇠 보유 Tag가 즉시 제거된다.

ViewModel은 다음 GAS 이벤트를 직접 구독하므로 별도의 UI Delegate가 필요하지 않다.

```cpp
ASC->RegisterGameplayTagEvent(Tag);
ASC->GetGameplayAttributeValueChangeDelegate(Attribute);
```

## Core, 최종 탈출 및 상호작용

### 게임 페이즈

현재 `ANF_GameState`는 다음 구조를 제공한다.

- `FNF_MatchState` 복제
- `OnRep_NFMatchState`
- `OnMatchStateChanged`
- 서버 종료 시간 기반 남은 시간 계산

게임 페이즈는 반드시 서버 권한 함수를 통해 변경한다.

```cpp
AuthoritySetTimedMatchPhase(...);
AuthoritySetUntimedMatchPhase(...);
```

클라이언트가 `FNF_MatchState`를 직접 수정하면 안 된다.

### 상호작용과 탈출

상호작용 Model은 다음 UI 상태를 조회할 수 있어야 한다.

```cpp
AActor* GetCurrentInteractionTarget() const;
bool CanInteract() const;
FText GetUnavailableReason() const;
float GetInteractionProgress() const;
bool IsInteractionInProgress() const;
```

필요한 변경 알림 예시는 다음과 같다.

```text
OnInteractionTargetChanged
OnInteractionAvailabilityChanged
OnInteractionProgressChanged
OnInteractionCompleted
OnInteractionCanceled
```

탈출문은 서버에서 최소한 다음 조건을 다시 검증해야 한다.

- 열쇠 보유 Gameplay Tag
- 같은 팀원 조건
- 필요한 인원 존재 여부
- 상호작용 거리
- 현재 게임 페이즈
- 사망 및 기절 상태
- 상호작용 지속 시간 완료 여부

권장 흐름은 다음과 같다.

```text
로컬 클라이언트 Trace
    -> 프롬프트용 예상 상호작용 상태 생성
    -> ViewModel 갱신

상호작용 입력
    -> Server RPC
    -> 서버에서 거리, 태그, 팀 및 페이즈 재검증
    -> 실제 상호작용 처리
    -> 결과 상태 복제
```

UI 표시를 위해 매 프레임 Server RPC를 호출하지 않는다. 클라이언트의 상호작용 가능 표시는 예상 결과이며 실제 성공 여부는 서버가 결정한다.

## ViewModel 재연결을 위해 필요한 Model 수명주기 알림

`UNF_ViewModelSubsystem`은 로컬 플레이어의 다음 객체를 Context로 제공한다.

- LocalPlayer
- PlayerController
- PlayerState
- Pawn
- AbilitySystemComponent

다음 상황에서는 Context 갱신이 필요하다.

- PlayerController 변경
- 새 Pawn Possess
- Pawn UnPossess
- PlayerState가 클라이언트에 늦게 복제됨
- 리스폰
- Seamless Travel
- ASC 교체 또는 재초기화

특히 Pawn은 존재하지만 PlayerState가 아직 복제되지 않은 시점이 있을 수 있다. `ANF_PlayerController::OnRep_PlayerState()` 등 PlayerState 준비 시점에 `UNF_ViewModelSubsystem::RefreshViewModelContext()`가 호출될 수 있도록 Core와 PlayerController 계층에서 연결해야 한다.

## 피해야 하는 구현

### Model에서 ViewModel 직접 호출

```cpp
// 사용하지 않음
PlayerStatusViewModel->UpdateProgression(...);
```

대신 다음처럼 Model Delegate를 발생시킨다.

```cpp
OnProgressionChanged.Broadcast();
```

ViewModel이 Delegate를 구독하고 Getter로 최신 상태를 읽는다.

### UI 갱신을 위한 Tick 또는 Polling 남용

일반 상태는 Delegate, OnRep, GAS 변경 이벤트를 사용한다.

시간에 따라 계속 변하는 다음 데이터만 제한적으로 타이머를 사용할 수 있다.

- 쿨다운 진행도
- 상호작용 홀드 진행도
- 게임 페이즈 남은 시간
- 매치메이킹 경과 시간

### UI 문구를 Model에 저장

Model에서는 Enum, Gameplay Tag, 상태 코드 및 수치만 제공한다.

```cpp
ENF_MatchPhase GetMatchPhase() const;
ENF_BuildResult GetBuildResult() const;
```

표시 문구 변환과 현지화는 ViewModel 또는 UI 표시 계층에서 담당한다.

### 클라이언트 UI 상태를 서버 판정으로 신뢰

`bCanInteract`, `bCanUse`, `bCanApplyBuild` 등은 UI 표시를 위한 예상값일 수 있다. 실제 게임 결과는 서버에서 모든 조건을 다시 검증해야 한다.

## 팀원별 완료 체크리스트

각 시스템 담당자는 다음 항목을 확인한다.

- [ ] ViewModel이 현재 상태를 한 번에 읽을 Getter 또는 Snapshot이 있다.
- [ ] 상태가 변경되면 Delegate가 발생한다.
- [ ] 서버에서 변경된 값이 필요한 클라이언트에 복제된다.
- [ ] 클라이언트 OnRep에서도 변경 Delegate가 발생한다.
- [ ] ViewModel이 Delegate를 정상적으로 해제할 수 있다.
- [ ] 리스폰과 객체 교체 후 최신 상태를 다시 조회할 수 있다.
- [ ] GAS 데이터는 Attribute 또는 Gameplay Tag로 표현되어 있다.
- [ ] GameplayEffect 제거 시 관련 Tag와 상태가 정상적으로 정리된다.
- [ ] Model이 ViewModel 또는 Widget을 직접 참조하지 않는다.
- [ ] UI 문구, 색상, Visibility가 Model 로직에 포함되지 않는다.
- [ ] 클라이언트 요청을 서버에서 다시 검증한다.
- [ ] 동일한 상태를 서로 다른 클래스가 중복 소유하지 않는다.

## 요약

각 시스템 담당자는 UI를 직접 갱신할 필요가 없다.

Model 측에서는 다음 세 가지를 안정적으로 제공하면 된다.

```text
1. 복제 가능한 권한 상태
2. 현재 상태를 읽는 Getter 또는 Snapshot
3. 상태 변경을 알리는 Delegate, OnRep 또는 GAS 이벤트
```

ViewModel은 이 계약을 구독하여 Model 상태를 UI용 `FieldNotify` 데이터로 변환한다.
