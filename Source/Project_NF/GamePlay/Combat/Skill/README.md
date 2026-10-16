# 스킬 컴포넌트 조합 튜토리얼

전용 스킬 가이드: [단방향 포탈](Portal/README.md), [천리안·공통 미니맵 공개](../../../UI/Minimap/REVEAL.md)

작성: 스킬 담당 / Codex · 기준일: 2026-10-14 · Unreal Engine 5.7 · GAS · Dedicated Server

이 문서는 현재 코드에서 자동 연결되는 조합을 기준으로 작성했다. 예제 BP/GE 이름과 수치는 제작 예시이며, 해당 에셋이 이미 만들어져 있다는 뜻은 아니다.

## 1. 먼저 알아둘 구조

스킬은 **GA가 발동을 관리하고, 실행 액터에 붙인 컴포넌트가 기능을 수행하는 구조**다.

| 구성 | 부모 클래스 | 담당하는 일 |
|---|---|---|
| 스킬 GA BP | `NFGA_SkillBase` | 입력, 목표 확정, 비용·쿨다운 Commit, 시전 대기, 실행 액터 생성·종료 |
| 실행 액터 BP | `NF_SkillExecutionActor` | 부착된 컴포넌트를 찾아 탐색·적용·반복·발사·설치를 자동 연결 |
| 프리뷰 액터 BP | `NF_SkillPreviewActor` | 홀딩 중 로컬 플레이어에게 목표 장판·예상 포물선 표시 |
| 효과 GE BP | `GameplayEffect` | 피해·회복·스탯 변경·태그·도트·스택의 실제 효과 |

GA에는 ActorComponent를 붙이지 않는다. 실행 기능은 실행 액터 BP에, 조준 표시 기능은 프리뷰 액터 BP에 붙인다. 프리뷰가 지면을 찾는 것과 실제 액터가 지면에 설치되는 것은 각각 설정해야 한다.

일반 흐름:

```text
입력 → (선택: 로컬 홀딩 프리뷰) → 목표 확정·서버 검증
     → 시전 시간 대기 → 실행 액터 준비 → GAS Commit → 실행 시작
     → 피해/효과 적용 또는 설치/발사 → 설정된 조건으로 종료
```

비용·쿨다운은 시전 대기가 끝나고 실행 준비에 성공했을 때 Commit된다.

공통 실행 액터의 지원 조합은 이벤트 그래프 연결 없이 동작한다. 일반 `Actor`에 같은 컴포넌트를 붙이면 자동 실행되지 않으며, 직접 초기화·호출하는 전용 로직이 필요하다. 서버가 판정과 GE 적용을 맡고 클라이언트는 프리뷰·복제된 빔·폭발 등의 연출을 표시한다.

## 2. 컴포넌트 역할표

| 컴포넌트 | 주로 붙이는 곳 | 역할 | 직접 하지 않는 일 |
|---|---|---|---|
| `NF_GroundTargetingComponent` | 실행/프리뷰 액터 | 시점 방향과 사거리로 지면 목표를 찾음 | 장판 표시, 방벽 체적의 설치 가능 검사 |
| `NF_BeamTargetingComponent` | 실행 액터 | 빔 시작·끝 계산, 빔 메시 표시 | 피해 적용, 반복 타이밍 |
| `NF_ProjectileLaunchComponent` | 실행 액터 | 투척 속도·각도·발사 위치 계산, 투사체 이동 준비·시작 | 폭발, 피해 |
| `NF_DetonationComponent` | 실행 액터 | 시간/충돌 조건을 한 번의 폭발 요청으로 변환, 폭발 연출 | 대상 탐색, GE 적용 |
| `NF_TargetQueryComponent` | 실행 액터 | 범위·팀·태그·가림 조건으로 대상 목록 반환 | GE 적용, 실루엣·미니맵 표시, 대상 목록 복제 |
| `NF_ApplyEffectComponent` | 실행 액터 | 자신·단일 대상·목록에 GE 적용, 소유한 지속 효과 정리 | 충돌 판정, 이동속도와 CharacterMovement 연결 |
| `NF_HitEffectComponent` | 실행 액터 | 충돌/오버랩/외부 명중 결과를 ApplyEffect로 전달 | 충돌체 생성, 범위 폭발 피해 |
| `NF_AreaDamageComponent` | 실행 액터 | 범위 탐색과 피해 Spec 제출을 제공하는 기존 경로 | 폭발·반복 타이밍 |
| `NF_RepeatApplicationComponent` | 실행 액터 | 일정 간격으로 적용 요청 발생 | 대상 판정, 피해 수치 |
| `NF_DeployableComponent` | 실행 액터 | 설치물 수명·종료·제거 관리 | 체력, 총알 차단 충돌 |
| `NF_DeployableHealthComponent` | 실행 액터 | 자체 ASC에 설치물 체력 Attribute 등록, 체력 소진을 설치 종료에 연결 | 무기 명중을 피해 GE로 변환 |
| `NF_AreaIndicatorComponent` | 프리뷰/실행 액터 | 원형 장판 표시 | 피해 범위 판정 |
| `NF_ProjectileTrajectoryComponent` | 프리뷰 액터 | 첫 충돌까지의 예상 포물선과 충돌 지점 계산·표시 | 실제 투사체 생성·이동 |
| `NF_SkillTestComponent` | 테스트 Pawn | 임시 ASC와 E 입력, Test Skill Class 부여·발동 | 정식 장착·입력 시스템 |

