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

### 가시갑옷: NFGA_Thorns (2026-10-22)

부모 NFGA_Thorns으로 GA BP를 만든다. Skill Type Passive, Activation Method On Granted, Activate Passive on Grant 끔, 실행/프리뷰 액터 None으로 사용한다. `NF > Thorns > Reflection Ratio`는 0.3이면 실제 체력 손실의 30%, Internal Cooldown은 기본 1초다. Reflection Effect Class는 기본 NFGE_ThornsDamage를 사용한다. 별도 컴포넌트/이벤트 그래프/Passive Effects 수동 등록은 필요 없다. 장착 시스템에서 GA를 부여하거나 NF_SkillTest 슬롯에 지정하고 PIE를 시작하면 구독하며, GA 제거 시 해제한다.

서버 ASC의 기존 Health 변경 알림(FOnAttributeChangeData)의 OldValue/NewValue와 GEModData를 사용한다. `(피해 전 체력 - Max(피해 후 체력, 0)) × 반사율`이며 손실은 0부터 피해 전 체력까지만 인정한다. 체력 100에서 1000 피해를 받아도 반사율 30%면 30을 요청한다. 100→70이면 9, 실드만 감소/회복/GE 정보 없는 직접 스탯 변경/자해/공격자 없는 피해는 반사하지 않는다. 반사 시도 전에 내부 쿨타임을 예약하며 반사 피해 Spec에 Damage.Reflected를 붙여 재반사를 막는다. 이 표시는 다른 반사 GE를 직접 사용할 때도 유지된다. 반사량 태그는 Data.Damage.Reflected다.

가시갑옷 관찰 대상은 NF_PlayerStatusAttribute.Health가 있는 플레이어 ASC다. 임시 SkillTest ASC에 체력이 없으면 아바타의 정식 ASC에서 해당 속성을 찾아 관찰한다. 기본 TPS에 이 체력 AttributeSet 자체가 없으면 경고를 출력하고 구독하지 않는다. 플레이어 AttributeSet/사망/무기 소스는 수정하지 않았다. 단순 원시 체력 감소 중계라 사망 상태로 이미 바뀌었다는 이유만으로 방금 받은 치명타격의 실제 체력 손실을 제외하지 않는다. 사망 처리에서 GA 자체를 먼저 제거하면 구독도 해제되므로 팀의 사망 처리 순서에 따라 추가 검증이 필요하다.

기본 반사 GE는 공격자의 플레이어/보스 Health 또는 몬스터/설치물 IncomingDamage에 피해를 전달한다. 기존 직접 체력 피해 흐름처럼 별도의 공격자 실드 흡수/방어력 감소 계산은 추가하지 않는다. 향후 공통 피해 계산을 도입하면 Reflection Effect Class를 해당 반사 피해 GE로 교체할 수 있다. 이 설정은 **피격자의 실드 피해를 반사 기준에서 제외**하는 규칙과 구분한다. 보호/면역 GE가 반사 피해를 거부하는 경우에도 발동 시도에 대한 내부 쿨타임은 소비한다.

테스트 소스 NF.Skills.Thorns.ActualHealthLossCooldownAndNoRecursion 추가: 초과 피해 상한, 실제 Health GE 콜백 반사, 실드/회복 제외, 내부 쿨타임, 재반사 방지, 장착 해제 검사. 빌드·테스트 실행·PIE는 수행하지 않았다.

### 사망 상태 발동 차단 처리 완료 (2026-10-22)

`NFGA_SkillBase` 생성자에 `ActivationBlockedTags.AddTag(NFTag_Player::Player_State_Death)`를 추가했다. 이 베이스를 상속하는 조합 스킬, 이동/가속화/그랩/EMP/탐색/포탈 및 패시브 GA에 공통 적용된다. 기존 BP가 상속 차단 태그 목록을 덮어써도 우회하지 못하도록 CanActivateAbility/CanContinueSkillExecution에서도 같은 상태를 확인한다. 사망 태그 변경을 공통 취소 감시에 등록해 로컬 조준을 해제하고 취소 가능한 실행 중 스킬을 기존 정리 경로로 종료한다.

