# HUD 경험치 필요량 연동 작업 분담

작성일: 2026-10-27  
대상: Player 담당자, UI 담당자

## 목적

HUD 경험치 바에서 다음 비율을 정상적으로 표시하기 위한 최소 연동 작업을 정리한다.

```text
ExperiencePercent = CurrentExp / RequiredExperience
```

현재 `UNF_PlayerStatusViewModel`은 GAS의 `CurrentLevel`, `CurrentExp`를 읽고 있지만
`RequiredExperience`를 얻는 공개 경로가 없어 경험치 바 비율이 항상 `0.0`으로 계산된다.

## 현재 구조

- `UNF_PlayerLevelAttribute`
  - `CurrentLevel`, `CurrentExp`를 GAS Attribute로 관리하고 복제한다.
  - 내부 함수 `GetTargetExpFromLevel()`이 `PlayerLevelDataTable`에서 현재 레벨의 필요 경험치를 계산한다.
  - `GetTargetExpFromLevel()`은 `private`이므로 UI ViewModel에서 직접 사용할 수 없다.
- `UNF_PlayerStatusViewModel`
  - `CurrentLevel`, `CurrentExp`의 GAS Attribute 변경을 구독한다.
  - `RequiredExperience`와 `GetExperiencePercent()`를 이미 가지고 있다.
  - `RequiredExperience`를 실제 데이터로 갱신하는 부분만 연결되지 않았다.

## Player 담당자 작업

현재 레벨의 필요 경험치를 외부에서 읽을 수 있는 읽기 전용 조회 함수를 제공한다.

예시:

```cpp
// NF_PlayerLevelAttribute.h
public:
    float GetRequiredExperienceForCurrentLevel() const;
```

```cpp
// NF_PlayerLevelAttribute.cpp
float UNF_PlayerLevelAttribute::GetRequiredExperienceForCurrentLevel() const
{
    return GetTargetExpFromLevel();
}
```

다음 조건도 Player 영역에서 보장한다.

- 반환값은 현재 레벨 구간에서 다음 레벨까지 필요한 전체 경험치이다.
- 클라이언트에서도 동일한 `PlayerLevelDataTable`을 사용해 조회할 수 있다.
- 데이터 에셋이나 데이터 테이블이 없거나 행을 찾지 못하면 `0.0f`를 반환한다.
- 최대 레벨에서 반환할 값의 규칙을 정한다.
- Model 또는 AttributeSet에서 Widget이나 ViewModel을 직접 참조하지 않는다.

필요 경험치를 별도의 GAS Attribute로 추가하는 방법도 가능하지만, 현재 값이 레벨별 정적
데이터에서 결정되므로 우선은 공개 Getter를 제공하는 방식으로 처리한다.

## UI 담당자 작업

`UNF_PlayerStatusViewModel::RefreshProgressionFromAbilitySystem()`에서
`UNF_PlayerLevelAttribute`를 가져와 공개 Getter를 호출한다.

예시:

```cpp
const UNF_PlayerLevelAttribute* LevelAttributeSet =
    ASC->GetSet<UNF_PlayerLevelAttribute>();

if (IsValid(LevelAttributeSet))
{
    NewRequiredExperience =
        LevelAttributeSet->GetRequiredExperienceForCurrentLevel();
}
```

조회한 값은 다음처럼 ViewModel에 반영한다.

```cpp
if (UE_MVVM_SET_PROPERTY_VALUE(RequiredExperience, NewRequiredExperience))
{
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetExperiencePercent);
}
```

UI 영역에서 추가로 처리할 사항은 다음과 같다.

- `CurrentLevel`이 변경되면 새 레벨의 필요 경험치를 다시 조회한다.
- `CurrentExp` 또는 `RequiredExperience`가 변경되면 `GetExperiencePercent` FieldNotify를 발생시킨다.
- ASC나 AttributeSet이 아직 준비되지 않았다면 `RequiredExperience`를 `0.0f`로 초기화한다.
- WBP에서는 `Level`, `Experience`, `RequiredExperience`, `GetExperiencePercent`를 표시 요소에 바인딩한다.
- `RequiredExperience <= 0.0f`인 경우 경험치 바 비율은 `0.0f`로 표시한다.

## 담당 경계

```text
Player 담당자
    레벨별 필요 경험치 원본과 조회 API 제공
    데이터 테이블 유효성 및 최대 레벨 규칙 보장

UI 담당자
    조회 API 호출
    ViewModel 값 및 FieldNotify 갱신
    WBP 텍스트와 ProgressBar 바인딩
```

Player 시스템은 UI 함수를 호출하지 않고, UI는 레벨업이나 경험치 판정을 수행하지 않는다.
서버는 경험치와 레벨을 GAS로 확정하고 복제하며, 각 클라이언트의 ViewModel은 복제된 레벨을
기준으로 필요 경험치를 조회한다.

## 완료 조건

- [ ] Player 담당자가 현재 레벨의 필요 경험치 Getter를 제공한다.
- [ ] Getter가 클라이언트에서도 올바른 값을 반환한다.
- [ ] UI ViewModel이 HUD 생성 직후 현재 값을 복원한다.
- [ ] 경험치 획득 시 경험치 숫자와 바가 갱신된다.
- [ ] 레벨업 시 다음 레벨의 필요 경험치로 다시 계산된다.
- [ ] 최대 레벨에서 합의한 표시 규칙이 적용된다.

