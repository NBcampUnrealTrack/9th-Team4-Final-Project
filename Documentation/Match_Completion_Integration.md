# 정상 매치 종료 연동

## 흐름

Core가 GameState에 확정 결과를 기록하고 결과 표시 시간을 마친 후 `RequestMatchCompleted`를 호출한다. GameMode BeginPlay에서 DedicatedMatchSubsystem에 연결하며, 정상 승리는 Abort 경로를 사용하지 않는다.

서버는 결과를 값으로 보관하고 중복 종료를 차단한다. 전투용 `ANF_PlayerController`와 로비용 `ANF_MatchLobbyPlayerController`의 Reliable Client RPC로 동일한 결과를 전송한다. 클라이언트는 승리 팀을 다시 계산하지 않는다. 별도 Controller를 사용하는 레벨은 해당 RPC 연결을 추가해야 하며, 미지원 Controller는 서버 오류 로그를 남긴다.

클라이언트 GameInstance의 MatchmakingSubsystem은 결과를 보관하고 `OnMatchCompleted`를 방송한다. 매칭 명단, 서버 주소, 큐와 파티 정보를 비우고 Ended로 전환한 뒤 `GameDefaultMap`으로 절대 이동한다. 정상 종료 시 파티도 해산하는 정책이다. `OnMatchmakingFailed`는 사용하지 않는다. 새 맵 UI는 `GetLastMatchResult()`로 이전 결과를 조회할 수 있다.

## 복귀 맵

`DefaultEngine.ini`의 `/Script/EngineSettings.GameMapsSettings` → `GameDefaultMap`을 사용한다. 현재 설정은 `/Game/ProjectNF/Persnal/GyeongHwan/Test_GH.Test_GH`이다. 실제 출시 메뉴/로비 경로는 프로젝트에서 확인 및 지정해야 한다. 맵이 패키징 대상에 포함되어야 한다.

## 비동기 정리 정책

- 클라이언트는 World 수명에 종속되지 않는 Core ticker로 GameSession과 NFPartySession의 실제 제거를 확인한다. 제거 진행 중에는 중복 DestroySession을 보내지 않는다.
- 10초 간격으로 최대 3회 시도/관찰한다. 콜백이 누락되어도 세션이 사라지면 성공이다. 실패하거나 Destroying 상태가 계속되면 약 30초 후 정리 실패로 처리한다.
- `OnMatchCleanupFinished(bool)`로 성공/실패를 전달한다. 실패는 승리 결과를 변경하지 않으며, 완료되지 않은 상태에서 새 매칭을 시작하지 않는다.
- 메뉴 UI에서 실패 시 `RetryCompletedMatchCleanup()`을 제공할 수 있다. OSS가 계속 멈춰 있으면 재시도도 실패할 수 있으며 재접속/앱 재시작이 필요하다. 로컬 세션 레코드만 강제로 지워 원격 정리를 성공으로 간주하지 않는다.
- 서버는 세션 제거 완료 후 3초 유예하여 프로세스를 종료한다. 제거 실패 또는 10초 콜백 타임아웃도 같은 종료 정책을 적용한다. 실제 Dedicated Server만 종료하며 Editor/Listen Server는 프로세스를 유지한다. 서버 재사용은 구현하지 않는다.
- GameMode와 Subsystem 종료 시 자신이 소유한 정상 종료 바인딩만 해제한다. 타이머와 ticker도 수명 종료 시 제거한다.

## 검증 상태

소스 정적 검토와 `git diff --check`를 수행했다. 빌드 권한 요청이 거절되어 UHT/컴파일 및 실제 네트워크 실행은 검증하지 못했다.

실행 검증 항목:

1. 실제 전투 GameMode에서 정상 종료 핸들러가 연결되는지 확인한다.
2. 승리/패배 팀 모두 같은 WinningTeamId와 EscapeSucceeded 결과를 받는지 확인한다.
3. Core 결과 표시 시간 이후 기본 맵으로 복귀하고 실패 알림이 나타나지 않는지 확인한다.
4. 중복 결과 전달 시 복귀와 종료가 반복되지 않는지 확인한다.
5. 세션 제거 즉시 거절, 실패 콜백, 콜백 누락 및 Destroying 고착을 각각 시험한다.
6. 정리 성공 뒤 새 매칭이 가능하고, 실패 중에는 차단되며 명시적 재시도로 회복되는지 확인한다.
7. 패키징된 Dedicated Server에서 결과 전송 후 세션 정리 및 프로세스 종료를 확인한다.
