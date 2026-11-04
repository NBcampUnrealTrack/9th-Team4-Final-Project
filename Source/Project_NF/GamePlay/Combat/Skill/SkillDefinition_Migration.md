# 최종 스킬 정의 전환 (2026-11-02)

## 설정의 유일한 소유자

- NF_SkillDataAsset: 타입, 카테고리, 코스트, 발동 메타데이터, 클래스/레벨, 아이콘, 상태 규칙.
- NF_ActiveSkillDataAsset: 시전 모션, 실행/시전 시간, 입력 해제 방식, 목표 요구, 프리뷰, 실행 액터.
- NF_PassiveSkillDataAsset: 장착 GE, 이벤트 태그, 자동 활성화, 발동/유지 Cue.
- GA는 DA의 설정을 **직접 조회**한다. ConfigureInstance 및 DA→GA 필드 복사 코드는 제거했다.
- GA의 공통 편집 설정 필드는 제거했다. GA에 표시할 원본 참조는 SkillDefinition 하나다.
- 공통 베이스에는 외부 ASC와 BP를 위한 가상 입력/목표 API만 남았다. 입력 상태/구현은 액티브에만 있다.

개별 스킬의 공격 범위, 이동 속도 등 고유 설정은 이번 공통 베이스 설정 전환 범위에 포함하지 않는다.

## GA가 보관하는 것

- 공통: 선택한 정의의 참조, 아바타/ASC 참조, 상태 태그 구독 핸들.
- 액티브: 실행 중 액터/프리뷰 참조, 현재 목표, 타이머, 입력 홀딩, 시전 연출 중복 방지.
- 패시브: 실제 적용한 GE 핸들, 이벤트 구독 핸들, 장착 여부, 현재 유지 Cue.
- 보스의 실행 도중 계산 시간은 RuntimeExecutionDuration으로 분리했다. 공유 DA를 수정하지 않는다.

네이티브 C++ 스킬의 기존 기본값도 GA scalar가 아닌 숨겨진 transient DA 프리셋에 초기화한다.
프리셋은 외부 정의가 없을 때 사용하는 별도의 선택 가능한 원본이며, 외부 DA 값과 필드별로 병합하지 않는다.
서브오브젝트는 NativeActiveSkillDefaults / NativePassiveSkillDefaults라는 새 이름으로 만들고,
이전 공통형 NativeSkillDefaults 직렬화 객체를 상속/재사용하지 않는다.

## 정의 선택

우선순위: Spec.SourceObject의 스킬 DA → GA의 SkillDefinition → 네이티브 typed DA 프리셋.
액티브에는 ActiveSkillDataAsset, 패시브에는 PassiveSkillDataAsset이 필요하다.
원본을 선택한 뒤에는 모든 공통 설정을 그 DA에서 직접 읽는다.
실행 중 DA/상태 규칙 편집은 지원 계약이 아니다. 장착 전 정의를 확정한다.

## 삭제/호환 항목

- bOverrideIdentity / bOverrideStateRules / bOverridePassiveSettings 제거.
- bOverrideActiveSettings는 **보스 외부 코드의 컴파일용 비편집 필드**로만 남았다.
  스킬 코드에서는 읽지 않으며, 설정 적용은 항상 DA 직접 조회다.
  보스 담당자가 구형 guard를 제거한 뒤 이 필드도 삭제할 수 있다.
- EMP와 구형 InputCancelTags 대신 StateRules를 사용한다. 액티브 DA 기본값에는 EMP 차단/강제 취소가 들어 있다.
  EMP 제외는 해당 규칙 삭제, 새 사용만 차단은 bBlockActivation=true / bCancelRunning=false로 설정한다.
- 사망 차단은 공통 베이스에서 유지한다.
- 패시브 장착 GE 수명은 실행 GA와 별개다. 장착 GE 자체를 조건부 중지하려면 GE 유지/적용 조건도 설정해야 한다.

## 중요: 에셋 이전 전에는 병합/플레이 검증 완료 상태가 아님

**기존 공통형 NF_SkillDataAsset 에셋은 HasValidSettings에서 거절한다.**
설정/코스트가 없는 에셋을 무료 스킬로 처리하지 않기 위한 조치다.
기존 BP의 편집값을 자동 보존한다고 보장하지 않는다. 원본 .uasset은 이번 작업에서 수정/저장하지 않았다.

