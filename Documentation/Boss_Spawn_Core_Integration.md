# 보스 출현 Core 연동

## 레벨 설정

1. 전투 레벨의 GameMode를 `ANF_GameMode` 파생 클래스로 설정한다.
2. Persistent Level에 `ANF_BossSpawner`를 하나 배치한다. 액터 Transform이 출현 위치·회전·크기다.
3. 스포너의 `BossClass`에 GAS 패턴과 StateTree가 설정된 실제 보스 BP를 지정한다.
4. 같은 보스를 레벨에 미리 배치하거나 별도 BP에서 생성하지 않는다. 이미 활성 보스가 있으면 출현 요청은 실패한다.
5. 파밍이 끝나기 전에 스포너의 BeginPlay가 실행되어야 한다. 늦게 스트리밍되는 레벨에 두지 않는다.

## 실행 흐름

서버 스포너 BeginPlay → Registry 등록 → GameMode 등록 → 파밍 종료 → Core 요청 → 스포너의 동기 SpawnActor → BeginPlay에서 GAS·AI 초기화 → 준비 상태 확인 → 반환 → Core 검증 및 Boss 페이즈 진입.

Registry 등록의 성공은 Core에 동일한 UObject 델리게이트가 등록되었음을 뜻한다. 클라이언트, UObject 소유자가 없는 델리게이트, 중복 등록, 출현 요청/취소 이후의 등록은 거부한다. 스포너 EndPlay와 Registry Deinitialize는 원래 등록 소유자로 Core 바인딩을 해제한다.

스포너는 요청을 한 번만 처리한다. 생성 위치 충돌은 가능한 경우 조정하고 해결되지 않으면 생성하지 않는다. 반환 전에 ASC Owner/Avatar, 체력, BeginPlay 완료, StateTree 실행 상태를 검사한다. StateTree가 없는 경우 기존 InitialPatternAbility fallback이 부여되어 있어야 한다. 실패 또는 생성 중 매치 종료 시 이번 요청으로 만든 보스만 Destroy하고 nullptr를 반환한다.

기존 WakeBoss 호출 정책은 유지한다. AI 준비 완료와 전투 시작은 별개이므로 기존 각성 트리거가 있어야 한다. 스포너 종료는 정상 출현한 보스를 제거하지 않는다.

## 검증 상태와 실행 점검

C++ 구현과 정적 diff 검토를 수행했다. 빌드 실행 권한 요청이 거절되어 컴파일, BP/레벨 설정, Dedicated Server 실행은 검증하지 못했다.

- 정상: 파밍 종료 시 한 번 생성되고 Core 활성 보스와 Registry 활성 보스가 같으며 Boss 페이즈로 전환되는지 확인한다.
- 초기화 실패: 잘못된 AI 설정 또는 생성 위치 충돌 시 실패가 Core로 전달되고 부분 생성 보스가 남지 않는지 확인한다.
- 등록 수명: 중복 스포너가 기존 핸들러를 덮어쓰지 않고, 등록하지 못한 스포너 종료가 기존 핸들러를 해제하지 않는지 확인한다.
- 서버/클라이언트: Dedicated Server만 생성하고 클라이언트에는 복제된 보스만 존재하는지 확인한다.
- 생성 중 종료: 보스 BeginPlay 콜백이 매치를 종료하면 생성 보스가 정리되는지 확인한다.
