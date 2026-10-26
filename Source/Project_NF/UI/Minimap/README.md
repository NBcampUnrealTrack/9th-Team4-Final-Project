# 미니맵 Widget: Snapshot 기반 표시

작성: 이정준 / Codex · 2026-10-26 · UE 5.7

`NF_MinimapWidget`은 ViewModel이 공급하는 전체 Snapshot을 받아 좌표 변환과 그리기만 담당한다.
게임 정보 수집, 팀 판정, 공개 마커 중복 제거, 효과 만료는 Model/VM의 역할이다.
공개/복제 경로 자체는 변경하지 않았다. [공통 공개 목록 가이드](REVEAL.md)

## UI 담당 연결

1. `NF_MinimapWidget`을 부모로 한 WBP를 HUD에 배치한다.
2. WBP의 MVVM 바인딩에서 `NF_MinimapViewModel.Snapshot`을 대상 위젯의 `SetSnapshot`으로 단방향 연결한다.
3. HUD는 기존처럼 VM 생성·연결·해제를 담당한다. 위젯은 VM을 생성하거나 검색하지 않는다.
4. VM 연결 해제/유효 Pawn 소멸 시 무효 Snapshot을 전달해서 이전 표시를 정리한다.

기존 HUD C++에는 Minimap VM 생성·할당 코드가 있다. 이번 작업에서 WBP 에셋의 바인딩을 수정하거나
확인하지는 않았으므로 UI 담당이 실제 WBP에서 연결해야 한다. 바인딩 없이 자동으로 Pawn/팀원을 수집하지 않는다.

## Snapshot 적용 규칙

`UI/Data/NF_MinimapSnapshot.h`의 기존 구조체를 사용하며 중복 선언하지 않는다.

- `SetSnapshot(const FNF_MinimapSnapshot&)`은 BlueprintCallable이다.
- 매번 전체 목록으로 교체한다. 빈 목록은 이전 마커를 제거한다.
- `bHasView == false`, 비정상 ViewLocation/ViewYaw이면 중심·방향·마커를 초기화한다.
  유효한 다음 Snapshot을 받으면 다시 표시한다.
- 무효 View에서는 단색 BackgroundColor만 그리고 지도 텍스처·마커·본인 화살표는 숨긴다.
- ID 없음, 비정상 좌표/크기, 0 이하 크기의 마커는 제외한다.
- 마커의 만료 시각은 0으로 저장한다. VM이 다음 목록에서 제외할 때 제거하며 위젯이 별도로 만료시키지 않는다.
- 팀 판정, ID 중복 제거, 천리안 권한 판단은 수행하지 않는다. VM이 허용된 전체 목록을 공급해야 한다.

## 표시 설정

북쪽은 월드 +X, 오른쪽은 +Y다. 지도 UV는 U=+Y, V=-X다.
`MapWorldMin/Max`는 텍스처 전체 월드 범위, `VisibleWorldSize`는 화면 한 변의 월드 길이(cm)다.
기본 4000cm이면 중심 기준 각 방향 2000cm가 보인다. 지도는 북쪽 고정이고 본인 화살표만 회전한다.
직사각형 슬롯에서는 중앙 정사각형에 표시하고, Z/층 구분은 하지 않는다.

- 마커 크기는 VM의 `Marker.Size`를 UMG 로컬 단위로 사용한다. 지도 한 변보다 크면 그 길이로 제한한다.
  별도 팀원/공개 마커 크기 설정으로 덮어쓰지 않는다. 현재 VM 기본 마커 크기는 20이다.
- 마커 색상은 Widget의 종류별 색상과 WidgetStyle 틴트를 사용한다. Icon이 있으면 해당 텍스처에도 틴트가 적용된다.
- 본인 화살표는 `bShowLocalPlayer`, `LocalPlayerColor`, `LocalPlayerSize`를 유지한다.
- 지도 배율·텍스처·색·클리핑과 `SetVisibleWorldSize`, `ProjectWorldToMinimap`은 유지한다.

## 기존 API/BP 호환

`UpsertMarker`, `RemoveMarker`, `ClearMarkers`, `ClearMarkersByType`, `SetViewLocation`은 삭제하지 않았다.
첫 `SetSnapshot` 전에는 수동 표시 모드로 사용할 수 있다. 이후 해당 위젯 인스턴스는 Snapshot 전용이 되어
수동 마커/중심 변경을 무시하며 Upsert/Remove는 false를 반환한다. 수동 모드로 되돌리는 API는 없다.
외부 마커도 VM의 전체 목록에 합쳐야 하며, 숨기거나 정리할 때도 다음 Snapshot으로 전달해야 한다.
수동 모드에서만 기존 Lifetime(수신 후 로컬 초, 0=무한) 처리를 유지한다.

`Follow Owning Pawn`, `Show Teammates`, `Show Registered Reveals`, `TeammateMarkerSize`, `RevealMarkerSize`는
직렬화/BP 호환용 Deprecated 속성으로 남겼지만 데이터 수집이나 Snapshot 표시 판단에는 사용하지 않는다.
기존 Temporary 전체 표시 속성도 무시한다.
`ArePlayerStatesTeammates`는 기존 테스트/호출자 호환용 정적 유틸리티로 남겼으며 위젯 표시 경로에서 호출하지 않는다.

C++ 전체 검색에서 위젯 구현 외의 UpsertMarker/RemoveMarker/ClearMarkers 호출처는 발견하지 못했다.
기존 팀 판정 테스트가 정적 유틸리티를 사용하므로 유지했다.
BP 에셋 내부 호출처는 확인하지 못했으므로 UI 담당이 Find References로 확인 후 VM 공급으로 전환해야 한다.

## 검증 상태

- 정적 확인: Widget Tick/Paint의 Pawn·Controller·팀·Registry 수집 경로 제거.
- `NF.UI.Minimap.Snapshot` 테스트 소스 추가: 전체 교체, 빈 목록, 무효 View/마커, 재수신,
  VM 크기 보존, 기존 API의 Snapshot 덮어쓰기 방지.
- 기존 좌표 변환/팀 판정 테스트 소스는 유지했다.
- 빌드·자동화 테스트 실행·WBP 컴파일·PIE 표시 확인은 실행하지 않았다.
