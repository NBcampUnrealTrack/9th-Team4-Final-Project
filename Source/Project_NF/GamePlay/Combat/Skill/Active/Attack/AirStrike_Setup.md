# 폭격기 호출 설정

작성: 스킬 담당 (구현: Codex) | 2026-10-13

## 구조

```text
NFGA_AirStrike
  조준: NF_SkillPreviewActor를 부모로 한 BP + GroundTargeting + AreaIndicator
  확정: 서버에서 NF_AirStrike 생성, 비용/쿨타임 Commit, GA 종료

NF_AirStrike
  WarningArea + PlaneBody + PlaneWings
  경고 대기 → 비행 → 구역 내 목표점 순서대로 폭탄 투하 → 경고 정리

NF_AirStrikeBomb
  Collision + Mesh + ProjectileMovement + AreaDamage
  낙하 → 첫 충돌 → 공통 광역 피해/GE 전달 → 연출 → 제거
```

## 바로 테스트하기

1. `NFGA_AirStrike`를 상속한 GA BP를 만든다.
2. 테스트 캐릭터의 `NF Skill Test Component > Test Skill Class`에 이 GA BP를 지정한다.
3. 공통 프리뷰 BP에 GroundTargeting/AreaIndicator를 부착하고 [BP 조합 가이드](../../Preview/SkillPreview_BP_Setup.md)의 Update Preview 연결을 작성한다. GA의 Preview Actor Class에 이 BP를 지정한다.
4. E를 누른 채 지면 목표를 조준하고, 떼면 공습을 확정한다.
5. 기본 도형 비행기와 폭탄이 제공되므로 별도 메시/피해 GE 없이 비행·투하를 테스트할 수 있다. 빨간 착탄 피해 검색 구는 Development/PIE 디버그 표시다.

## 기본값

| 설정 | 위치 | 기본값 |
|---|---|---|
| 목표 사거리 | GA | 4,000cm |
| 빌드 코스트 | GA | 4 |
| 폭탄당 피해 | GA | 30 |
| 공습 구역 반경 | 공습 액터 | 1,000cm |
| 경고 대기 | 공습 액터 | 1.5초 |
| 목표 중심 대비 비행 고도 | 공습 액터 | 2,200cm |
| 구역 진입 전/후 비행 거리 | 공습 액터 | 각각 1,000cm |
| 비행 속도 | 공습 액터 | 1,200cm/s |
| 폭탄 수 | 공습 액터 | 24, 최대 128 |
| 초기 하강 속도 | 공습 액터 | 400cm/s |
| 개별 폭탄 폭발 반경 | 폭탄 액터 | 250cm |

목표점은 구역 원 안에서 서버가 무작위로 분산한다. 지면 탐색에 실패한 목표점은 제외되므로 실제 투하 수는 설정 수보다 적을 수 있다. 비행 방향 순서로 목표점을 정렬하고, 비행기가 각 목표점의 진행축 위치에 도달하면 투하한다. 투하 간격은 목표점 분포와 비행 속도로 결정된다.

## BP로 연출/설정 교체

- `NF_SkillPreviewActor`의 자식 BP: GroundTargeting/AreaIndicator를 직접 추가하고 Update Preview 이벤트를 연결한다. 장판 색/데칼은 AreaIndicator에서 설정한다. GA의 Preview Actor Class에 지정하며, 공습 전용 C++ 프리뷰 클래스는 사용하지 않는다.
- `NF_AirStrike`의 자식 BP: 비행기 메시, WarningArea, 반경/고도/속도/폭탄 수 등을 설정. GA의 Strike Class에 지정.
- `NF_AirStrikeBomb`의 자식 BP: 폭탄 메시, ProjectileMovement 중력 배율, 폭발 반경, AreaDamage 대상 태그/가림 설정, On Explosion Cosmetic 연출을 지정. 공습 BP의 Bomb Class에 지정.
- PlaneWings는 임시 도형 날개다. 실제 비행기 메시로 교체할 때 이 컴포넌트의 메시를 비울 수 있다.
- 메시 물리 시뮬레이션은 끈다. 폭탄 Initial Speed는 0을 유지한다. 투하 코드가 초기 속도를 계산하고 ProjectileMovement가 이동한다.

## 피해와 네트워크

- GA의 Damage Effect Class와 Damage Magnitude Tag는 팀의 공통 피해 GE/SetByCaller 태그와 맞춘다. Damage Per Bomb은 양수 피해량이며 실제 감소 처리는 GE/Attribute 계산에서 수행한다.
- 피해 설정이 없으면 비행/폭탄/폭발 연출만 실행한다. 공격자 출처는 GA에서 생성한 Spec에 유지한다.
- 폭탄당 ASC 중복을 제거하지만 다른 폭탄에 연속 적중하면 각각 피해를 받을 수 있다.
- 기본 대상 필터는 자기 자신을 제외한다. 아군/사망/무적 등은 폭탄 BP의 Can Affect Target과 공통 GE 계약에서 설정한다. 팀 판정을 자동으로 구현하지 않았다.
- 개인 조준 표시는 로컬에서만 생성한다. 확정한 공습 액터/폭탄은 서버가 생성하고 이동을 복제한다. 경고 장판은 서버가 복제한 중심/반경으로 관련 클라이언트에 표시한다.
- 실제 비용/쿨타임은 Commit 시 적용한다. 확정 뒤에는 GA가 종료되어도 이미 생성한 공습과 폭탄은 계속 진행한다.
- 폭탄은 첫 충돌에서 폭발하며 바운스하지 않는다. 지형/장애물/플레이어에 조기 충돌하면 계획한 지점보다 먼저 폭발할 수 있다. 충돌이 없으면 Max Lifetime 후 제거한다.
- 경고 원은 폭탄 목표 중심이 분포하는 영역이다. 개별 폭탄의 폭발 반경 때문에 원 밖에도 피해가 갈 수 있으며 구역 전체가 매 순간 동일하게 피해를 받는 방식이 아니다.

## 목표 선택의 범위

- 로컬/서버 모두 Avatar 눈 위치와 조준 회전으로 지면 목표를 계산한다. TPS 카메라의 정확한 화면 중앙 TargetData 전송은 아직 연결하지 않았다.
- 정면 Trace가 지면을 맞히면 해당 위치를 사용한다. 아무것도 맞히지 않으면 조준 끝점에서 아래쪽 지면을 탐색한다. 벽과 너무 가파른 표면은 목표로 허용하지 않는다.
- 서버 GA가 발동할 때 중심/비행 방향을 확정하고, Cast Time이 있더라도 목표를 고정한다. 준비 중 사거리 밖으로 이동하면 비용을 적용하지 않고 취소한다.
- `NF_SkillTestComponent`는 공습 전용 코드를 갖지 않는다. 공통 GA의 Preview Actor Class/Activate On Input Release 연결을 그대로 사용한다.

## 사용자 빌드 후 확인

- E 홀딩 중 장판 위치 갱신, 해제 시 고정 구역 경고와 비행이 시작되는지.
- 서버/클라이언트에서 같은 비행기와 폭탄을 보는지.
- 구역 내 여러 지점에 낙하하며 폭탄끼리 충돌하지 않는지.
- 각 폭탄의 착탄 피해/팀 필터/벽 가림/공격 출처가 맞는지.
- 취소/코스트 부족 시 공습이 생성되지 않고, 공습 종료 후 액터/경고가 정리되는지.

이번 변경은 빌드/에디터 실행 검증을 하지 않았다.
