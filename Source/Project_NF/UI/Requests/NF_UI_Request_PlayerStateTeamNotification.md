# HUD 연동 요청 — PlayerState 팀 변경 알림

- 작성일: 2026-10-26
- 대상: 플레이어 담당자 (PlayerState 담당)
- 연동 담당: 사용자 (Core·UI)

## 목적

HUD가 먼저 생성되고 팀 ID가 나중에 복제되면 팀원 목록이 비어 있을 수 있습니다. 본인 또는 다른 플레이어의 팀 ID가 클라이언트에 반영될 때 UI가 목록을 다시 조회할 수 있도록 PlayerState에 변경 알림을 추가합니다.

이 문서는 구현 요청과 예시 코드입니다. 실제 C++ 파일에는 적용하지 않았습니다.

## 요청 사항

1. 기존 `NF_TeamID`를 `ReplicatedUsing`으로 변경합니다.
2. 클라이언트에서 새 팀 ID가 반영된 뒤 `OnTeamIDChanged`를 발생시킵니다.
3. 서버의 `SetNFTeamID()`에서도 값이 변경되면 같은 알림을 발생시킵니다.
4. 팀 ID 변경은 `SetNFTeamID()`를 사용하도록 유지합니다.
5. 기존 `DOREPLIFETIME(ThisClass, NF_TeamID)` 등록은 유지합니다.

## NF_PlayerState.h

델리게이트 선언은 `NF_PlayerState.generated.h` include 아래, 클래스 선언 위에 추가합니다. 기존 클래스 내부에는 아래 멤버를 추가하고, 기존 `NF_TeamID` 선언을 교체합니다. `SetNFTeamID()`의 기존 선언은 유지합니다.

```cpp
// generated.h include 아래, 클래스 선언 위
DECLARE_MULTICAST_DELEGATE(FNF_OnTeamIDChanged);

// ANF_PlayerState 클래스 내부
public:
    FNF_OnTeamIDChanged OnTeamIDChanged;

    // 기존 NF_TeamID 선언 교체
    UPROPERTY(
        ReplicatedUsing = OnRep_NFTeamID,
        BlueprintReadOnly,
        Category = "Core|ID")
    int32 NF_TeamID = INDEX_NONE;

private:
    UFUNCTION()
    void OnRep_NFTeamID();
```

## NF_PlayerState.cpp

기존 `SetNFTeamID()` 구현을 교체하고 `OnRep_NFTeamID()`를 추가합니다.

```cpp
bool ANF_PlayerState::SetNFTeamID(int32 NewTeamID)
{
    if (!HasAuthority() || NewTeamID < 1)
    {
        return false;
    }

    if (NF_TeamID == NewTeamID)
    {
        return true;
    }

    NF_TeamID = NewTeamID;

    // C++ 서버의 직접 대입에서는 RepNotify가 자동 호출되지 않는다.
    OnTeamIDChanged.Broadcast();

    ForceNetUpdate();
    return true;
}

void ANF_PlayerState::OnRep_NFTeamID()
{
    // 클라이언트에서 새 팀 ID가 반영된 후 알림.
    OnTeamIDChanged.Broadcast();
}
```

기존 `GetLifetimeReplicatedProps()`의 다음 등록은 유지합니다.

```cpp
DOREPLIFETIME(ThisClass, NF_TeamID);
```

## 동작 규칙

- 서버에서는 Setter로 실제 값이 변경될 때 알림을 발생시킵니다.
- 클라이언트에서는 팀 ID의 RepNotify에서 알림을 발생시킵니다.
- 알림 처리 중 `NF_TeamID`를 조회하면 변경된 값을 읽을 수 있어야 합니다.
- 같은 팀 ID를 다시 설정하면 성공을 반환하지만 변경 알림은 발생시키지 않습니다.
- 현재 Setter의 서버 권한 검사와 1 이상의 팀 ID만 허용하는 규칙은 유지합니다.

델리게이트 자체가 네트워크로 전송되는 것은 아닙니다. 팀 ID가 복제된 뒤 각 클라이언트에서 로컬 알림이 발생하는 구조입니다. 서버에서 알림을 발생시키는 것만으로 클라이언트 HUD가 갱신되지는 않습니다.

## Core·UI 담당 후속 작업

사용자가 GameState에서 각 PlayerState의 팀 변경 알림을 구독하고, 입장·퇴장 알림과 함께 ViewModel에 전달합니다. TeamPanelViewModel은 알림을 받으면 기존 `RefreshMembers()`를 호출합니다.

HUD가 늦게 생성되어 이전 알림을 받지 못한 경우에는 구독 직후 현재 명단을 조회해 복원합니다. PlayerState에서 Widget이나 ViewModel을 직접 참조할 필요는 없습니다.

팀원 목록 구성 이후 체력·실드 갱신은 기존 GAS Attribute 구독을 유지합니다. 이번 요청은 팀 ID 변경 알림에 한정하며 GAS Attribute나 MVVM 바인딩 변경을 요구하지 않습니다.

## 완료 확인

- [ ] 서버에서 최초 팀 배정 시 변경 알림이 발생합니다.
- [ ] 본인의 팀 ID가 늦게 복제되어도 클라이언트에서 알림이 발생합니다.
- [ ] 다른 플레이어의 팀 ID가 늦게 복제되어도 관찰 클라이언트에서 알림이 발생합니다.
- [ ] 본인 또는 다른 플레이어의 팀 변경 시 새 값을 조회할 수 있습니다.
- [ ] 같은 팀 ID를 다시 설정할 때 불필요한 변경 알림이 발생하지 않습니다.
- [ ] 기존 팀 배정 경로는 `SetNFTeamID()`를 사용합니다.
