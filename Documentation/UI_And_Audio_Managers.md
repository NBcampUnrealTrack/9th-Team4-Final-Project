# UI 매니저와 오디오 매니저 사용법

이 문서는 `UNF_UIManagerSubsystem`과 `UNF_AudioSubsystem`을 다른 게임 코드에서 사용하는 방법을 설명합니다.

## UI 매니저

`UNF_UIManagerSubsystem`은 `ULocalPlayerSubsystem`입니다. 로컬 플레이어마다 별도로 존재하며, 해당 플레이어의 화면에 위젯을 추가하고 입력 모드와 마우스 커서를 관리합니다.

헤더:

```cpp
#include "UI/SubSystem/NF_UIManagerSubsystem.h"
```

### 위젯 열기

로컬 플레이어 컨트롤러가 있는 곳에서 매니저를 얻고 위젯 클래스를 전달합니다.

```cpp
if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
{
    if (UNF_UIManagerSubsystem* UIManager =
        LocalPlayer->GetSubsystem<UNF_UIManagerSubsystem>())
    {
        UUserWidget* Widget = UIManager->PushWidget(
            MyWidgetClass,
            ENF_UIInputMode::GameAndUI,
            true,  // 커서 표시
            100);  // ZOrder
    }
}
```

`MyWidgetClass`는 `UUserWidget` 또는 그 하위 클래스의 클래스 참조입니다. 반환된 위젯 포인터는 나중에 닫을 때 사용하므로 필요한 경우 호출 측 객체에 저장합니다.

입력 모드는 다음 중 하나를 선택합니다.

| 모드 | 동작 |
|---|---|
| `GameOnly` | 게임 입력만 받습니다. |
| `GameAndUI` | 게임과 UI 입력을 함께 받습니다. 메뉴와 게임을 동시에 조작해야 할 때 사용합니다. |
| `UIOnly` | UI 입력만 받습니다. |

### 위젯 닫기

매니저가 연 위젯은 `RemoveWidget`으로 닫습니다.

```cpp
if (IsValid(UIManager) && IsValid(Widget))
{
    UIManager->RemoveWidget(Widget);
}
```

여러 화면을 쌓은 경우 `PopTopWidget()`은 가장 위 화면을 닫고, `PopAllWidgets()`는 전부 닫습니다. 매니저를 통해 연 위젯을 직접 `RemoveFromParent()`로 제거하면 UI 스택에 오래된 항목이 남을 수 있으므로 매니저 API로 닫아 주세요.

위젯 클래스가 이미 스택에 있으면 `PushWidget()`은 같은 클래스를 다시 쌓지 않고 기존 위젯을 재사용해 최상단으로 올립니다.

### 알림 메시지

로컬 화면에 잠깐 나타나는 알림은 다음처럼 표시합니다.

```cpp
UIManager->ShowAnnouncement(
    FText::FromString(TEXT("보스가 나타났습니다!")),
    3.0f);
```

이 알림은 모달 UI 스택이나 입력 모드를 바꾸지 않습니다. 현재 구현은 C++ API입니다.
알림 위젯에는 `SetMessage()`로 문구만 전달하며, 표시 시간·화면 제거·참조 정리는 `UNF_UIManagerSubsystem`이 관리합니다.

## 오디오 매니저

`UNF_AudioSubsystem`은 `UGameInstanceSubsystem`입니다. 한 게임 인스턴스에서 한 개의 BGM과 Ambient를 관리하고, 로컬 효과음 재생과 볼륨 배율을 제공합니다. 레벨 전환 뒤에도 GameInstance가 유지되는 동안 매니저가 유지됩니다.

헤더:

```cpp
#include "Core/Audio/NF_AudioSubsystem.h"
```

### C++에서 매니저 얻기

월드가 있는 UObject에서 정적 `Get()`을 사용합니다.

```cpp
if (UNF_AudioSubsystem* Audio = UNF_AudioSubsystem::Get(this))
{
    Audio->PlaySFX(MySound);
}
```

### 효과음 재생

UI 소리처럼 위치가 필요 없는 소리는 2D로 재생합니다.

```cpp
Audio->PlaySFX(MySound, 1.0f, 1.0f, 0.0f);
```

월드 위치에서 들려야 하는 소리는 위치 재생 함수를 사용합니다.