UE 기본 컴포넌트도 함께 사용한다. `Sphere Collision`은 투사체 충돌체, `Projectile Movement`는 투사체 이동, `Box Collision`은 방벽 충돌체, `Static Mesh`는 외형, `Ability System Component`는 설치물 자체 GE 수신에 사용한다.

## 3. 처음 스킬 만들기

1. `NF_SkillExecutionActor` 부모로 실행 액터 BP를 만든다.
2. 원하는 기능의 컴포넌트를 추가하고 디테일을 설정한다.
3. `NFGA_SkillBase` 부모로 GA BP를 만든다.
4. GA의 `Execution Actor Class`에 실행 액터 BP를 지정한다.
5. 필요하면 프리뷰 BP를 만들어 `Preview Actor Class`에 지정한다.
6. 테스트 Pawn의 `NF_SkillTest`에 GA BP를 지정하고 PIE에서 E로 확인한다.

에셋 이름 예시: `NFGA_Grenade_Composed`, `BP_NF_GrenadeExecution`, `BP_NF_GrenadePreview`, `NFGE_Damage`. C++ 클래스 이름의 `A`/`U` 접두사는 에디터 클래스 검색에서 생략되어 보일 수 있다.

### GA 공통 설정

| 항목 | 설정 기준 |
|---|---|
| Skill Type | 액티브는 `Active`, 장착 패시브는 `Passive` |
| Skill Category | Attack / Utility / Movement / Search 분류 정보 |
| Skill Activation Method | 일반 테스트 입력은 `Input`; 분류값만 바꿔 모든 발동 연결이 생기지는 않음 |
| Skill Execution Mode: Instant | 실행 시작 후 GA 종료. 설치물·투사체는 자체 수명으로 유지 |
| Skill Execution Mode: Timed | Execution Duration 이후 GA와 실행 액터 종료 |
| Skill Execution Mode: Continuous | 현재 테스트 입력에서는 키 해제 시 종료 |
| Cast Time | 자동 실행 전에 기다리는 준비 시간 |
| Activate On Input Release | 켜면 홀딩 후 키를 뗄 때 발동. 끄면 누를 때 발동 |
| Requires Confirmed Target Data | 지면 목표를 확정해 설치할 때 켬. 조합 투사체의 방향 확정은 별도로 자동 처리 |
| Build Cost | 장착 예산 정보. 발동 자원 소모와는 별도 |
| Cost / Cooldown Gameplay Effect Class | 실제 사용 비용·쿨다운 GE |
| Net Execution Policy | 자동 실행 조합은 `Server Only` 사용 |
| Instancing Policy | 기본 `Instanced Per Actor` 유지 |

`Timed`는 Execution Duration을 0보다 크게 설정한다. `Instant`에 무제한 반복만 넣을 수는 없다. Repeat의 Duration/Max Applications로 유한 수명을 주거나 Deployable로 수명을 관리한다. 일반 Instant 실행 액터는 별도 설치·반복·폭발·명중 수명이 없으면 약 0.5초 후 제거된다.

### 프리뷰 설정과 현재 제한

