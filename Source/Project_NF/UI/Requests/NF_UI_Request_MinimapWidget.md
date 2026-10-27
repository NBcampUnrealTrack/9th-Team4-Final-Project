# HUD 연동 요청 — 미니맵 Widget의 MVVM 분리

작성일: 2026-10-25  
대상: 미니맵 위젯 담당자

## 작성자 코멘트
<요청 내용>

- HUD를 MVVM으로 구성하므로, 미니맵 Widget은 VM에서 전달한 표시 상태를 받아 좌표 변환과 그리기를 담당하도록 변경을 요청

- MVVM 패턴 적용을 위해 UNF_MinimapViewModel을 생성해두었습니다.

- 원활한 작업을 위해 ENF_MinimapMarkerType에 FNF_MinimapSnapshot struct를 추가해 놓았습니다.

- 해당 구조체는 본인 위치·방향과 마커 목록을 하나로 전달해 서로 다른 시점의 값이 섞이는 것을 방지해줍니다.

- 정준님의 코드에 제가 추가한 부분이기 때문에 먼저 Dev를 작업 브랜치에 병합 후 시작해주시기 바랍니다.

- 하단 MD파일 참조

## 목적과 작업 분담

HUD를 MVVM으로 구성하므로, 미니맵 Widget은 VM에서 전달한 표시 상태를 받아 좌표 변환과 그리기를 담당하도록 변경을 요청합니다.

- 미니맵 담당: Snapshot 수신 함수, Widget의 게임 상태 직접 조회 제거, 기존 그리기 기능 유지.
- HUD UI 담당: MinimapViewModel 구현, 본인·팀원·공개 마커 조회와 중복 처리, HUD에서 VM 생성·연결·해제, WBP 바인딩.

이 요청서는 Widget 측 작업 범위입니다. 게임 모델이나 공개 권한·복제 로직을 변경하는 요청은 아닙니다.

## 현재 확인한 상태

- `UI/Minimap/NF_MinimapTypes.h`에 `FNF_MinimapSnapshot`이 이미 선언되어 있습니다. 같은 구조체를 중복 추가할 필요는 없습니다.
- `NF_MinimapWidget`에는 아직 `SetSnapshot()`이 없습니다.
- `NativeTick()`에서 본인 Pawn 위치와 팀원을 직접 조회합니다.
- `NativePaint()`에서 Controller의 공개 마커 Registry를 직접 조회합니다.
- 기존 좌표 변환, 배경 지도, 마커, 본인 화살표 그리기는 재사용할 수 있습니다.

위 내용은 작성일의 C++ 소스 기준입니다.

## 1. Snapshot 수신 함수 추가

기존 Snapshot의 필드는 다음과 같습니다.

| 필드 | 의미 |
|---|---|
| `bHasView` | 표시 기준 위치 유효 여부 |
| `ViewLocation` | 지도 중심의 월드 위치 |
| `ViewYaw` | 본인 화살표 방향 |
| `Markers` | 현재 표시할 전체 마커 목록 |

`NF_MinimapWidget.h`에 Blueprint에서 바인딩할 수 있는 함수를 추가해주세요.

```cpp
UFUNCTION(BlueprintCallable, Category = "NF|Minimap")
void SetSnapshot(const FNF_MinimapSnapshot& InSnapshot);
```

CPP 구현 예시입니다. 내부 저장 방식은 기존 코드에 맞춰 조정해도 됩니다.

```cpp
void UNF_MinimapWidget::SetSnapshot(
    const FNF_MinimapSnapshot& InSnapshot)
{
    bHasView = InSnapshot.bHasView;
    Center = InSnapshot.ViewLocation;
    CenterYaw = InSnapshot.ViewYaw;

    Entries.Reset();

    for (const FNF_MinimapMarker& Marker : InSnapshot.Markers)
    {
        if (Marker.Id.IsNone() ||
            Marker.WorldLocation.ContainsNaN() ||
            !FMath::IsFinite(Marker.Size) ||
            Marker.Size <= 0.0f)
        {
            continue;
        }

        FNF_MinimapEntry& Entry = Entries.AddDefaulted_GetRef();
        Entry.Marker = Marker;
        Entry.ExpiresAt = 0.0;
    }

    InvalidateLayoutAndVolatility();
}
```

수신 규약:

- Snapshot은 변경분이 아닌 전체 현재 상태입니다. 빈 목록도 적용하여 이전 마커를 제거해주세요.
- `bHasView == false`이면 이전 중심·본인 화살표·마커가 유효한 상태처럼 남지 않게 처리해주세요. 배경 표시 여부는 기존 디자인 정책을 유지해도 됩니다.
- 비정상 중심 좌표나 방향 값도 검증해주세요. 유효하지 않다면 표시 기준을 무효화하는 방식으로 처리할 수 있습니다.
- 효과 종료·만료 여부는 모델/VM이 판단하고 다음 목록에서 제외합니다. Snapshot 경로의 마커를 Widget이 별도로 만료시키지 않아도 됩니다.
- 마커 색상·크기·지도 배율 등 기존 표시 설정의 적용 규칙을 정리해주세요. VM의 `Marker.Size`와 Widget 설정이 충돌하지 않도록 최종 적용 기준을 알려주세요.

