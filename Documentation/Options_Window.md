# 옵션창 MVVM 연결 가이드

## 구성

- `UI/ViewModel/Settings/NF_OptionsViewModel`: 설정 조회, 선택값, 적용/저장, 해상도 자동 복원, FieldNotify.
- `UI/Widget/NF_OptionsWidget`: WBP 부모. ViewModel 생성/연결 및 종료 처리, Esc/F10 닫기. C++ 위젯 트리 생성 없음.
- `NF_UIManagerSubsystem`: 설정한 WBP를 생성하고 입력/커서/스택을 관리.

## WBP 제작 및 등록 (에디터 작업 필요)

1. `NF_OptionsWidget`을 부모로 하는 Widget Blueprint `WBP_Options`를 생성한다.
2. Viewmodels 패널에서 `NF_OptionsViewModel`을 추가한다. 이름을 **OptionsViewModel**, Creation Type을 **Manual**로 설정한다. 부모가 창마다 하나를 만들어 공급하므로 별도로 생성하지 않는다.
3. 원하는 디자인으로 해상도/화면 모드/FPS ComboBoxString, 볼륨 Slider 3개, 안내 TextBlock 2개, 적용/유지/복원/닫기 버튼을 배치한다. 슬라이더 범위는 0~1로 설정한다.
4. 아래 표대로 One Way to Widget 바인딩과 입력 이벤트를 연결한다.
5. 전투/로비 PlayerController BP의 Class Defaults → NF|UI → OptionsWidgetClass에 WBP_Options를 지정한다. F10은 Controller.OpenOptions에서 UIManager.PushWidget을 호출한다.
6. 메뉴 버튼도 Controller.OpenOptions를 호출하거나, WBP 클래스를 직접 UIManager.PushWidget(UIOnly, 커서 표시, ZOrder 1000)에 전달한다.

UIManager에는 옵션 전용 클래스 참조나 등록 API가 없다. 기존 CreateWidget → AddToPlayerScreen → 스택 관리 흐름을 그대로 사용한다. 이전 UIManager의 SetOptionsWidgetClass/OpenOptions를 호출하던 BP는 위 호출 방식으로 변경해야 한다. 이전 DefaultGame.ini의 UIManager OptionsWidgetClass 설정도 더 이상 사용하지 않는다. WBP 에셋은 아직 제작해야 한다.
## 표시 바인딩

| ViewModel 필드 | 대상 |
|---|---|
| ResolutionIndex | 해상도 ComboBoxString.SetSelectedIndex |
| WindowModeIndex | 화면 모드 ComboBoxString.SetSelectedIndex |
| FrameRateIndex | FPS ComboBoxString.SetSelectedIndex |
| MasterVolume / SFXVolume / BGMVolume | 대응 Slider.Value |
| 같은 볼륨 필드 | 퍼센트 TextBlock: ToText(Percent) 변환 사용 |
| bCanApplyFrameRate | FPS 적용 버튼.IsEnabled |
| bDisplayPending | 유지/복원 버튼.IsEnabled 또는 bool→Visibility 변환 |
| Status / FrameRateStatus | 대응 TextBlock.Text |

목록과 선택값은 FieldNotify 필드다. ComboBoxString 목록은 OnOptionsViewModelReady 이벤트에서 GetOptionsViewModel로 가져와 채운다. 각 콤보를 ClearOptions한 뒤 ResolutionOptions / WindowModeOptions / FrameRateOptions를 순회하여 AddOption하고, 마지막에 해당 Index로 SetSelectedIndex한다. 목록 초기화 중에는 bool 가드로 OnSelectionChanged 전달을 차단한다. 데이터의 최초 바인딩이 목록 생성보다 먼저 실행될 수 있으므로 마지막 선택값 설정이 필요하다. 목록은 창을 열 때 초기화되며, 현재 지원 목록 밖의 FPS 제한도 유지한다. INDEX_NONE은 ClearSelection으로 처리한다.

## 입력 이벤트

이벤트는 GetOptionsViewModel을 대상으로 호출한다. 설정 시스템을 WBP에서 직접 호출하지 않는다.

| WBP 이벤트 | 함수 |
|---|---|
| 해상도 OnSelectionChanged → GetSelectedIndex | SelectResolution(Index) |
| 화면 모드 OnSelectionChanged → GetSelectedIndex | SelectWindowMode(Index) |
| FPS OnSelectionChanged → GetSelectedIndex | SelectFrameRate(Index) |
| 각 Slider.OnValueChanged | ChangeMaster / ChangeSFX / ChangeBGM(Value) |
| 해상도 적용 OnClicked | ApplyDisplay |
| 변경 유지 OnClicked | ConfirmDisplay |
| 복원 OnClicked | RevertDisplay |
| FPS 적용 OnClicked | ApplyFrameRateLimit |
| 닫기 OnClicked | 부모 위젯의 Close (ViewModel 함수 아님) |

FieldNotify 표시 바인딩은 Tick 폴링 없이 갱신한다. UI 갱신으로 입력 이벤트가 재진입해도 Select 함수는 설정 적용을 하지 않으며 같은 값은 알림을 반복하지 않는다.

## 동작 및 수명

- 해상도: HD/FHD/QHD, 전체 화면/창 모드. 15초 미확인 또는 창 제거 시 원래 값 복원. PIE에서는 적용하지 않음.
- FPS: 30/60/120/144/165/240/무제한. 명시적 적용 시 저장. 해상도 확인 중에는 적용 불가.
- 볼륨: 변경 즉시 반영, 창 제거 시 저장. 기존 AudioSubsystem 적용 범위는 동일.
- NativeDestruct에서 Shutdown을 실행하여 미확정 해상도 복원, 타이머 해제, 볼륨 저장, Manual 소스 분리를 수행한다. 다시 붙는 창은 상태를 다시 읽는다.
- 디자인 미리보기에서는 런타임 ViewModel 연결이 실행되지 않으므로 WBP의 디자인용 기본 텍스트를 사용한다.

## 검증

빌드는 실행하지 않았다. 정적 코드 및 diff 검사만 수행했다. C++ 컴파일 후 WBP를 제작/등록하고 아래를 실행 검증해야 한다.

- 열기/닫기/재열기와 현재 설정 표시
- 슬라이더 변경 및 재실행 후 복원, 퍼센트 텍스트 갱신
- FPS 선택과 적용/저장, 해상도 확인 중 버튼 비활성화
- 해상도 확인/자동 복원 시 콤보 및 메시지 갱신
- 맵 전환 중 해상도 복원과 타이머 정리