1. 새 코드 빌드/재시작 **전에**, 기존 클래스가 로드된 에디터에서 Editor/SkillDefinitionMigration.py의 export_snapshot()으로 설정을 내보낸다.
   원래 GA scalar 버전과 중간 버전의 메타데이터 getter를 지원한다. 기존 DA의 아이콘/레벨도 내보낸다.
2. 이미 새 코드로 로드하여 이전 필드가 없어졌다면, 기존 소스/에셋 백업에서 내보낸다.
   최종 코드의 native preset만 보이는 경우 도구는 기본적으로 거절한다. 사라진 값을 추측하지 않는다.
3. 에디터 종료 후 일반 빌드, 재시작한다. 구조 변경을 Live Coding으로 적용하지 않는다.
4. apply_snapshot(snapshot_path)는 기본 dry_run=True로 계획만 출력한다.
5. 백업/계획 검토 후 apply_snapshot(snapshot_path, dry_run=False)로 새 DA_Migrated_<BP이름>을 생성하고 BP SkillDefinition에 연결한다.
   기존 DA/상점 카탈로그는 덮어쓰지 않는다. **빌드 담당자가 기존 공통형 DA 참조를 새 typed DA 참조로 교체해야 한다.**
6. 기존 BP에서 제거된 설정 변수를 직접 읽는 노드는 DA 조회 또는 기존 GetCastTime/GetExecutionDuration 등의 함수로 변경한다.
   함수 소유자 redirect는 유지하지만, Self 변수 접근을 다른 DA 객체 접근으로 자동 변환하는 property redirect는 제거했다.
7. BP 컴파일 오류가 있으면 해당 BP 저장 전에 멈춘다. 실제 에디터에서 설정 비교, 컴파일, 저장, 멀티플레이 검증이 필요하다.

```python
# 기존 에디터 Python 콘솔: 빌드 전 내보내기
exec(open(r"D:\Unreal\9th-Team4-Final-Project\Source\Project_NF\GamePlay\Combat\Skill\Editor\SkillDefinitionMigration.py", encoding="utf-8").read())
snapshot_path = export_snapshot()
# 새 빌드로 재시작한 뒤 스크립트를 다시 로드하고 계획 확인
apply_snapshot(r"D:\Unreal\9th-Team4-Final-Project\Saved\SkillMigration\skill_defaults.json")
# 백업/계획 검토 후에만 저장 실행
apply_snapshot(r"D:\Unreal\9th-Team4-Final-Project\Saved\SkillMigration\skill_defaults.json", dry_run=False)
```

Python 도구는 구문 검사만 수행했다. 실제 Unreal API/에셋 이전 실행은 미검증이다.

## 다른 담당자 요청

- 빌드: 기존 공통형 DA 참조를 typed DA로 교체하고 스킬 부여 시 그 DA를 SourceObject로 전달. 무기/장비는 유지.
- UI: 코스트/아이콘은 실제 DA/BuildItemSource에서 조회. GA CDO의 네이티브 프리셋과 장착한 원본 DA를 혼용하지 않는다.
- 보스: 기존 bOverrideActiveSettings guard는 새 의미가 없어 제거 필요. 자동 생성한 빈 DA도 이제 그 자체가 원본이다.
  원래 BP에서 설정했던 시전 모션/입력/실행 설정이 필요하면 실제 패턴 DA에 기입하고 PatternSkillData에 연결한다.
  보스의 자체 타이머/페이즈 계산은 유지하며, 스킬 쪽에서 계산 실행 시간만 런타임 변수로 옮겼다.

이번 변경은 스킬 소스/스킬 테스트/이전 도구만 포함한다. 플레이어/ASC/빌드/UI/보스 액터 생산 코드는 수정하지 않았다.
기존 Temporary 클래스는 비교/참조 보존용이며 정식 경로에서 사용하지 않는다.

## 검증 상태

스킬 베이스의 편집 필드 제거, 직접 DA 조회, 런타임 계산과 정의 불변성 테스트 소스 추가/수정.
정적 참조/선언/변경 형식 및 Python 구문 검사만 수행했다.
빌드, UHT, 자동화 테스트, PIE, BP/DA 에셋 이전은 실행하지 않았다.