프리뷰 BP에서 `Automatic Composition`을 켠다. `GroundTargeting` 또는 `ProjectileTrajectory` 중 하나와 `AreaIndicator`를 추가하면 공통 코드가 매 프레임 연결한다. 기존 수동 UpdatePreview 그래프는 자동 모드에서 호출되지 않는다.

| Preview Data 값 | 의미 |
|---|---|
| Display Radius | 표시할 원의 반지름. 실제 판정 크기와 따로 확인 |
| Target Range | 지면 목표의 최대 거리 |
| Ground Search Depth | 지면을 아래로 찾는 탐색 깊이 |

**현재 `FNF_SkillPreviewData` 멤버에는 EditAnywhere가 없어 GA의 Default Preview Data 내부 값이 디테일에 표시되지 않는다.** 검색 필터를 지워도 해결되지 않는다. 기본 GA는 코드에서 Display Radius 300cm, Target Range 4000cm, Ground Search Depth 5000cm를 사용한다. 지면 스킬마다 디테일에서 값을 설정하려면 해당 선언 보완이 필요하다. 임시 BP 대안은 GA의 `Get Skill Preview Data`를 Override하고 Make NF Skill Preview Data를 반환하며 Return Value를 true로 설정하는 것이다. 이 경우만 값 제공용 그래프가 필요하다.

조합 투사체의 프리뷰 값은 실제 Launch/ProjectileMovement/Sphere/Detonation 설정에서 자동 계산한다. 해당 데이터 함수를 임의로 덮어쓰면 실제 발사와 예측이 달라질 수 있다.

## 4. 탐색과 효과 설정

### TargetQuery: 누구를 대상으로 할지

| 설정 | 의미 |
|---|---|
| Shape | Sphere / Box / Capsule / Sector |
| Radius | 구·캡슐 반지름 또는 부채꼴 전방 거리 |
| Box Half Extent | 박스의 절반 크기. 전체 크기 600×200×200cm라면 300,100,100 |
| Capsule Half Height | 반구를 포함한 캡슐의 로컬 Z축 절반 높이. Radius 이상 |
| Sector Half Angle / Half Height | 로컬 X축 전방 기준 절반 각도 / 절반 높이 |
| Object Channels | 대상 충돌체의 Object Type. Pawn / WorldDynamic 등을 선택 |
| Include Source | 시전자도 포함할지 |
| Require Ability System | ASC가 있는 대상만 찾을지. GE 적용용은 켜는 것을 권장 |
| Team | All / Allies / Enemies |
| Required / Blocked Tags | 필요한 태그 전부 보유 / 차단 태그 하나라도 있으면 제외 |
| Require Line Of Sight / Trace Channel | 범위 중심에서 대상까지 가림 검사 |
| Sort By Distance | 가까운 순으로 정렬 |
| Max Targets | 0은 무제한, 1은 한 대상. 거리 정렬과 함께 쓰면 가장 가까운 대상 |

Team은 Actor 또는 Pawn Controller의 GenericTeamAgentInterface 팀 정보를 사용한다. 팀 정보가 없으면 Allies/Enemies 필터에서 제외된다. 아직 팀 시스템이 연결되지 않았다면 All로 테스트한다. 범위 탐색은 충돌 Overlap이며 렌더링 레이 트레이싱이 아니다. Sector는 충돌 컴포넌트 Bounds 중심으로 각도·거리·높이를 검사한다.

### ApplyEffect: 무엇을 적용할지

| Target Mode | 자동 조합 |
|---|---|
| Self | 시전자에게 적용. Query/Area/Hit/Launch와 함께 사용하지 않음 |
| Query | TargetQuery 또는 AreaDamage가 찾은 대상에 적용 |
| Hit | HitEffect가 전달한 명중 대상에 적용 |

Effects 배열에 GE와 필요한 SetByCaller 태그/값을 설정한다. 한 번에 여러 GE를 적용할 수 있다. SetByCaller 태그는 GE Modifier의 Data Tag와 정확히 같아야 한다. 고정 Scalable Float GE에는 SetByCaller 설정이 필요 없다.

| Removal | 효과 수명 |
|---|---|
| Independent | 실행 액터가 종료되어도 효과 유지. GE 자체 지속시간/제거 규칙 사용 |
| OnExecutionEnd | 컴포넌트가 적용한 지속/무한 GE를 실행 종료·EndPlay 때 제거 |

