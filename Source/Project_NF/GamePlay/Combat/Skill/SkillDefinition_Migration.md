# 스킬 정의 호환 전환 (2026-11-02)

이번 변경은 스킬 디렉터리 내 소스만 수정한다. 기존 BP/DA 에셋은 수정하거나 재저장하지 않았다.

## 정식 클래스

- NFGA_SkillBase: 기존 클래스 경로, 직렬화 속성, 외부 호출 API 유지.
- NFGA_ActiveSkillBase: 액티브 부모. 패시브 설정 카테고리를 숨긴다.
- NFGA_PassiveSkillBase: 패시브 부모. 입력/프리뷰/실행/시전 모션 설정 카테고리를 숨긴다.
- NF_SkillDataAsset: 기존 AbilityClass, AbilityLevel, Icon 및 BuildItemSource 계약 유지.
- NF_ActiveSkillDataAsset / NF_PassiveSkillDataAsset: 기존 DA의 자식. 각 유형에 필요한 선택적 설정을 제공한다.

플레이어용 네이티브 스킬만 새 중간 부모를 상속한다. 보스 GA/보스 액터는 변경하지 않았다.
기존 BP가 참조하는 네이티브 클래스 이름 및 기존 속성 소유자는 변경하지 않는다.
enum은 NF_SkillTypes.h로 옮겼지만 enum 이름/값과 /Script/Project_NF 패키지는 유지한다.

## 기존 설정 보존

- 기존 DA의 모든 override 옵션은 기본 false다. 기존 GA의 코스트/프리뷰/GE 등이 그대로 적용된다.
- bOverrideIdentity: 해당 DA의 SkillCategory / BuildCost 사용.
- bOverrideActiveSettings: 액티브 입력 해제, 프리뷰, 실행 액터, 실행 시간, 목표 요구 설정을 한 묶음으로 덮어쓴다. 켜기 전에 모두 이전해야 한다.
- bOverridePassiveSettings: 장착 GE, 이벤트 태그, 자동 활성화, 발동/유지 Cue를 한 묶음으로 덮어쓴다. 켜기 전에 모두 이전해야 한다.
- StateRules는 기존 사망/EMP/InputCancelTags 제한에 **추가**된다. 기존 제한을 해제하는 옵션은 아니다.
- DA를 Spec.SourceObject로 받은 부여 인스턴스에만 설정을 적용한다. CDO는 수정하지 않는다. 실행 중 DA 편집의 실시간 반영은 지원하지 않는다.
- 타입은 기존 GA의 타입에서 판별하며, 액티브 DA/패시브 DA에 잘못된 타입의 GA를 지정하면 거절한다.
- 보스처럼 공통 베이스를 직접 상속하는 기존 GA도 타입이 맞으면 DA를 사용할 수 있다.

## 부여 계약

```cpp
// 서버에서 호출. 무기/장비 부여 경로에 일괄 적용하지 말고 스킬에만 적용.
FGameplayAbilitySpecHandle Handle = SkillData->GrantTo(ASC, SlotIndex);
// 또는 기존 부여 코드에서 SourceObject로 실제 SkillData를 전달.
ASC->GiveAbility(FGameplayAbilitySpec(SkillData->AbilityClass,
    SkillData->AbilityLevel, SlotIndex, SkillData));
```

현재 빌드 컴포넌트는 SourceObject에 빌드 Definition을 넘긴다. 해당 담당자가 실제 스킬 DA를 전달하기 전에는
기존 GA 설정으로 동작한다. DA 코스트는 BuildItemSource 조회부터 적용되지만 런타임 설정은 DA 전달 후 적용된다.
따라서 담당자 연결 완료 전에는 override를 켜거나 기존 GA 설정을 제거하지 않는다.

기존 ASC의 PrepareConfirmedTargetData / ClearConfirmedTargetData / ReleaseSkillInput 및
프리뷰/실행 액터 Initialize 함수는 그대로 사용할 수 있다. 이번 단계에서는 입력 RPC 변경이 필요 없다.

## 남은 전환

1. 빌드 담당: 스킬 SourceObject를 실제 스킬 DA로 전달. 장착/탈착/동일 GA 다중 부여 검증.
2. UI 담당: 종류/코스트/아이콘을 원본 DA/BuildItemSource에서 조회. GA CDO 코스트와 DA 코스트 혼용 금지.
3. 보스 담당: 보스 자체 데이터 전달 용도와 스킬 DA 전달 용도를 협의하고 필요 시 부여 경로 전환.
4. 에셋: 각 BP의 실제 설정을 DA로 옮기고 override 적용, 에디터 재시작 후 BP 컴파일/저장/멀티플레이 테스트.
5. 모두 이전 후 공통 베이스의 구형 필드/API 제거 여부 결정. 이 단계에서는 제거하지 않는다.

공통 베이스 구현은 NFGA_SkillBase.cpp(상태/수명/DA), Active/NFGA_SkillBase_Active.cpp,
Passive/NFGA_SkillBase_Passive.cpp로 나눈다. 직렬화 호환을 위해 함수/속성 소유 클래스는 아직 공통 베이스다.
즉, 최종 얇은 베이스로의 전환을 완료한 것이 아니다.

Temporary 디렉터리는 비교/기존 임시 참조 보존용이다. 정식 코드에서 사용하지 않는다.
임시 BP/DA를 정식 클래스에 자동 변환하는 CoreRedirect는 추가하지 않았다.

## 검증

NF.Skills.Data.CompatibleDefinition: 기존 DA/비-DA 소스 호환, 런타임 DA 적용,
CDO 불변, 상태 차단, 패시브 장착/탈착 테스트를 추가했다.
이번 작업에서 빌드/자동화 테스트/PIE는 실행하지 않았다. 에셋 설정 보존은 에디터에서 최종 확인이 필요하다.
