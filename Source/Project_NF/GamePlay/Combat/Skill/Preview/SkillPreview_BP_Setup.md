# 공통 프리뷰 BP 조합 가이드

작성: 스킬 담당 (구현: Codex) | 2026-10-13

## 공통 규칙

- 부모는 `NF_SkillPreviewActor`다. 스킬마다 C++ 프리뷰 액터를 만들지 않는다.
- 필요한 기능 컴포넌트를 BP에 직접 부착하고 이벤트에서 명시적으로 연결한다.
- GA의 `Preview Actor Class`에 만든 BP를 지정한다. 이 값은 기본으로 자동 생성하지 않는다.
- `Preview Started`: Source Avatar/Ability 설정 직후 한 번 호출. 조준 시작 처리.
- `Update Preview`: 초기화 직후 및 홀딩 중 매 프레임 호출. 별도 Event Tick을 연결할 필요 없다.
- `Preview Stopped`: 표시 액터 종료 시 호출. 표시 정리 처리.
- `Get Preview Data`: GA의 `Get Skill Preview Data`에서 최신 공통 구조체를 가져온다. 특정 스킬로 Cast할 필요 없다. 출력 구조체는 Break NF Skill Preview Data로 필요한 필드를 꺼낸다.
- 조준 설정을 프리뷰 BP에 중복 입력하지 않는다. 사거리, 깊이, 반경, 투척 속도/중력 등은 GA와 공격 액터 설정에서 제공한다.
- 일반 BP BeginPlay 시점에는 Source Avatar/Ability가 아직 지정되지 않았을 수 있다. 초기 데이터가 필요한 처리는 Preview Started에 연결한다.
- GA가 생성/종료하고, 공통 액터가 로컬 시전자 유효성 및 갱신 수명을 관리한다. 실제 GE 적용/피해는 서버의 GA/공격 액터에서 처리한다.

## 현재 공습 BP 연결

이미 부모를 공통 액터로 바꾼 `BP_AirStrikePreview`에 다음 두 컴포넌트를 추가한다.

- `NF Ground Targeting Component`
- `NF Area Indicator Component`

Update Preview 이벤트의 실행 흐름:

```text
Update Preview
 → Get Preview Data → Branch(Return Value)
    False: AreaIndicator.Hide Area
    True:
      → GroundTargeting.Find Ground Target
          Source Avatar = Get Source Avatar
          Target Range = Data.Target Range
          Ground Search Depth = Data.Ground Search Depth
      → Branch(Return Value)
          False: AreaIndicator.Hide Area
          True: AreaIndicator.Show Area
                  Location = Break Hit Result(Out Ground Hit).Impact Point
                  Radius = Data.Display Radius
                  Surface Normal = (0, 0, 1)

Preview Stopped → AreaIndicator.Hide Area
```

장판 컴포넌트의 색/데칼은 BP의 컴포넌트 디테일에서 편집한다. 사거리/표시 반경은 GA가 제공하므로 별도 값으로 덮어쓰지 않는다. 실제 공습 GA도 같은 GroundTargeting 계산 함수를 사용한다.

공습 GA BP의 Preview Actor Class는 계속 `BP_AirStrikePreview`를 지정한다. `NF_AirStrikePreviewActor` 전용 C++ 클래스는 삭제했다. 변경 전 부모를 참조하는 다른 BP가 있다면 공통 부모로 재부모화해야 한다.

## 수류탄을 같은 방식으로 연결하기

기존 수류탄 BP의 전용 부모를 아직 바꾸지 않았다면 현재 동작은 유지된다. 전환 시에는:

1. 공통 부모의 새 프리뷰 BP를 만들거나 기존 프리뷰 BP를 공통 부모로 변경한다.
2. `NF Projectile Trajectory Component`와 `NF Area Indicator Component`를 추가한다.
3. Preview Started → Trajectory.Start Preview.
4. Update Preview → Get Preview Data. 실패하면 AreaIndicator.Hide Area.
5. 성공하고 Has Projectile Path가 true이면 Trajectory.Update Preview에 데이터의 Launch Location/Launch Velocity/Projectile Radius/Gravity Z/Flight Time을 전달한다.
6. Trajectory.Get Predicted Impact가 true이면 AreaIndicator.Show Area에 Hit.Impact Point, Data.Display Radius, Hit.Impact Normal을 전달한다. false이면 Hide Area.
7. Preview Stopped → Trajectory.Stop Preview + AreaIndicator.Hide Area.
8. 수류탄 GA의 Preview Actor Class에 이 BP를 지정한다.

`NF_GrenadePreviewActor`는 기존 부모 참조를 보호하는 호환 클래스다. 새 스킬은 이 전용 액터 방식을 복제하지 않는다. 모든 수류탄 BP가 공통 부모로 전환되면 참조 확인 후 함께 삭제할 수 있다.

## 새로운 스킬

공통 GA의 Get Skill Preview Data 이벤트를 재정의하거나 C++ 구현에서 데이터를 채운다. 프리뷰 BP는 같은 Get Preview Data를 사용한다. 새로운 표시/대상 판정 방식이 있을 때만 기능 컴포넌트를 추가한다.

데이터를 제공하지 않은 GA의 기본 구현은 false다. 컴포넌트를 부착하기만 하고 Update Preview 연결을 하지 않으면 표시가 생기지 않는다. 경고 장판처럼 다른 플레이어에게 표시할 때는 별도의 서버 복제/GameplayCue 경로를 사용하며 이 로컬 조준 액터와 구분한다.

이번 작업은 C++ 구조/API 및 문서를 수정했다. `.uasset`의 컴포넌트/이벤트 노드는 자동 수정하지 않았고, 빌드/에디터 실행 검증은 수행하지 않았다.