OnExecutionEnd는 Instant GE와 공유 스택 GE에는 사용할 수 없다. 지속시간이 있고 No Stacking인 효과만 사용한다. 같은 실행에서 같은 대상 ASC에 동일 항목을 재적용해도 소유한 지속 효과를 중복 생성하지 않는다. Independent는 반복 시 새 효과가 적용될 수 있으므로 GE의 스택·재사용 제한을 설계한다.

도트와 출혈 스택 자체는 GE에서 설정한다. 반복 컴포넌트로 같은 도트 GE를 계속 적용하면 의도치 않게 효과가 여러 개 생성되거나 지속시간이 갱신될 수 있다. **GE Period**와 **Repeat Interval**은 각각 효과 내부 주기와 스킬의 새 적용 주기다.

### 기존 AreaDamage를 유지하는 방법

기존 조합은 그대로 사용한다. 실행 액터의 Damage Effect Class / Damage Magnitude Tag / Damage Magnitude를 설정하고 AreaDamage를 붙인다. 내부 탐색·GE 제출은 공통 코드로 연결되어 있다.

새 조합에서는 AreaDamage를 `TargetQuery + ApplyEffect(Query)`로 교체하고 효과 설정을 ApplyEffect로 옮긴다. TargetQuery와 AreaDamage를 동시에 붙이면 조합 검증에 실패한다. `AreaDamage + ApplyEffect(Query)`도 가능하며 이 경우 효과는 ApplyEffect에서 설정한다.

## 5. 예제: 조합 수류탄

### 실행 BP

`NF_SkillExecutionActor` 부모로 `BP_NF_GrenadeExecution`을 만들고 다음을 추가한다.

- Sphere Collision **1개**
- Projectile Movement **1개**
- NF Projectile Launch
- NF Detonation
- NF Target Query
- NF Apply Effect
- Static Mesh: 수류탄 외형, 자체 충돌은 끔

| 위치 | 예시 설정 |
|---|---|
| 액터 | Follow Source 끔 |
| Sphere | Radius 10cm |
| Projectile Launch | Throw Speed 1200cm/s, Throw Angle Offset 15°, Configure Collision 켬 |
| Projectile Movement | Projectile Gravity Scale 1, Max Speed 0 또는 발사 속도 이상 |
| Detonation | Mode Fuse, Fuse Time 3초 |
| TargetQuery | Sphere, Radius 300cm, Require Ability System 켬 |
| ApplyEffect | Target Mode Query, Effects에 피해 GE, Removal Independent |

Launch가 Sphere를 이동 충돌 루트로 구성한다. 메시를 Sphere 아래에 배치해 루트 변경 후에도 외형이 따라가게 한다. Simulate Physics는 끄고 ProjectileMovement로 이동한다. Fuse는 시간 폭발, Impact는 충돌 폭발, FuseOrImpact는 먼저 도달한 조건으로 폭발한다. Impact도 Max Lifetime 만료 시 폭발한다.

### 프리뷰 BP / GA

프리뷰 BP는 `NF_SkillPreviewActor` 부모로 만들고 `ProjectileTrajectory + AreaIndicator`를 추가한다. Decal Material이 없으면 디버그 원으로 표시한다. 포물선은 첫 충돌까지의 예측이며 튕김 이후의 최종 폭발 위치까지 예측하지 않는다.

GA에는 Execution Actor Class / Preview Actor Class를 연결하고 Active / Attack / Input / Instant로 설정한다. Activate On Input Release를 켠다. 방향 목표는 조합 투사체에서 자동으로 확정한다.

흐름: **E 홀딩 → 궤적·충돌 예상 지점 표시 → 해제 → 서버 투척 → 3초 후 탐색·피해 GE 적용 → 제거**.

## 6. 예제: 플레이어가 쏘는 레이저

실행 BP에 `BeamTargeting + TargetQuery + ApplyEffect + RepeatApplication`을 추가한다.

