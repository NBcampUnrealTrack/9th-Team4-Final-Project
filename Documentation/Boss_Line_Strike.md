# Boss Line Strike

`UNFGA_BossLineStrike`는 장판(`BossGroundDamageZone`) 다음, 유도탄(`BossHomingVolley`) 이전에 실행된다.

- 기본 반경 3000cm 안의 생존 플레이어를 중복 없이 무작위 최대 3명 선택한다.
- 보스와 대상의 지면 위치를 시전 시 저장한다. 이후 이동해도 경고 영역은 움직이지 않는다.
- 보스에서 각 저장 위치까지의 선분을 중심으로 폭 200cm의 직사각형 데칼을 2초 동안 표시한다.
- 2초 뒤 같은 직사각형 박스와 겹치는 모든 생존 플레이어에게 `UNFGE_CommonDamage`를 적용한다. 세 영역이 겹쳐도 한 시전당 한 번만 피해를 받는다.
- 지면 중심 위아래 200cm가 기본 판정 높이이다. 플레이어 캡슐이 영역과 겹치는지 판정한다.
- 피해량은 보스 전투 데이터 테이블의 `LineStrikeDamage`(기본 40), 나머지는 GA/BP 기본값에서 조절한다.
- 취소·보스 사망·StateTree 이탈 시 타이머와 데칼을 정리한다.

## StateTree 연결

새 트리 생성 코드는 `GroundDamageZones → LineStrike → HomingVolley` 상태를 생성한다.
이미 저장된 트리는 `FNF_BossAbilityTask.bAppendLineStrikeAfterGroundZone`의 기본값 true를 통해 장판 완료 후 직선 공격까지 기다린 다음 다음 상태로 넘어간다. 기존 트리 uasset을 이번 변경에서 재생성하지 않았다.

별도 LineStrike 상태를 수동 추가한다면 **장판 태스크의 `Append Line Strike After Ground Zone`을 false로 설정**하여 중복 실행을 막는다. 생성 도구는 이를 자동으로 false로 설정한다.

보스의 패턴 목록에 직선 공격이 없으면 BeginPlay에서 장판 다음에 네이티브 GA를 추가한다. BP 파생 GA가 이미 있으면 그 클래스를 유지한다. 별도 상태를 수동 작성하는 경우 태스크 AbilityClass와 보스 패턴 목록의 GA 클래스를 일치시켜야 한다.

## 데칼

기본 머티리얼: `/Game/ProjectNF/Dev/GamePlay/Boss/M_BossLineTelegraph`.
원형 마스크 없이 전체 사각형을 채우는 Substrate Deferred Decal이다. `WarningColor`, `WarningOpacity`로 색과 투명도를 조절할 수 있다. GA의 `WarningMaterial`을 다른 직사각형 데칼로 교체할 수 있다.

서버에서 생성한 `ANF_BossLineTelegraph`가 위치, 회전, 크기, 머티리얼을 클라이언트에 복제한다. 머티리얼이 없거나 경고 생성이 실패한 영역은 공격하지 않는다.

## 검증 상태

머티리얼 생성 커맨드렛 실행 및 에셋 저장 성공. C++ 빌드 실행 승인이 거절되어 컴파일·PIE 검증은 수행하지 못했다.
에디터 재빌드 후 기존 트리/새 트리의 순서, 회피, 겹친 영역의 단일 피해, 멀티플레이 데칼 표시, 시전 중 보스 처치 시 취소를 확인해야 한다.