무기 Fire/Reload/Zoom GA는 이미 사망 차단 태그가 있어 변경하지 않았다. 플레이어 사망 GA/몬스터 전용 기본 공격 GA/플레이어 테스트 전용 GA는 변경하지 않았다. `CancelAbilitiesWithTag`는 발동 시 다른 능력을 취소하는 설정이며 사망 태그 감시 기능이 아니므로 공통 베이스에 추가하지 않았다. 사망 시 취소는 기존 태그 변경 감시를 사용한다. 기본 패시브 GE와 이미 GA에서 분리된 설치물/발사체를 사망만으로 제거하는 정책은 이번 변경 범위에 포함하지 않는다.

플레이어 사망 처리에서 **스킬이 부여된 ASC에 `Player.State.Death`를 실제 부여해야** 차단된다. 플레이어 소스와 사망 태그 부여/해제 정책은 수정하지 않았다. 별도 NF_SkillTest ASC로 테스트하는 경우 플레이어 본 ASC의 태그가 자동 공유되지는 않는다. 이 테스트 구성의 ASC 통합은 별도 연결이 필요하다.

처리 내용: 공통 스킬 GA 생성자 사망 차단 태그 등록, BP 태그 덮어쓰기 우회 방지, 시전 중 사망 취소 연결. 테스트 소스 `NF.Skills.Death.BlockCastAndCancelPendingExecution` 추가. 빌드·테스트 실행·PIE 사망 검증은 수행하지 않았다.

### 공격력 증가·마탄: 무기 패시브 (2026-10-21)

| 스킬 | GA BP 부모 | 디테일 | 기본값 |
|---|---|---|---|
| 공격력 증가 | NFGA_AttackPower | NF > Attack Power > Attack Damage Bonus | +10, Build Cost 1 |
| 마탄 | NFGA_MagicBullet | NF > Magic Bullet > Last Round Damage Multiplier | 2배, Build Cost 3 |

두 GA 모두 Skill Type Passive, Category Attack, Activation Method On Granted, Execution/Preview Actor None, Activate Passive on Grant 끔으로 사용한다. 이벤트 그래프·추가 컴포넌트는 필요 없다. GE를 Passive Effects에 수동 추가하지 않는다. 각 GA가 네이티브 Infinite GE를 생성하고 SetByCaller 수치를 전달하며 공통 패시브 정리 기능에 등록한다. 직접 GE를 추가하면 중복 적용되거나 필수 SetByCaller 값이 누락될 수 있다.

서버가 GA를 부여할 때 소유 ASC에 `NF_OffenseAttributeSet`을 자동 등록한다. 기존 플레이어 AttributeSet/PlayerState 소스를 수정하지 않는다. 속성은 `AttackDamageBonus`(기본 0), `LastRoundDamageMultiplier`(기본 1)이다. GA를 제거하면 자신이 적용한 GE만 제거한다. 다른 패시브가 남을 수 있으므로 AttributeSet 자체는 제거하지 않는다. EMP는 두 패시브를 중단시키지 않는다.

장착 시스템에서 GA를 부여하거나 NF_SkillTest의 비어 있는 E/Q/Shift 슬롯에 두 GA를 지정하고 PIE를 시작한다. 패시브라 키를 누를 필요 없다. NF_SkillTest의 능력 부여 시 적용되며 직접 키로 활성화를 시도하면 거부되는 것이 정상이다. 해제는 ClearAbility 등으로 실제 Ability Spec을 제거해야 한다.

`NF_WeaponBase::FireProjectile()`가 **탄약 차감 전** `CurrentAmmo == 1`을 판정하고 `(WeaponData.Damage + 공격력 보너스) × (마지막 탄환이면 마탄 배율, 아니면 1)`로 이번 발사 피해를 확정한다. 기본 30/+10/마탄 2배면 일반 탄환 40, 마지막 탄환 80이다. 샷건은 해당 발사의 모든 펠릿에 같은 값을 전달한다. 마지막 탄환에 발동한 마탄은 빗나가도 소모한 발사에 해당하며, 명중 시점에 다시 탄약이나 패시브를 조회하지 않는다. 이미 발사된 탄환은 패시브 해제/무기 교체 이후에도 저장된 피해를 유지한다. WeaponData 원본과 탄약/재장전 로직은 변경하지 않는다.

공격력 증가 GE가 중복 부여되면 보너스는 합산되고, 마탄 GE가 중복 부여되면 배율은 Multiply Compound로 곱해진다. 같은 스킬 중복 장착을 금지하려면 장착 시스템에서 제한한다. 기본 플레이어 ASC와 임시 NF_SkillTest ASC가 함께 존재하는 테스트 환경에서는 현재 발사자 아바타에 연결된 ASC들의 공격 보너스를 중복 조회 없이 합산/곱산한다.