| 위치 | 예시 설정 |
|---|---|
| BeamTargeting | Range 2000cm, Half Width 20cm, Visual Material 지정 |
| TargetQuery | Require Ability System 켬, 팀/태그/가림 필터 설정 |
| ApplyEffect | Query, 피해 GE, Independent |
| RepeatApplication | Interval 0.2초, Apply Immediately 켬 |
| GA | Active / Attack / Input, Activate On Input Release 끔 |

빔 공격 시 실행 액터가 탐색 모양을 **빔 길이에 맞는 Box**로 자동 변환한다. 폭은 Beam의 Half Width를 사용하므로 Query의 Shape를 별도로 빔 길이에 맞출 필요가 없다. 빔 표시는 기본 큐브 메시를 늘리며 교체 메시도 로컬 X축 방향·100cm 기준이다.

- **누르면 3초 발사:** GA Timed, Execution Duration 3초. Repeat Duration/Max Applications는 0으로 두고 GA가 종료 관리.
- **누르는 동안 발사:** GA Continuous. Repeat Duration/Max Applications는 0으로 두고 입력 해제로 종료.

BeamTargeting이 매 프레임 시전자 시점을 다시 계산하므로 발사 중에도 조준이 따라간다. Follow Source는 빔 계산에 필수가 아니다. 일반 Query만 붙인 자기 주변 영역을 따라 움직이게 할 때 사용하는 옵션이다.

관통 여부는 충돌 채널로 결정한다. Trace Channel을 적이 Block하면 첫 적에서 빔이 멈춘다. 적을 관통하려면 벽만 Block하는 빔 전용 채널을 연결하고 Query의 가림 정책도 그 의도에 맞춘다.

## 7. 예제: 방벽 설치

실행 BP에 `GroundTargeting + Deployable + DeployableHealth + AbilitySystemComponent + Box Collision + Static Mesh`를 추가한다.

| 위치 | 예시 설정 |
|---|---|
| 액터 | Follow Source 끔 |
| Deployable | Lifetime 15초, Destroy Owner On End 켬, Disable Owner Collision On End 켬 |
| DeployableHealth | Initial Max Health 300, End Deployment On Depleted 켬 |
| ASC | Component Replicates 켬. 방벽 액터 자체 소유 ASC 하나 |
| Box | Query and Physics, 팀 총알 판정 채널 Block. 필요에 따라 Pawn Block |
| 메시 | 자체 충돌 끔. 메시·Box 상대 Z를 벽 높이 절반만큼 올림(중앙 원점 메시 기준) |
| GA | Active / Utility / Input / Instant, Activate On Input Release 및 Requires Confirmed Target Data 켬 |

프리뷰 BP에는 `GroundTargeting + AreaIndicator`를 붙이고 GA에 연결한다. 실제 실행 BP에도 GroundTargeting을 붙인다. 벽 액터 원점은 지면이며 외형·충돌체의 상대 위치로 벽 바닥을 맞춘다. 현재 프리뷰는 원형이므로 방벽의 직사각형 외형과는 다르다.

흐름: **홀딩 조준 → 해제 → 지면에 설치 → 15초 경과 또는 체력 0 → 충돌 해제·제거**. Instant는 설치 동작 종료를 뜻하며 벽의 수명은 Deployable이 관리한다. Lifetime 0은 시간 제한 없음이다.

**총알 충돌만으로 체력이 깎이지 않는다.** 무기 시스템이 서버에서 방벽 자체 ASC에 피해 GE를 적용해야 한다. 현재 설치물 Attribute는 아래와 같다.

| Attribute | 용도 |
|---|---|
| NF_DeployableHealthAttribute.Health | 설치물 현재 체력 |
| NF_DeployableHealthAttribute.MaxHealth | 설치물 최대 체력 |
| NF_DeployableHealthAttribute.IncomingDamage | 양수 피해 전달. 적용 시 Health 차감 후 0으로 초기화 |

방벽용 피해 GE는 예를 들어 Instant + IncomingDamage + Add (Base)로 구성한다. 고정 피해 또는 팀 피해 태그와 연결한 SetByCaller를 사용할 수 있다. 이 Attribute는 플레이어 체력용이 아니다. Health는 MaxHealth를 초과하지 않으며 MaxHealth만 증가시켜도 현재 Health가 자동으로 증가하지는 않는다.

현재 지면 탐색은 바닥·거리·가림을 검사한다. 방벽 전체 크기가 주변 벽과 겹치는지, 설치물 개수 제한 등은 별도 배치 검증 기능이 필요하다.

