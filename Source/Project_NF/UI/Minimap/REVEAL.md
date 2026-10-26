# 공통 미니맵 공개 목록과 천리안

작성: 이정준 / Codex · 2026-10-15 · UE 5.7 / Dedicated Server / GAS

## 구조

새 ActorComponent는 없다. `NF_MinimapRevealRegistry` UObject를 `NF_PlayerController`가 소유한다.

```text
서버 천리안 GA ──────┐
                    ├→ 수신자의 Registry → 해당 PlayerController의 Client RPC
서버 열쇠 소유권 로직 ┘                              ↓
                                          클라이언트 Registry
                                               ↓
                                    ViewModel → Snapshot → MinimapWidget.SetSnapshot
```

각 플레이어의 목록은 별개다. RegisterReveal은 서버에서만 동작하며 클라이언트의 공개 요청용 Server RPC는 없다. Registry UObject 자체와 대상 액터를 복제하지 않는다. 서버가 대상 좌표를 읽고 ID/양자화 좌표/표시 종류의 전체 스냅샷을 소유 클라이언트에 0.2초 간격으로 전송한다. 대상이 없어지면 빈 스냅샷을 한 번 전송하고 빈 목록은 계속 전송하지 않는다.

클라이언트 스냅샷에는 1초의 수신 유효시간이 있어 갱신이 끊기면 아이콘을 숨긴다. 시간은 로컬 수신 기준이고 네트워크 지연이 없는 실시간 위치를 보장하지 않는다. 현재는 소규모(8인)용 Reliable 전체 스냅샷이며 대규모 확장 시 델타/갱신 빈도 최적화를 별도로 검토한다.

## 에디터 설정: 천리안

1. 실제 사용하는 **PlayerController가 `NF_PlayerController` 또는 그 자식인지 확인**한다. 기본 TPS PlayerController를 그대로 쓰면 이 공개 RPC 경로가 없으므로 천리안 발동을 거부한다. 프로젝트 PlayerController의 기존 입력 데이터/캐릭터 설정은 해당 담당 설정을 유지한다.
2. **`NFGA_Clairvoyance` 부모**로 스킬 BP를 만든다. 공통 SkillBase 자체의 자식으로 만들지 않는다.
3. Execution Actor Class / Preview Actor Class는 None, Requires Confirmed Target Data는 끔으로 둔다. 별도 실행 BP/컴포넌트/GE를 추가할 필요 없다.
4. Skill Execution Mode Timed, Execution Duration 기본 5초, Detection Radius 기본 2000cm, Detection Interval 기본 0.2초. Cast Time 기본 0초. 발동 비용과 쿨다운은 기존 GA의 Cost/Cooldown GE에 설정한다.
5. Net Execution Policy Server Only, Instancing Policy Instanced Per Actor를 유지한다. 입력은 기본 누를 때 발동하고 키를 떼어도 지속된다.
6. 팀 정보가 아직 없어 Target Filter의 Team은 All이다. 자신은 제외하고 플레이어 Pawn만 탐지한다. 몬스터·SpectatorPawn은 제외한다. 벽 너머도 탐지한다. Require Ability System을 켜면 ASC 없는 기본 TPS Pawn은 제외된다.
7. 미니맵 WBP에서 **ViewModel.Snapshot → MinimapWidget.SetSnapshot** 단방향 바인딩을 연결한다. 기존 자동 조회 옵션은 더 이상 사용하지 않는다.
8. 미니맵 Owning Player는 실제 사용자 Controller다. 정식 Ability 부여/입력 경로 또는 기존 NF_SkillTest를 통해 GA를 부여한다. 기존 ASC가 있으면 테스트 ASC를 중복 추가하지 않는다.

천리안은 서버 시전자 위치를 중심으로 구 범위를 재탐색한다. 대상은 Pawn Object Type의 충돌체가 필요하다. 범위에 들어오면 등록하고 나가면 다음 탐색 때 해제한다. UI 반영에는 탐색 및 전송 주기에 따른 지연이 있다. 공개 범위와 미니맵 표시 범위는 별개다. 예를 들어 탐지 반경 2000cm와 Visible World Size 4000cm는 각각 구 반경과 정사각형 한 변의 길이다.

자식 GA가 끝나거나 취소되면 타이머와 해당 실행 출처만 정리한다. 시전자 소멸/교체도 다음 검사 때 종료한다. 사망·기절은 팀 태그가 정해지면 기존 Input Cancel Tags / Activation Blocked Tags 및 Target Filter Blocked Tags에 연결한다. 살아 있는 Actor에 사망 태그만 생긴 것을 별도 설정 없이 자동 판별하지는 않는다.

## 열쇠 담당이 사용할 서버 API

수신자 `NF_PlayerController → Get Minimap Reveal Registry`를 통해 다음을 호출한다.

| API | 입력/역할 |
|---|---|
| RegisterReveal | SourceId, Target, Kind=EscapeKey, Lifetime=0 |
| RemoveReveal | SourceId, Target 한 건 해제 |
| RemoveRevealSource | 해당 열쇠 공개 출처 전체 해제 |

SourceId는 `New Guid`로 한 번 만들고 공개가 유지되는 동안 저장한다. 매 프레임 새 GUID를 만들지 않는다. RegisterReveal은 같은 출처/대상을 중복 등록하지 않고 정보를 갱신한다. Lifetime 0은 자동 만료 없음이므로 소유권 로직에서 명시적으로 해제해야 한다.

열쇠를 모든 플레이어에게 공개한다면 **서버에서 각 수신자 Controller의 Registry에 등록**한다. 한 Registry 등록이 전체 방송되는 것은 아니다. 드롭/이전 시 이전 공개를 제거하고 새 소유자를 등록한다. 소유권 변경 처리 자체와 뒤늦은 접속자에 대한 등록은 열쇠 시스템이 호출해야 하며 이번 작업에서 자동 연결하지 않았다. 바닥 열쇠 공개 여부는 별도 기획 정책이다.

같은 대상이 천리안과 열쇠로 공개되면 아이콘은 하나이며 EscapeKey가 우선한다. 열쇠 공개를 해제해도 천리안 출처가 남아 있으면 탐지 아이콘으로 돌아간다. 천리안이 끝나도 열쇠 출처가 남아 있으면 열쇠 표시는 유지한다. 열쇠 종류는 기존 미니맵 Objective 색, 천리안은 Enemy 색을 사용한다. 아이콘 텍스처 디자인은 별도다.

## ViewModel 연결 (2026-10-26)

ViewModel이 클라이언트 Registry의 `GetVisibleMarkers()`를 읽고, 팀원 목록과 중복을 정리한 전체 Snapshot을 전달한다. 위젯은 Registry나 Pawn을 직접 조회하지 않는다. 효과 종료는 다음 Snapshot에서 해당 마커가 제외되는 방식으로 반영한다. 위젯 재생성 시 현재 Snapshot을 다시 전달한다. 기존 UpsertMarker 등의 외부 공급도 VM으로 통합해야 한다. 구체적인 수신·표시·호환 규칙은 [위젯 가이드](README.md)를 참조한다.

## 검증 상태

출처 중복 합산·열쇠 우선순위·해제 후 다른 출처 유지·범위 집합 교체·만료·대상 소멸의 자동화 테스트 `NF.UI.Minimap.RevealRegistry`를 추가했다. 이번 작업에서는 빌드/테스트 실행/멀티플레이 PIE 검증을 하지 않았다. BP 에셋도 생성하지 않았다.