이 변경은 **무기 탄환의 피해량 계산**만 연결한다. 수류탄·레이저 등 독립 스킬 피해에는 자동 반영되지 않는다. 일반 몬스터·플레이어 대상 명중 피해 GE 연결을 새로 추가하지도 않는다. 실제 대상의 체력 감소는 기존 탄환 명중 처리 지원 범위에 따른다. 공격 보너스 SetByCaller 태그는 `Data.Weapon.AttackDamageBonus`, 마탄은 `Data.Weapon.LastRoundMultiplier`이며 NFTag_Skill에 소유자·날짜·목적과 함께 등록했다.

테스트 소스: `NF.Skills.WeaponPassives.AttackBonusLastRoundAndProjectileSnapshot`. 실제 무기 발사 루프의 펠릿 피해 스냅샷·마지막 탄환·원본 데이터 유지·장착 해제·중복 효과·EMP 유지·별도 테스트 ASC 조회를 검사한다. 빌드/자동화 테스트 실행/PIE 확인은 수행하지 않았다. BP 에셋은 변경하지 않았다.

### 더블점프: NFGA_DoubleJump (2026-10-21)

부모 클래스를 **NFGA_DoubleJump**으로 선택해 `GA_DoubleJump` BP를 만든다. Max Jump Count는 2, Skill Type은 Passive, Category는 Utility, Activation Method는 On Granted로 둔다. Execution Actor/Preview Actor는 None, Activate Passive on Grant는 끈다. Passive Effects에는 기본으로 등록된 `NFGE_DoubleJump`를 유지한다. 이 GE가 기존 `State.DoubleJumpEnabled` 태그를 부여한다. 이벤트 그래프 연결은 필요 없다.

장착 시스템에서 GA를 서버 ASC에 부여하면 즉시 적용되고, 해당 Ability Spec을 제거하면 해제된다. 임시 테스트는 NF_SkillTest의 비어 있는 E/Q/Shift 슬롯에 이 GA를 지정해 플레이를 시작하면 된다. **E/Q/Shift를 누르는 방식이 아니다.** 서버 BeginPlay의 능력 부여 시 적용되므로 Space → 공중에서 Space로 확인한다. 키를 눌러 직접 활성화하려 하면 패시브이므로 거부되는 것이 정상이다. 실행 중 Test Skill Class만 바꾸는 것은 능력 제거가 아니므로 해제 테스트는 장착 시스템의 ClearAbility 또는 PIE 재시작으로 한다.

기존 Character.Jump 입력을 그대로 사용한다. 캐릭터의 점프 입력이 연결되어 있어야 하며, 기본 Character 및 모션매칭 캐릭터 모두 대상으로 한다. 새 키 바인딩/플레이어 소스 수정은 없다. 커스텀 캐릭터가 별도 CanJump 정책으로 공중 점프를 막으면 그 정책 확인이 필요하다. 이 작업에서 플레이어 입력·애니메이션은 수정하지 않았다.

스킬 내부 `NF_JumpCountEffect`가 캐릭터별 요청을 묶어 점프 횟수를 `Max(원래 횟수, 요청 횟수)`로 설정한다. 중복 장착은 횟수를 합산하지 않으며 마지막 요청이 해제되면 원래 횟수를 복구한다. 아바타 교체 시 이전 캐릭터는 복구하고 새 캐릭터에 다시 적용한다. JumpCurrentCount를 초기화하지 않아 공중 장착으로 사용 횟수를 리필하지 않는다. EMP는 패시브를 제거하지 않는다.

UE 5.7 Character.cpp는 JumpMaxCount의 기본 복제를 비활성화하므로 스킬 내부 보조 액터가 요청 횟수를 복제해 서버/클라이언트 양쪽에 적용한다. 보조 액터 BP/컴포넌트 추가는 필요 없다. 장착/해제 반영에는 네트워크 지연이 있을 수 있다. 별도 점프 횟수 제어 코드가 동시에 값을 직접 쓰는 정책은 팀 간 조율이 필요하며, 해제 시 외부 변경 값이 확인되면 덮어쓰지 않는다.

