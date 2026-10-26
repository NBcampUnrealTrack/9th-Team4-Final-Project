# HUD 연동 요청 — 스킬 표시 데이터

작성일: 2026-10-25  
대상: 스킬 담당자 / 빌드 아이템 데이터 담당자

## 목적

HUD의 장착 스킬 슬롯에 아이콘을 표시하기 위한 데이터 제공 요청입니다. 액티브 2개와 패시브 1개가 대상입니다.

## 현재 확인한 구조

- `UNF_SkillSlotViewModel`은 `UNF_BuildComponent::GetCurrentBuild()`와 `FindItemDefinition()`으로 장착 스킬을 조회합니다.
- `UNF_BuildItemDefinitionDataAsset`에는 `ItemId`, `DisplayName`, `AbilityClass`가 있지만 아이콘 필드는 없습니다.
- VM에는 `SkillIcon`과 `SetSkillIcon()`이 있지만, 원본 데이터에서 아이콘을 가져오는 연결은 없습니다.
- 쿨타임은 기존 VM이 ASC의 Ability에서 남은 시간과 전체 시간을 조회하고 있습니다. HUD용 쿨타임 복제 변수를 별도로 추가할 필요는 없습니다.

위 내용은 작성일의 C++ 소스 기준입니다. 별도 Blueprint 전용 데이터가 있다면 그 조회 경로를 알려주세요.

## 요청 사항

1. 담당자가 관리하는 스킬 또는 빌드 아이템 DataAsset에 스킬 아이콘 참조를 제공해주세요.
2. 별도 스킬 DataAsset을 사용하는 경우, 현재 장착 `ItemId`에서 해당 DataAsset을 찾는 참조 또는 공개 조회 경로를 제공해주세요.
3. 표시 이름과 아이콘의 원본 위치를 알려주세요. 기존 `DisplayName`을 계속 사용할 수 있다면 그대로 사용하겠습니다.
4. 액티브·패시브와 티어별 아이템 모두 장착 데이터에서 표시 정보까지 조회할 수 있어야 합니다.

아이콘 필드 이름과 참조 방식은 담당 파트의 규칙에 맞춰 정해주세요. UI에서 별도 `SkillId → Icon` 매핑 DataAsset을 중복 관리하지 않겠습니다.

## 데이터 흐름과 UI 담당 범위

```text
장착 빌드 → 담당자 DataAsset → SkillSlotViewModel → FieldNotify → 스킬 위젯
```

UI에서 DataAsset의 아이콘을 읽어 `SkillIcon`을 갱신하고 위젯에 바인딩합니다. 장착 스킬 변경 시 이전 아이콘을 정리하고 새 아이콘을 반영하는 작업도 UI에서 처리합니다.

모델이나 DataAsset에서 VM 또는 Widget을 직접 참조하거나 호출할 필요는 없습니다.

## 전달받고 싶은 내용

- DataAsset 클래스와 아이콘 필드 또는 Getter 이름
- 장착 `ItemId`에서 해당 데이터로 접근하는 방법
- 값이 지정된 예시 에셋 하나
- 아이콘 미지정 허용 여부

## 완료 확인

- [ ] 액티브·패시브 모두 장착 데이터에서 아이콘을 조회할 수 있습니다.
- [ ] 티어 또는 스킬 교체 후 새 표시 데이터를 조회할 수 있습니다.
- [ ] 아이콘 데이터는 담당자 DataAsset이 원본입니다.
