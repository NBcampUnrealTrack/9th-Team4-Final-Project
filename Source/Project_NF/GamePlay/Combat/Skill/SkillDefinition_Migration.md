# 스킬 베이스 실제 분리 (2026-11-02)

## 완료한 소스 변경

- NFGA_SkillBase: DA 참조, 상태 규칙, 사망 차단, 부여/아바타/제거 수명만 소유한다.
- NFGA_ActiveSkillBase: 입력 대기/확정, 프리뷰, 목표 검증, 실행 액터, 타이머, 시전 몽타주, EMP 호환 설정을 실제로 소유한다.
- NFGA_PassiveSkillBase: 장착 GE, 이벤트 구독, 패시브 발동/유지 Cue 및 정리를 실제로 소유한다.
- SkillType / SkillCategory / SkillActivationMethod / BuildCost scalar는 공통 GA에서 제거했다.
  네이티브 기본값은 공통 DA 하위 객체(NativeSkillDefaults)에 보관하며 개별 GA 생성자에서 초기화한다.
- NF_SkillDataAsset / NF_ActiveSkillDataAsset / NF_PassiveSkillDataAsset이 정식 정의다.
- 개별 액티브/패시브 및 **스킬 폴더 내부 보스 GA**의 부모와 기본값 초기화를 새 구조에 맞췄다.
  NF_BossBase 액터, 플레이어, ASC, 빌드, UI 생산 코드는 변경하지 않았다.
- 스킬 관련 기존 자동화 테스트 4개의 캐스팅/설정 접근을 새 소유 클래스에 맞췄다.

## 공통 베이스에 남은 입력 함수

BeginLocalInput / ConfirmLocalInput / CancelLocalInput / PrepareConfirmedTargetData /
ReleaseSkillInput 등은 기존 ASC 호출이 컴파일되도록 남긴 **가상 호환 프로토콜**이다.
공통 구현은 false/빈 값/no-op이며, 상태나 타이머를 소유하지 않는다. 실제 구현은 액티브에서 재정의한다.
분리한 프리뷰/패시브 UFUNCTION 및 필드 참조는 NF_SkillRedirects에서 새 소유자로 redirect한다.
Config 파일은 변경하지 않는다. redirect가 모든 BP 그래프와 기본값 이전을 보장하지는 않는다.

## DA 선택과 설정

우선순위: Spec.SourceObject의 스킬 DA → GA의 SkillDefinition → 네이티브 기본값.
SkillDefinition을 BP에 지정하면 현재 빌드 부여 방식도 DA 설정을 사용할 수 있다.
빌드 담당자가 실제 스킬 DA를 SourceObject로 전달하면 장착한 정의가 우선한다.

- bOverrideIdentity: DA의 카테고리/코스트/발동 방식 메타데이터 사용.
- bOverrideActiveSettings: 입력 해제/프리뷰/실행 액터/시전·실행 시간/목표 요구/시전 모션 설정을 한 묶음으로 적용.
- bOverridePassiveSettings: 장착 GE/이벤트/자동 활성화/발동·유지 Cue 설정을 한 묶음으로 적용.
- bOverrideStateRules=false: StateRules가 기존 액티브 EMP/InputCancelTags에 추가된다.
- bOverrideStateRules=true: 기존 EMP/InputCancelTags 대신 DA의 StateRules만 사용한다. 사망 차단은 항상 유지한다.
- 패시브 GE 장착 수명은 GA 실행 수명과 별도다. StateRules는 GA 활성화/실행 및 조건 이벤트 제한이다.
  장착 GE 자체를 태그로 중지하려면 GE 적용/유지 조건도 설정해야 한다.

## 중요: 기존 BP 설정 이전이 필요함

이번에는 속성의 실제 소유자를 옮겼다. 이전 단계와 달리 **그대로 빌드해도 모든 BP 값이 자동 보존된다고 보장할 수 없다.**
특히 기존 BP에 편집된 카테고리/코스트/타입/발동 방식은 GA 필드에서 없어졌다.
아래 도구는 제공만 했고 실행하지 않았다. 원본 .uasset은 수정/재저장하지 않았다.

1. 기존 클래스를 아직 로드한 에디터에서, **새 코드 빌드/재시작 전에** Editor/SkillDefinitionMigration.py를 로드해 export_snapshot()을 실행한다.
   읽기 전용으로 모든 스킬 BP 기본값과 기존 DA의 아이콘/레벨 정보를 Saved/SkillMigration/skill_defaults.json에 저장한다.
   새 코드가 이미 로드되었다면 이전 소스와 원본 에셋 백업으로 내보내야 한다. 복구되지 않은 값을 추측하지 않는다.
2. BP/DA 에셋을 별도로 백업한 뒤 에디터를 종료하고 새 코드 빌드, 재시작한다. Live Coding으로 반영하지 않는다.
3. apply_snapshot(snapshot_path)는 기본 dry_run=True로 계획만 출력한다.
4. 검토 후 apply_snapshot(snapshot_path, dry_run=False) 실행 시 스킬별 DA_Migrated_<BP이름>을 생성하고 BP의 SkillDefinition에 연결한다.
   직접 공통 베이스를 부모로 삼은 BP만 타입에 맞춰 재부모화한다. 사용자 정의 BP 부모를 자동으로 버리지 않는다.
5. 기존 빌드용 DA/상점 카탈로그는 덮어쓰지 않는다. 기존 DA가 여러 단계/레벨이면 담당자가 새 DA들과 통합해야 한다.
6. BP 컴파일 오류가 있으면 스크립트가 해당 BP 저장 전에 멈춘다. 옮겨진 멤버 노드/메타데이터 직접 접근을 고친다.
7. 모든 BP 컴파일, 설정 비교 및 멀티플레이 검증 후 저장/병합한다.

Python 도구는 구문 검사만 완료했다. 실제 Unreal Python API/에셋 이전은 실행 검증 전이다.

## 다른 담당자 연결

- 빌드: 스킬 DA를 Spec.SourceObject로 전달(또는 GrantTo). 무기/장비 SourceObject는 유지.
- UI: 원본 DA/BuildItemSource에서 코스트/아이콘 조회. CDO 기본값과 장착한 DA 값을 혼용하지 않는다.
- 보스: 스킬 GA 부모는 소스에서 Active로 옮겼다. 보스 액터의 부여 경로는 기존 상태이며,
  보스 DA를 적용할 때 기존 SourceObject=this 용도와 충돌하는지 확인한다.
- 기존 Tmp 클래스는 비교 및 참조 보존용으로 남겼으며 정식 코드에서 사용하지 않는다.

## 검증 상태

소스 구조/함수 소유자/변경 형식 및 Python 구문 검사만 수행한다.
이 작업에서 빌드, UHT, 자동화 테스트, PIE, BP 에셋 이전은 실행하지 않았다.

## 빌드 전 내보내기 예시 (에디터 Python 콘솔)

```python
exec(open(r"D:\Unreal\9th-Team4-Final-Project\Source\Project_NF\GamePlay\Combat\Skill\Editor\SkillDefinitionMigration.py", encoding="utf-8").read())
snapshot_path = export_snapshot()
```

새 빌드로 재시작한 뒤 같은 스크립트를 로드하고, 반환된 JSON 경로로 계획부터 확인한다.

```python
apply_snapshot(r"D:\Unreal\9th-Team4-Final-Project\Saved\SkillMigration\skill_defaults.json")
# 계획/백업 검토 후에만 실행
apply_snapshot(r"D:\Unreal\9th-Team4-Final-Project\Saved\SkillMigration\skill_defaults.json", dry_run=False)
```