자동화 테스트 소스 `NF.Skills.DoubleJump.GrantRemoveOverlapAndAvatarChange`를 추가했다. 빌드·테스트 실행·PIE 멀티 점프 검증은 수행하지 않았다.

### 롤링 봄: 지면 이동 조합 (2026-10-20)

`BP_RollingBomb`의 부모는 `NF_SkillExecutionActor`를 유지한다. 기존 `NF Projectile Launch`와 `Projectile Movement`를 **제거**하고 아래 조합을 사용한다. 이벤트 그래프 연결은 필요 없다.

| 컴포넌트 | 설정 예시 |
|---|---|
| Sphere Collision | 반지름 15cm. 이동 컴포넌트가 루트/충돌을 자동 설정 |
| Static Mesh | 폭탄 메시. Sphere 아래 배치, Simulate Physics 끔 |
| NF Ground Travel | Travel Speed 1000cm/s, Initial Boost Multiplier 2, Boost Duration 0.4초, Max Travel Distance 2500cm, Spawn Forward Distance 100cm |
| NF Detonation | Mode Impact, Max Lifetime 10초 |
| NF Area Damage | Sphere, Radius 400cm |

Class Defaults의 Follow Source는 끄고, Damage Effect Class·Damage Magnitude Tag·Damage Magnitude는 기존 피해 설정을 유지한다. Repeat/Deployable/GroundTargeting/BeamTargeting/HitEffect는 넣지 않는다. ApplyEffect를 쓰는 경우 Query 모드로 Area와 조합한다.

GA 부모는 `NFGA_SkillBase`, Execution Actor Class는 `BP_RollingBomb`, Mode는 Instant, Cast Time 0, Activate on Input Release 끔, Requires Confirmed Target Data 끔, Preview Actor Class None으로 시작한다. 이동 컴포넌트를 감지해 바라보는 방향 데이터를 자동 전달하며 수직 조준은 수평으로 투영한다. 투척용 포물선 프리뷰는 사용하지 않는다. GA를 NF_SkillTest의 원하는 키에 지정한다.

폭탄은 캐릭터의 발 높이를 기준으로 전방 지면에 생성되어 초기 부스트 후 순항 속도로 경사를 따라 전진한다. 아래 트레이스로 지면을 찾고 Sphere를 SafeMoveUpdatedComponent로 이동한다. 적/벽 충돌, 급경사, 지면 끊김, 최대 거리에서 이동을 끝내고 폭발을 요청한다. 바닥 접촉 자체로는 폭발하지 않는다. 낭떠러지에서는 떨어지지 않고 끝 지점에서 폭발한다. 시작 지면이나 발사 공간이 없으면 생성 준비를 거부한다. 이동은 서버에서만 수행하고 실행 액터의 위치 복제를 사용한다.

`NF Ground Travel > Boost`의 Initial Boost Multiplier가 출발 배율, Boost Duration이 순항 속도로 내려오는 시간이다. 기본 1000cm/s × 2배 = 2000cm/s로 출발해 0.4초 동안 부드럽게 1000cm/s로 내려온다. 배율 1 또는 시간 0으로 기존 일정 속도를 사용할 수 있다. 부스트는 실제 이동속도 변화이며 별도 이펙트/사운드는 추가하지 않았다. 부스트 거리도 최대 이동거리에 포함되고 벽/적 충돌 판정은 그대로 유지한다. 빌드·실행 검증은 수행하지 않았다.

Ground Search Depth는 아래 지면 탐색 깊이, Step Height는 위쪽 탐색 여유, Ground Clearance는 지면과 Sphere의 간격, Min Ground Normal Z는 허용 경사 기준이다. 별도 점프/단차 오르기/물리 굴림은 구현하지 않는다. 메시의 바퀴 회전·추진 연출은 별도 cosmetic 작업이다.

이동 종료에 따른 폭발은 Detonation의 타이머와 별개로 요청한다. Impact 모드에서는 Max Lifetime이 빗나갔을 때의 최대 수명이다. Fuse/FuseOrImpact 모드를 선택하면 Fuse Time이 추가 시간 제한이 된다. BP 에셋 변경·빌드·PIE는 수행하지 않았으며 자동화 테스트 소스 `NF.Skills.GroundTravel.GroundWallDistanceAndComposition`을 추가했다.

### 가속화: NFGA_Acceleration (2026-10-20)