## 8. 예제: 자신에게 가속화·버프

실행 BP에 `ApplyEffect`만 추가하고 Target Mode Self로 설정한다. GA는 Instant, Activate On Input Release 끔, Requires Confirmed Target Data 끔, Preview Actor Class None으로 설정한다.

가속화 GE 예시:

| 항목 | 값 |
|---|---|
| Duration Policy | Has Duration |
| Duration Magnitude | Scalable Float 5초 |
| Period | 0 |
| Modifier Attribute | 플레이어의 MoveSpeed Attribute |
| Modifier Op / Magnitude | Multiply (Compound) / 1.3 |
| ApplyEffect Removal | Independent |

**현재 이 작업 범위에는 플레이어 MoveSpeed Attribute가 구현되어 있지 않다.** 목록에 없으면 선택할 수 없으며 Attribute 추가·캐릭터 ASC 등록·CharacterMovement.MaxWalkSpeed 반영 연결이 선행되어야 한다. GE만 추가한다고 캐릭터가 빨라지지 않는다. 이동속도 변화·GE 만료가 실제 이동에 반영되도록 플레이어 담당과 GAS 연결을 맞춘다.

플레이어 체력·공격력·방어력 증가도 해당 Attribute와 게임 로직 연결이 먼저 있어야 한다. 상시 장착 패시브는 GA의 Passive Effects에 지속/무한 자기 GE를 지정하는 기존 경로를 사용할 수 있다.

## 9. 예제: 명중형 투사체 / 지속 피해 장판

### 명중형 투사체

`Sphere + ProjectileMovement + ProjectileLaunch + HitEffect + ApplyEffect(Hit)`를 조합한다. ApplyEffect에 화상·출혈 등의 GE를 넣는다. 폭발이 필요 없으면 Detonation은 생략할 수 있으며 명중 전용 투사체의 기본 수명 제한은 10초다.

HitEffect의 Once Per Target은 켜면 동일 ASC에 한 번만 적용한다. 끄면 Reapply Interval마다 재적용을 허용한다. Listen To Overlaps를 켜면 BeginOverlap도 감지하되 충돌체의 Overlap 설정·Generate Overlap Events가 필요하다. 기본은 OnComponentHit이고 자동 선택 순서는 Sphere → Primitive 루트 → 첫 Primitive다.

End Execution After Hit 기본값은 true다. 명중 뒤 도트가 남아야 한다면 ApplyEffect Removal을 Independent로 사용한다. 현재 자동 조합의 Hit 모드는 Area/Query/Repeat/Beam과 함께 사용하지 않는다. 직격과 폭발을 동시에 다루는 스킬은 전용 스크립트에서 공통 기능을 호출하는 방식으로 확장한다.

### 지속 피해 장판

`GroundTargeting + TargetQuery + ApplyEffect(Query) + RepeatApplication`을 조합한다. 필요하면 Deployable로 설치 수명을 관리하고 AreaIndicator로 범위를 표시한다.

예: GA Instant, Repeat Interval 1초, Duration 5초, Apply Immediately 켬, Query Radius 300cm, 매 적용 Instant 피해 GE. 지면 선택은 Requires Confirmed Target Data를 켜고 프리뷰를 연결한다. 피해가 없는 방벽에 Repeat/Query/ApplyEffect를 억지로 추가할 필요는 없다.

## 10. 조합 규칙과 자주 막히는 부분

