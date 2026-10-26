# 스킬 아이콘 원본 제공

작성: 이정준 (구현: Codex), 2026-10-26

## 이번 작업 범위

스킬 측 UNF_SkillDataAsset에 아이콘 원본과 공개 Getter를 제공한다.
빌드 공통 구조체/빌드 DA/빌드 조회 코드 및 UI 코드는 수정하지 않는다.
이전에 추가했던 FNF_BuildItemData.Icon 필드는 원상복구했다.
따라서 기존 ResolveItemData 결과에서 아이콘을 바로 읽는 연결은 아직 없으며 빌드 담당이 구현한다.

## 스킬 원본

- 클래스: UNF_SkillDataAsset.
- AbilityClass: 장착할 NFGA_SkillBase 자식 GA.
- AbilityLevel: 부여할 레벨, 기본 1.
- Icon: 원본 Texture2D 참조, EditDefaultsOnly / BlueprintReadOnly.
- GetSkillIcon(): 원본 Icon을 반환하는 BlueprintPure Getter.
- 기존 NF_BuildItemSource 인터페이스는 구현한다. 종류/코스트/GA/레벨만 기존 구조체로 반환한다.
- 종류와 코스트는 GA 기본값의 GetSkillType() / GetBuildCost()에서 읽어 중복 설정하지 않는다.
- 아이콘 None을 허용한다. 대체 이미지/숨김은 UI 담당이 결정한다.
- GA 미지정/추상 클래스/음수 코스트/1 미만 레벨은 빌드 데이터 조회 실패로 처리한다.
- ItemId / DisplayName은 기존 빌드 항목 DA의 소유다.

## 에셋 설정

컴파일 후 Data Asset 생성에서 NF_SkillDataAsset을 선택하고 Ability Class / Icon / Ability Level을 지정한다.
예시 설정: DA_Skill_MagicBullet → BP_GA_MagicBullet, 사용할 Texture2D, 레벨 1.
이번 작업에서 실제 아이콘 이미지/uasset은 만들거나 지정하지 않았다.
액티브/패시브에 동일한 클래스를 사용하며, 티어별 원본이 필요하면 별도 DA로 설정한다.

## 빌드/UI 담당 전달 사항

스킬 DA의 Icon 또는 GetSkillIcon()에서 아이콘을 읽을 수 있다.
현재 BuildRules.ItemSources는 ItemId → 원본 DA를 연결하는 구조다.
이 원본 참조를 UI에 공개할지, 기존 조회 결과에 Icon을 추가할지는 빌드 담당이 정하고 구현한다.
UI 담당은 제공된 조회 경로로 아이콘을 읽고 VM/위젯을 갱신한다.
스킬 DA에서 VM이나 Widget을 참조하거나 호출하지 않는다.

## 검증 상태

NF.Skills.Data.SourceIcon 테스트 소스는 스킬 원본 Getter, 아이콘 None 허용,
기존 인터페이스의 종류/코스트/레벨 및 잘못된 설정의 결과 초기화를 검사한다.
빌드/자동화 테스트 실행/에셋 설정 확인은 수행하지 않았다.