부모 클래스를 `NFGA_Acceleration`으로 선택해 `GA_Acceleration` BP를 만든다. 이벤트 그래프·실행 액터·속도 GE는 필요 없다.

| 항목 | 설정 |
|---|---|
| NF > Acceleration > Speed Multiplier | 1.3 = 30% 증가 |
| Skill Type / Category | Active / Movement |
| Skill Execution Mode / Execution Duration | Timed / 5초 |
| Cast Time | 0 (원하면 시전 대기 시간 지정) |
| Execution Actor Class / Preview Actor Class | None / None |
| Activate on Input Release / Requires Confirmed Target Data | 모두 끔 |
| Affected by EMP / Cancel on EMP | 모두 켬 |
| Cooldown Gameplay Effect Class | 필요하면 기존 쿨타임 GE 지정 |

캐릭터의 `NF_SkillTest` E/Q/Shift 중 원하는 Test Skill Class에 새 GA를 지정한다. 누르면 시전 대기 후 배율을 적용하고, 키를 떼어도 지속시간 동안 유지한다. 시간 종료·EMP·취소·캐릭터 제거 시 자신이 곱했던 배율만 한 번 나누어 해제한다. 시전 대기 중 취소하면 배율과 비용/쿨타임을 적용하지 않는다.

대상은 Character와 CharacterMovement가 있어야 한다. 실제 `MotionMatchingCharacterMovementComponent`와 `MotionMatchingComponent`를 함께 쓰면 기존 배율 API/복제를 사용한다. 일반 TPS의 기본 CharacterMovement는 `MaxWalkSpeed`와 `MaxWalkSpeedCrouched`에 배율을 적용한다. 이 값들은 엔진에서 자동 복제하지 않으므로 GA가 내부 `NF_AccelerationEffect`를 생성해 서버·클라이언트 양쪽에 적용하고 종료 시 해제한다. 보조 액터용 BP나 수동 연결은 필요 없다. 플레이어 소스는 수정하지 않는다. 가속화가 켜진 상태에서 재입력해도 같은 GA의 효과를 중복 적용하지 않는다.

일반 TPS에 테스트 목적으로 `MotionMatchingComponent`만 추가했다면 제거한다. 해당 컴포넌트는 매 틱 `MaxWalkSpeed`를 다시 쓰므로 기본 이동 경로와 충돌하며 `[Acceleration]` 경고를 출력한다. 완전한 모션매칭 캐릭터의 컴포넌트는 제거하지 않는다.

다른 속도 효과도 현재 배율에 곱하고 해제 시 자신의 배율로 나누는 규칙을 사용하면 함께 유지된다. 다른 코드가 Setter로 절대 배율을 덮어쓰면 복구 결과가 어긋날 수 있다. 이 구현은 해당 문제를 해결하는 공통 배율 목록을 플레이어에 추가하지 않는다. 네트워크 예측은 구현하지 않아 클라이언트의 체감 반영은 기존 복제 지연을 따른다.

자동화 테스트 소스: `NF.Skills.Acceleration.LifetimeEMPAndFactorCleanup`. 빌드·테스트 실행·PIE 검증은 수행하지 않았다.

### GE를 사용하는 일반 자기 버프

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

### 버서커: 실제 체력 피해에 따른 공격력 중첩

`NFGA_Berserker`를 부모로 GA BP를 만들고 장착/부여한다. 입력으로 실행하는 액티브가 아니라 부여 중 서버에서 체력 변화를 관찰하는 패시브다. 실행 액터나 별도 GE BP는 필요 없다.

| 설정 | 기본값 | 의미 |
|---|---|---|
| Damage Bonus Per Stack | 3 | 중첩당 무기 공격력 가산 |
| Max Stacks | 5 | 최대 중첩 수 (기본 최대 +15) |
| Stack Duration | 0초 | 0이면 사망/장착 해제까지 유지. 양수이면 피격마다 전체 중첩 시간이 갱신되고 만료 시 함께 제거 |

GE 적용으로 실제 Health가 감소한 경우에만 타격당 1중첩이다. 실드 피해·회복·직접 Health setter는 제외하며, 자해/도트/반사 피해도 실제 Health를 감소시키면 포함한다. 치명타격으로 체력이 0이 되면 새 중첩 대신 초기화한다. 사망 태그, 아바타 변경, 장착 해제 시에도 초기화한다.

