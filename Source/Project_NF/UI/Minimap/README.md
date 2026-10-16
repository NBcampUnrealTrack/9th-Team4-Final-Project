# 미니맵 C++ 위젯 베이스

천리안/열쇠의 서버 공개 경로와 설정: [공통 공개 목록 가이드](REVEAL.md)

작성: 이정준 / Codex · 2026-10-15 · UE 5.7

`NF_MinimapWidget`은 지도 이미지와 외부에서 받은 마커를 그리는 UUserWidget이다. BP 에셋·HUD 배치·레이아웃은 생성하지 않았다. 기존 ViewModel 브랜치를 병합하거나 수정하지 않으며 이후 ViewModel에서 아래 API를 호출할 수 있다.

## BP에서 사용할 때

1. `NF_MinimapWidget` 부모로 Widget BP를 만든다. 별도의 Image/Canvas 이름 바인딩은 필요 없다.
2. 기존 HUD 안에서 이 위젯에 크기(예: 240×240)를 할당한다. 단독 Create Widget 사용 시에도 Owning Player와 화면 크기·배치를 지정한다. 이 클래스는 자체 HUD 생성/Add To Viewport를 하지 않는다.
3. Class Defaults의 `NF → Minimap`에서 지도 이미지·월드 범위·표시 거리·종류별 색을 설정한다.
4. 기본 Follow Owning Pawn을 켜면 본인 위치를 따라간다. 본인 화살표 방향은 Controller의 Control Rotation Yaw를 사용한다.
5. 팀원/몬스터/공개 적 데이터는 `Upsert Marker`로 전달한다. 동일 ID면 위치와 속성이 갱신된다.

북쪽은 월드 +X, 오른쪽은 +Y다. 지도 이미지를 이 방향으로 준비한다. Map World Min/Max는 **이미지 전체**의 XY 범위다. 기본 범위는 (-5000,-5000)~(5000,5000)cm이며 실제 레벨 크기에 맞춘다. 이미지의 U는 +Y, V는 -X 방향이다. 지도 이미지는 런타임 카메라 촬영이 아닌 정적 Texture2D를 사용한다.

Visible World Size는 화면 한 변의 전체 월드 길이다. 기본 4000cm면 본인을 중심으로 각 방향 2000cm가 보인다. 지도는 북쪽 고정으로 스크롤하고 본인 화살표만 회전한다. Z는 무시하며 층별 표시는 없다. 직사각형 슬롯에서는 짧은 변 크기의 정사각형으로 중앙에 그린다.

## 데이터 API

| 함수 | 역할 |
|---|---|
| UpsertMarker(Marker, Lifetime) | ID별 추가/갱신. 위치·종류·선택 아이콘·크기 제공 |
| RemoveMarker(Id) | 특정 아이콘 제거 |
| ClearMarkers() | 외부 마커 전체 제거. 본인 화살표는 별도 |
| ClearMarkersByType(Type) | 공개 적 등 특정 종류만 제거 |
| SetViewLocation(Location, ViewYaw) | Follow Owning Pawn을 끈 상태에서 중심/화살표 수동 지정 |
| SetVisibleWorldSize(cm) | 표시 범위 변경. 값이 작아지면 확대 |
| ProjectWorldToMinimap(...) | 외부 BP/C++에서 같은 좌표 변환 사용. 출력은 UMG 로컬 좌표 |

마커 종류는 Teammate / Monster / RevealedEnemy / Objective / Player다. 아이콘 텍스처가 없으면 종류별 색의 사각형이다. 텍스처도 종류 색으로 틴트된다. 표시 범위 밖의 마커는 숨기지만 삭제하지 않는다. 이미지 범위 밖 배경은 Background Color로 표시한다.

Lifetime 0은 만료 없음이다. 양수면 **이 클라이언트가 수신한 시점부터의 남은 초**이며 로컬 월드 시간을 사용한다. 같은 ID를 재갱신하면 만료 시간도 갱신된다. 위치만 갱신할 때 전체 지속시간을 반복 전달하면 만료가 계속 연장되므로, 데이터 공급자가 계산한 남은 시간을 전달하거나 스킬 종료 시 Remove/ClearByType으로 제거한다. 서버 절대 타임스탬프를 Lifetime에 그대로 넣지 않는다. 위젯이 재생성되면 외부 데이터 공급자가 현재 상태를 다시 전달한다.

## GAS·멀티플레이 경계

이 위젯에는 자동 몬스터 등록, 팀 판별, GAS 발동, 네트워크 복제가 없다. 로컬 플레이어 위젯으로 사용하고 데디케이티드 서버에서는 생성하지 않는다.

### 임시 전체 플레이어 표시

Class Defaults → NF → Minimap → Temporary의 **Temporary Show All Players** 기본값은 false다. 옵션을 켜면 위젯이 로컬 월드의 플레이어 Pawn 목록을 0.5초마다 확인하고 위치는 매 프레임 읽어 Player 색상의 사각형으로 표시한다. 팀 검사는 하지 않으며 몬스터와 SpectatorPawn은 제외한다. 원격 클라이언트에서 Controller가 없을 수 있어 PlayerState 보유 여부도 확인한다. 본인은 기존 중앙 화살표로만 표시한다.

실제 이동 위치는 UE의 기존 Pawn 복제 결과를 사용하며 새 RPC/서버 전체 위치 방송은 추가하지 않았다. 따라서 클라이언트에 아직 복제되지 않은 Pawn이나 미니맵 표시 범위 밖 플레이어는 표시되지 않는다. Visible World Size를 늘리면 지도 표시 범위는 넓어지지만 네트워크 복제 범위를 넓히지는 않는다. 재접속/리스폰으로 새 Pawn이 생기면 다음 목록 갱신 때 반영한다.

임시 마커는 외부 UpsertMarker 데이터와 별도로 관리한다. ViewModel을 연결할 때 **Temporary Show All Players를 끄고** 데이터 API를 사용한다. Remove/ClearMarkers는 외부 마커에만 적용되며 임시 자동 표시를 중단하려면 해당 옵션을 끈다. 같은 플레이어를 외부 API와 임시 수집 양쪽으로 전달하면 중복 표시될 수 있다.

천리안은 서버가 공개 대상을 결정하고 허용된 클라이언트에 위치/남은 시간을 전달한 뒤 이 API로 표시한다. 복제 거리 밖 대상도 표시하려면 Actor 참조만 전달하지 않고 좌표 데이터도 전달해야 한다. 같은 적이 여러 공개 효과로 보이는 경우 합산 수명·참조 관리는 데이터 공급자가 맡는다. UI의 타입/색 필터로 적 정보 공개 권한을 대신하지 않는다.

이번 작업은 C++ 기반과 좌표 변환 자동화 테스트(`NF.UI.Minimap.Projection`) 작성까지다. 빌드·자동화 테스트 실행·PIE 표시 확인은 수행하지 않았다.