## 2. Widget의 직접 조회 제거

| 현재 Widget의 처리 | 요청 변경 |
|---|---|
| `NativeTick()`에서 `GetOwningPlayerPawn()` 위치 조회 | 제거하고 Snapshot 중심·방향 사용 |
| `UpdateTeammates()`의 Pawn 수집·팀 판정 | VM 담당으로 이전, Widget의 수집 경로 제거 |
| `NativePaint()`에서 Controller/Registry 조회 | 제거하고 수신한 마커만 그리기 |
| 공개 팀원과 일반 팀원 마커 중복 판정 | VM 담당으로 이전 |
| `GetViewerTeamID()` 등 게임 상태 조회 | Widget 내부 사용 제거 |
| 좌표 변환·지도 배경·마커·본인 화살표 그리기 | 유지 |
| 지도 텍스처·색상·배율·클리핑 설정 | View의 표시 기능으로 유지 가능 |

`ArePlayerStatesTeammates()`와 기존 프로퍼티가 다른 코드·테스트·BP에서 참조되는지는 확인 후 정리해주세요. 호환용으로 남기더라도 Snapshot 표시 경로에서는 게임 상태를 다시 읽지 않아야 합니다.

Tick 자체를 반드시 없애야 하는 것은 아닙니다. 표시 보간 등 View의 연출에 필요한 경우 유지할 수 있지만, 게임 데이터 수집과 팀·공개 상태 판단은 VM에서 처리합니다.

## 3. 기존 마커 API와 설정 호환

이 구성에서는 VM이 표시 마커 전체를 공급합니다.

- 기존 `UpsertMarker()`, `RemoveMarker()`, `ClearMarkers()` 호출처가 있다면 목록을 알려주세요. Snapshot 교체로 기존 외부 마커가 사라질 수 있으므로 HUD UI 담당이 VM 공급 경로로 통합해야 합니다.
- 기존 직접 조회 옵션인 `Follow Owning Pawn`, `Show Teammates`, `Show Registered Reveals`가 Snapshot 수신 후 자동 수집을 다시 실행하지 않도록 정리해주세요.
- 순수 표시 필터를 남긴다면 VM에서 판정한 팀·공개 권한을 Widget이 다시 계산하지 않고, 수신한 마커 종류에만 적용해주세요.
- 지도 좌표계는 기존 규칙인 북쪽 월드 +X, 오른쪽 +Y를 유지해주세요.

## 4. HUD UI 담당이 연결할 부분

```text
내 Pawn·팀원 상태·공개 허용 Registry
    → MinimapViewModel.Snapshot (FieldNotify)
    → WBP의 단방향 바인딩
    → MinimapWidget.SetSnapshot()
    → 기존 좌표 변환과 그리기
```

HUD UI 담당이 VM의 `Snapshot`을 Widget의 `SetSnapshot()`에 연결합니다. Widget에서 VM을 생성하거나 모델을 다시 찾을 필요는 없습니다.

기존 공개 규칙도 VM에서 유지하겠습니다. 적은 허용된 Registry 목록만 표시하고, 일반 팀원과 천리안 공개 팀원의 중복은 제거하되 열쇠 목적지 표시는 별도로 유지합니다.

## 완료 확인

- [ ] `SetSnapshot()`을 WBP의 바인딩 목적지로 사용할 수 있습니다.
- [ ] 빈 Snapshot을 받으면 이전 마커가 제거됩니다.
- [ ] 유효한 Pawn이 없는 상태를 받아 이전 본인 표시를 정리할 수 있습니다.
- [ ] Widget의 Tick/Paint에서 Pawn·팀 정보·공개 Registry를 직접 조회하지 않습니다.
- [ ] 지도·화살표·종류별 마커·색상·클리핑이 기존처럼 그려집니다.
- [ ] 마커 종료는 다음 Snapshot 목록에서 제거되는 방식으로 반영됩니다.
- [ ] 기존 마커 API 호출처와 설정 호환 여부를 공유했습니다.

## 전달받고 싶은 내용

- 수정 완료한 Widget의 Snapshot 수신 함수와 적용 규칙
- 기존 API 또는 BP 설정 중 변경되는 항목
- 외부 마커 공급 호출처와 HUD UI 측 전환이 필요한 항목
- 컴파일·표시 확인 결과와 남은 제약