기존 `NFGE_AttackDamageBonus`의 SetByCaller 수치만 갱신하며 자신의 GE만 제거하므로 공격력 증가 패시브와 함께 사용할 수 있다. 공격력은 현재 `WeaponBase`의 발사 시점 피해량 계산에 반영되며, 독립적인 수류탄/스킬 피해에 자동 적용되지는 않는다. 플레이어 코드는 수정하지 않는다. `NF.Skills.Berserker.HealthOnlyStacksAndCleanup` 테스트 소스를 추가했으며 이번 작업에서 빌드/실행하지 않았다.

### 패시브 발동 이펙트 설정

GA BP의 `NF > Skill > Passive > Visual`에서 다음을 설정한다. 조건 태그를 추가할 필요는 없으며, 실제 발동에 성공한 코드가 공통 함수를 호출한다.

| 항목 | 설정 |
|---|---|
| Activation Cue Tag | 순간 연출용. 비우면 연출 생략 |
| Active Cue Tag | 지속 연출용. 비우면 연출 생략. 서로 다른 패시브는 서로 다른 태그 사용 |

가시갑옷 기본 순간 태그는 `GameplayCue.Skill.Thorns.Activate`, 버서커는 `GameplayCue.Skill.Berserker.Activate`다. 같은 태그를 지정한 `GameplayCueNotify_Static` BP를 만들어 `On Execute`에서 Niagara/Sound를 재생한다. Cue BP는 프로젝트 Gameplay Cue 검색 경로 아래에 두어야 한다. 현재 별도 경로 설정이 없으므로 기본 `/Game` 검색을 사용한다. 서버에서 ASC의 Cue 경로를 호출하며, 실제 멀티 연출은 네트워크 관련성이 있는 클라이언트에서 확인한다.

`My Target`은 패시브 소유 캐릭터다. Parameters의 `Location`은 발동 위치, `Source Object`는 영향받은 액터, `Raw Magnitude`는 가시갑옷의 반사 요청 피해량 또는 버서커의 현재 중첩 수다. Source Object는 대상이 클라이언트에 복제되지 않으면 없을 수 있으므로 위치를 대체값으로 사용한다. 가시갑옷은 반사 GE 적용 성공 시에만, 버서커는 새 중첩이 생길 때만 순간 연출을 보낸다. 버서커가 최대 중첩에서 시간을 갱신할 때는 순간 연출을 반복하지 않는다.

버서커 지속 연출이 필요하면 별도 태그를 등록해 `Active Cue Tag`에 지정하고, 같은 태그의 `GameplayCueNotify_Actor` BP에서 `On Active`/`While Active`에 캐릭터 부착 이펙트를 만든다. `On Remove`에서 Niagara를 중지/제거하고 자동 종료 설정을 확인한다. 첫 중첩에서 시작하고 중첩 만료·사망·장착 해제·아바타 변경 시 종료한다. 같은 ASC에 같은 Active Cue 태그를 다른 경로로 중복 추가하지 않는다. 지속 무적 GE를 쓰는 불굴은 향후 그 GE의 Gameplay Cues에 직접 연결하는 방식도 가능하다.

공통 API는 `NotifyPassiveTriggered(AffectedActor, Location, Magnitude)`, `StartPassiveActiveCue`, `StopPassiveActiveCue`이며 서버에서 호출한다. 실제 발동 로직과 자동 연결되어 있지 않은 패시브는 성공 지점에서 이 함수를 호출해야 한다. 장착만으로 발동 이펙트를 재생하지 않는다.

**더블점프 연결 대기:** 기본 태그 `GameplayCue.Skill.DoubleJump.Activate`만 설정했다. 현재 스킬은 JumpMaxCount를 확장할 뿐 실제 점프 성공 이벤트를 받지 않는다. 플레이어 담당이 서버의 `OnJumped` 성공 지점에서 `JumpCurrentCount >= 2`인 경우 부여된 더블점프 GA 인스턴스의 `NotifyPassiveTriggered(Character, CharacterLocation, JumpCurrentCount)`를 호출하는 연결이 필요하다. 플레이어 수정 전 확인 요청 원칙에 따라 이번 작업에서는 플레이어 코드를 수정하지 않았다. 불굴은 발동 로직이 아직 없어 자동 연결하지 않았다. Cue BP/나이아가라 에셋은 직접 제작하며, 이번 작업에서는 빌드/자동화 테스트를 실행하지 않았다.

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
