# 단방향 포탈 스킬

작성: 이정준 / Codex · 2026-10-15 · UE 5.7 / GAS / Dedicated Server

## 구성

- `NFGA_Portal`: 기존 SkillBase를 상속한 전용 GA. 입력/지면 TargetData/시전 대기/비용·쿨다운/생성 실패 정리 담당.
- `NF_Portal`: 입구와 출구에 같은 클래스를 사용한다. 서버 Overlap·출구 공간 검사·텔레포트·짝 종료 담당.
- 기존 `NF_DeployableComponent`: 유지 시간과 종료/제거 담당. 포탈에 기본 부착되어 있다.
- 기존 지면 프리뷰: `NF_SkillPreviewActor + GroundTargeting + AreaIndicator` 재사용.

새 범용 컴포넌트는 추가하지 않았다. 특정 PlayerController 클래스에 의존하지 않아 기본 TPS Character도 사용할 수 있다. 포탈 이용자는 ACharacter 기반 플레이어이며 이동은 서버에서만 처리한다. 몬스터는 이용 대상에서 제외한다.

## BP 설정

1. `NF_Portal` 부모로 `BP_NF_Portal`을 만든다. 기본 Root / EntranceTrigger / PortalMesh / Deployment를 사용하며 Deployable·충돌체를 중복 추가하지 않는다.
2. `PortalMesh`의 모델/크기/위치를 원하는 외형으로 바꾼다. 기본값은 테스트용 얇은 큐브다. 실제 포탈 이펙트·아이콘은 포함하지 않았다. 메시 자체 충돌은 끈다.
3. `EntranceTrigger`는 Pawn Overlap과 Generate Overlap Events를 유지한다. X축은 통과 방향, 루트는 지면이다. 트리거 높이는 Construction에서 Box Z 반크기에 맞춰 자동 배치한다.
4. `Deployment`의 Lifetime을 설정한다(기존 기본 15초, 예: 8초로 변경). Destroy Owner On End / Disable Owner Collision On End는 켜 두는 것을 권장한다. 종료 시 표시를 숨기고 양쪽을 정리한다. Lifetime 0은 시간 제한 없음이다.
5. Entrance Material / Exit Material로 입출구를 구분할 수 있다. 기본은 같은 메시다. Portal State Changed BP 이벤트는 선택적 연출 훅이며 이동을 위해 연결할 필요 없다.
6. **`NFGA_Portal` 부모**로 `NFGA_Portal_Test`를 만든다. Portal Class에 `BP_NF_Portal`을 지정한다. Execution Actor Class는 None이다.
7. Active / Movement / Input / Instant, Server Only, Instanced Per Actor를 유지한다. Activate On Input Release 및 Requires Confirmed Target Data는 기본 켜짐이다. Cost / Cooldown GE는 필요한 것을 지정한다.
8. 홀딩 표시가 필요하면 `NF_SkillPreviewActor` 자식 BP에 GroundTargeting + AreaIndicator를 추가하고 Automatic Composition을 켠 뒤 GA Preview Actor Class에 지정한다. 프리뷰가 없어도 목표 계산과 서버 검증은 수행한다.
9. 정식 Ability 부여 경로 또는 기존 NF_SkillTest로 GA를 부여한다. 기존 ASC가 있으면 테스트 ASC를 중복 추가하지 않는다.

GA의 Portal Range(기본 1500cm), Ground Search Depth(3000cm), Entrance Forward Distance(180cm), Minimum Separation(350cm), Preview Radius(100cm)는 디테일에서 설정할 수 있다. 포탈은 전용 프리뷰 데이터 함수를 제공하므로 기본 PreviewData의 편집 제한과 관계없이 이 값을 사용한다.

## 동작

```text
E 홀딩 → 시점 방향의 지면에 출구 예상 장판
E 해제 → 클라이언트 지면 목표 전달 → 서버 거리·바닥·가림 검증
       → 시전자 앞 입구 바닥 찾기 → 입출구 간격·체적·도착 캡슐 검사
       → GAS Commit → 출구/입구 활성화 → GA 종료
입구 접촉 → 서버가 출구 앞의 바닥·공간 재검사 → 이동
유지 시간 만료 또는 한쪽 제거 → 양쪽 비활성화·제거
```

출구는 표시용이며 반대 방향으로 들어갈 수 없다. 포탈은 생성 후 시전자를 따라가지 않는다. 생성에 성공한 쌍은 GA 종료/시전자 소멸 이후에도 자체 설치 수명으로 유지된다. Only Caster Can Use가 켜져 있고 원래 시전자 액터가 없어지면 다른 플레이어는 이용할 수 없다.

유효하지 않은 위치는 Commit 전에 거부한다. Commit 이후 커스텀 BP가 포탈을 제거하거나 시작을 중단한 경우까지 비용 환불을 제공하지는 않는다. 준비 중 취소되면 미완성 포탈을 제거한다.

## 이동 설정

| 포탈 변수 | 의미 |
|---|---|
| Exit Forward Offset | 출구 원점에서 앞으로 이동해 도착 바닥을 찾는 거리(기본 160cm) |
| Arrival Search Height / Depth | 도착 바닥을 위/아래로 검사하는 범위 |
| Ground Clearance | 바닥에서 캡슐을 띄우는 여유(기본 5cm) |
| Reentry Cooldown | 같은 입구를 연속 이용하는 간격(기본 0.75초) |
| Only Caster Can Use | 끄면 모든 플레이어가 사용. 현재 팀 검사 없음 |
| Align View To Exit | 켜면 출구의 수평 방향으로 캐릭터·시점 회전. 시점 Pitch는 유지 |
| Preserve Velocity | 기본 끔: 이동 직후 기존 속도 정지. 켜면 기존 월드 속도를 유지 |

도착 위치는 실제 이용 캐릭터의 캡슐 크기·충돌 Profile로 검사한다. 걸을 수 있는 바닥이 없거나, 바닥 높이가 출구 원점에서 캐릭터 Max Step Height보다 크게 달라지거나, 머리/몸 공간이 막히면 이동하지 않는다. 출구 앞을 다른 플레이어가 막으면 해당 충돌 Profile이 서로 Block하는 경우 이동을 보류한다. 입구에 계속 서 있으면 0.1초마다 다시 시도한다. 이용자의 캡슐도 Generate Overlap Events를 켜야 한다.

기본 수직 중력/직립 Character를 전제로 한다. 층 선택, 벽 부착 포탈, 양방향 이동, 팀 전용 허용, 다른 포탈 쌍 사이의 전역 이용 쿨다운은 구현 범위에 포함하지 않는다. 홀딩 장판은 지면 목표만 표시하며 입출구 체적/도착 공간까지 미리 검증해 색을 바꾸지는 않는다. 최종 설치 가능 여부는 서버에서 결정한다.

## 실패 확인

Output Log의 `[Portal] Placement failed`를 확인한다. 지면이 Visibility 채널을 Block해야 기존 지면 목표 검증을 통과한다. 위치가 너무 가깝거나, 포탈 Box 크기만큼 빈 공간이 없거나, 출구 바로 앞이 낭떠러지/계단/벽이면 실패할 수 있다. 기본 메시를 바꿔도 충돌 판정은 EntranceTrigger와 이용자 캡슐을 기준으로 한다.

이번 작업은 C++ 구현 및 자동화 테스트 `NF.Skills.Portal.PlacementAndLifetime` 작성까지다. 빌드·테스트 실행·PIE 네트워크 이동 확인은 수행하지 않았다. BP 에셋은 생성하지 않았다.