| 현상 | 확인할 내용 |
|---|---|
| Invalid skill composition | 같은 기능 컴포넌트 중복, TargetMode와 Query/Hit 연결, GE·Removal 설정, 반복/설치 수명 |
| TargetQuery와 AreaDamage를 함께 추가 | 둘 중 하나만 사용. 두 탐색 경로 동시 부착은 불가 |
| 투사체 발동 실패 | Sphere/ProjectileMovement 각각 1개, Launch + (Detonation 또는 Hit), Repeat/Deployable 없음, Follow Source 끔 |
| 체력 설치물 검증 실패 | Deployable + DeployableHealth + 액터 자체 ASC 1개 필요 |
| GE가 적용되지 않음 | 시전자와 대상 ASC, Effects 클래스, GE의 Attribute·SetByCaller 태그, 면역/적용 조건 |
| 방벽이 캐릭터 위치에 생성 | 실제 BP GroundTargeting, GA Confirmed Target Data, Follow Source 확인 |
| 방벽 원점은 지면인데 외형이 떠 있음 | 메시·Box 상대 Z/메시 원점 확인 |
| 홀딩 장판 없음 | GA Activate On Input Release, Preview Actor Class, 자동 프리뷰 조합, Display Radius와 Decal 확인 |
| 포물선 없음 | 프리뷰 BP ProjectileTrajectory, 실행 BP Launch 구성, Trace Channel/표시 설정 확인 |
| 거리 설정이 디테일에 없음 | 현재 PreviewData 멤버의 편집 선언 제한. 3절의 제한·대안 참고 |
| 탐색 결과 0명 | Object Type, Shape, 팀 정보, 필수/차단 태그, 가림 채널 확인 |
| 스피드 GE가 있어도 속도 그대로 | 플레이어 이동속도 Attribute와 MaxWalkSpeed 연결 확인 |
| 장판은 보이는데 피해 없음 | AreaIndicator는 시각화 전용. Query/Area와 GE 설정이 별도로 필요 |

디버그 원·포물선은 개발 확인용이며 Shipping 표시를 보장하지 않는다. 게임용 장판은 원형 마스크가 있는 Deferred Decal 머티리얼을 지정한다. AreaIndicator는 현재 원형 표시라 Box/Sector 판정 모양을 그대로 그리지 않는다. 바닥이 데칼을 수신하는지도 확인한다.

NF_SkillTest는 ASC를 상속하는 임시 테스트 컴포넌트다. 기존 ASC가 없는 Pawn에 사용한다. 이미 정식 ASC가 있는 플레이어에 두 번째 ASC를 붙이지 말고 정식 입력·Ability 부여 경로를 사용한다. Test Skill Class는 플레이 전에 설정하며 런타임 교체용이 아니다.

## 11. 팀에 넘기기 전 확인

- GA / 실행 BP / 프리뷰 BP / GE를 함께 전달하고 각 클래스 참조를 확인한다.
- 대상 Attribute, 팀 정보, 피해 태그, 충돌 채널을 관련 담당자와 맞춘다.
- GAS 사망·기절·침묵 취소 정책은 GA Input Cancel Tags와 Activation Blocked Tags 등 팀 태그로 설정한다.
- 비용·쿨다운 GE와 재발동 시 효과 중복 정책을 확인한다. Build Cost만 설정하면 발동 비용이 생기지 않는다.
- 로컬 프리뷰와 실제 서버 생성 위치·피해·파괴를 Dedicated Server 멀티플레이에서 확인한다.
- 특수 이동/피격 이벤트/무기 교체/폭격기 비행/포탈/실루엣 UI는 전용 로직으로 구현하고 공통 탐색·GE·수명 기능을 재사용한다.

새 태그는 팀의 용도.분류.이름 규약을 따르고 목적·작성자·날짜를 남긴다. 배포된 태그는 수정하지 않는다. 이 튜토리얼은 코드나 태그를 추가하지 않는다.

## 12. 코드 위치

| 역할 | 경로 |
|---|---|
| GA·실행·프리뷰·테스트 | `Source/Project_NF/GamePlay/Combat/Skill/` |
| 조합 컴포넌트 | `Source/Project_NF/GamePlay/Combat/Components/` |
| 포물선 프리뷰 | `Source/Project_NF/GamePlay/Combat/Projectiles/NF_ProjectileTrajectoryComponent.*` |
| 설치물 체력 Attribute | `Source/Project_NF/GAS/Attributes/NF_DeployableHealthAttribute.*` |
| 공통 효과 제출 경로 | `Source/Project_NF/GamePlay/Combat/Damage/NF_DamageLibrary.*` |
| 자동화 테스트 | `Source/Project_NF/Tests/NF_SkillComponentTests.cpp` |

2026-10-14 검증: Editor Win64 Development 빌드 통과, EffectsAndHits / TargetQuery 자동화 테스트 통과. 이는 BP 에셋의 배치·재질·무기 연결·Dedicated Server 플레이까지 검증했다는 뜻은 아니다. 이 문서 작성 작업에서는 빌드를 재실행하지 않았다.