```cpp
Audio->PlaySFXAtLocation(
    MySound,
    SoundLocation,
    FRotator::ZeroRotator,
    1.0f,                 // 재생별 볼륨 배율
    1.0f,                 // 피치
    0.0f,                 // 시작 시각
    AttenuationSettings,
    ConcurrencySettings);
```

재생별 볼륨에 효과음 볼륨과 마스터 볼륨이 곱해집니다. 위치 효과음의 감쇠 거리와 동시 재생 제한은 사운드 에셋 설정 또는 전달한 `USoundAttenuation`·`USoundConcurrency` 에셋에서 정합니다.

### BGM과 Ambient

```cpp
Audio->PlayBGM(BGMSound, 1.0f, 1.0f);       // 페이드 시간, 재생별 볼륨
Audio->PlayAmbient(AmbientSound, 1.0f, 1.0f);

Audio->StopBGM(1.0f);                       // 페이드아웃 시간
Audio->StopAmbient(1.0f);
```

BGM이나 Ambient가 바뀌면 기존 소리는 페이드아웃하고 새 소리는 페이드인합니다. 반복 여부는 사운드 에셋에서 설정합니다. 같은 에셋이 이미 재생 중이면 새 컴포넌트를 만들지 않고 볼륨을 갱신합니다.

### 볼륨 설정

각 볼륨은 `0.0`부터 `1.0`까지 설정합니다.

```cpp
Audio->SetMasterVolume(0.8f);
Audio->SetSFXVolume(0.7f);
Audio->SetBGMVolume(0.5f);
Audio->SetAmbientVolume(0.4f);

const float CurrentSFXVolume = Audio->GetSFXVolume();
```

현재 값은 메모리에만 있습니다. 게임 재실행 후에도 설정을 유지하려면 별도의 저장 설정 연결이 필요합니다.

BGM 덕킹은 사용자 BGM 볼륨을 바꾸지 않고 일시적으로 BGM을 낮추거나 복구합니다.

```cpp
Audio->SetBGMDuckMultiplier(0.25f, 0.3f); // 낮춤
Audio->SetBGMDuckMultiplier(1.0f, 0.3f);  // 복구
```

## 멀티플레이에서의 재생 위치

두 매니저는 네트워크 RPC가 아닙니다. `UNF_AudioSubsystem`은 실행 중인 프로세스의 게임 인스턴스에서 로컬로 재생하며, 전용 서버에서는 재생 요청을 무시합니다. 서버에서 `PlaySFXAtLocation()`을 한 번 호출한다고 클라이언트에 소리가 전달되지는 않습니다.

모든 플레이어에게 들려야 하는 게임플레이 소리는 서버가 발생 시점을 확정하고, GameplayCue 또는 복제 RPC를 통해 각 클라이언트가 이벤트를 받게 해야 합니다. 그 이벤트를 받은 각 클라이언트에서 `UNF_AudioSubsystem::Get(...)` 후 재생 함수를 호출합니다. 로컬 UI 소리나 해당 플레이어에게만 필요한 효과음은 그 로컬 플레이어에서 직접 호출하면 됩니다.

## 블루프린트에서 사용

- 위젯 클래스 참조를 준비한 뒤 `Get Local Player Subsystem`으로 `NF UI Manager Subsystem`을 가져와 `Push Widget`을 호출합니다. `Input Mode`, 커서 표시, ZOrder를 지정합니다.
- 닫을 위젯 참조를 보관하고 `Remove Widget`에 전달합니다. 화면 스택 최상단이나 전체를 닫을 때는 `Pop Top Widget` 또는 `Pop All Widgets`를 사용할 수 있습니다.
- 오디오 노드는 `NF Audio Subsystem`의 정적 `Get` 노드에서 시작합니다. 월드 컨텍스트 입력에는 현재 액터나 위젯을 연결하고, 반환된 매니저에서 `Play SFX`, `Play SFX at Location`, `Play BGM`, `Play Ambient`, 볼륨 setter를 호출합니다.
- BGM은 루프하도록 사운드 에셋을 설정합니다. UI/2D 효과음과 위치 효과음은 적절한 재생 함수를 선택합니다.

## 기존 연결

플레이어 Motion Matching 컴포넌트의 점프·착지 사운드는 오디오 매니저의 `PlaySFXAtLocation()`을 사용합니다. 다른 C++나 블루프린트에서 직접 `Play Sound`를 호출하는 부분은 자동으로 전환되지 않으므로, 볼륨 매니저를 통과시킬 필요가 있는 호출부에서 새 API로 바꾸어야 합니다.
