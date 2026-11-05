# 지상 유도탄 패턴

`NFGA_BossHomingVolley`가 비행/강하 패턴을 대체한다.

- 범위 안의 생존 플레이어 중 가까운 순서로 최대 3명, 각 1발을 선택한다.
- 매 회차마다 대상을 다시 선택하며 총 3회 반복한다. 첫 탄은 즉시, 이후 탄은 0.6초 간격으로 발사한다.
  마지막 실제 발사 후 5초에 다음 회차를 시작한다. 대상이 없거나 모든 발사가 실패한 회차도 1회로 세며 5초 후 재선정한다.
- 각 발사 시 `ShotMontage`를 보스 메시에서 재생한다. GA BP의 `NF|Boss|Homing|Animation`에서
  몽타주, 재생 속도, 시작 섹션을 지정한다. None이면 연출만 생략한다. 몽타주 길이와 관계없이 발사는 0.6초 간격이다.
  기존 스킬 연출 복제 액터를 사용하여 클라이언트에도 전달한다.
- 각 대상에서 보스 쪽 월드 X 방향으로 600cm 떨어진 바닥에서 발사한다.
  Y는 대상의 발사 시점 좌표로 고정한다. 바닥을 찾지 못한 발사 지점은 건너뛴다.
- 월드 X/Z만 초당 600cm 속도로 추적한다. 대상의 Y 이동은 추적하지 않는다.
- 감지 구체(기본 120cm)에 생존 플레이어가 들어오면 폭발한다. 이동 구간을 Sweep하여 관통 누락을 방지한다.
- 생성 후 4초가 지나면 현재 위치에서 폭발한다. 대상이 사망/제거되면 정지하여 남은 수명을 기다린다.
- 폭발 반경 기본 300cm. 플레이어마다 한 번씩 CommonDamage와 1초 Airborne GE를 적용하고 위로 LaunchCharacter한다.
  1초는 상태 GE 지속시간이며 실제 착지 시간은 캐릭터 중력/지형의 영향을 받는다.
- 기존 DT_BossCombat의 RelocateDamage를 사용한다(기본 40). 나머지 수치는 GA BP에서 조정한다.
- 서버가 이동/폭발/피해를 처리하고 액터 이동 및 제거를 복제한다. 메시 기본값은 구체이며 ProjectileClass BP로 교체 가능하다.
- 3회차 발사가 끝나고 남은 유도탄이 모두 사라지면 GA가 종료된다. 회차 사이에는 종료하지 않는다.
  GA 취소/보스 사망 시 발사·회차 타이머와 남은 탄을 정리한다.

## StateTree 연결

기존 저장된 `NFGA_BossRelocate` 참조는 호환 클래스가 새 GA를 상속하므로 새 패턴을 실행한다.
새 트리 생성 도구는 `HomingVolley` 상태와 `NFGA_BossHomingVolley`를 직접 지정하며 보스 부여 목록도 함께 갱신한다.
기존 `.uasset`의 상태 이름은 재생성 전에는 RelocateAndEmerge로 남을 수 있다.
기존 Relocate BP의 비행 관련 설정은 더 이상 사용하지 않는다.

## 나이아가라 BP 설정

- 유도탄 BP (`NF_BossHomingProjectile` 파생)의 `NF|BossProjectile|VFX`에서
  `ExplosionNiagaraSystem`과 `ExplosionNiagaraScale`을 지정하면 근접/시간초과 폭발 위치에 재생한다.
  이 BP를 GA의 `ProjectileClass`에 지정한다. 취소로 제거될 때는 폭발 연출이 없다.
- GA BP의 `NF|Boss|Homing|VFX`에서 `LaunchNiagaraSystem`, `LaunchEffectSocket`,
  `LaunchEffectLocalOffset`, `LaunchNiagaraScale`을 지정하면 매 발사마다 보스 쪽에 재생한다.
  소켓 미지정/누락 시 보스 메시 위치를 사용한다. 발사 시점의 위치에 고정되는 일회성 효과다.
- 커스텀 BP 그래프는 유도탄의 `Event OnHomingExplosion(Location, BlastRadius)` 또는
  GA의 `Event OnHomingMissileLaunched(Boss, Projectile, BossEffectTransform)`을 사용한다.
  두 이벤트 모두 서버에서 호출된다. 그래프에서 `Spawn Replicated Boss Niagara` 노드를 호출하여
  AuthorityActor에는 Self(유도탄 이벤트) 또는 Boss(GA 이벤트), 시스템과 월드 위치/회전/스케일을 전달한다.
  일반 `Spawn System at Location`만 호출하면 서버에서만 재생되므로 멀티플레이에는 복제 노드를 사용한다.
- 자동 시스템 지정과 이벤트 내 재생을 같은 효과에 동시에 사용하면 두 번 재생된다.
  이벤트로 직접 처리할 경우 자동 시스템은 None으로 둔다.
- 복제용 연출 액터는 유도탄과 독립적이므로 폭발 직후 유도탄이 제거되어도 재생이 유지된다.
  완료 후 자동 정리되는 **비루프(one-shot) Niagara System**을 사용한다.

## 실행 점검

자동화: `NF.Boss.Homing.WorldXZTracking`
PIE 점검: 대상 0/1/3/4명, Y 방향 회피, 감지 시 조기 폭발, 4초 강제 폭발,
대상 사망/접속 종료, 보스 사망 중 탄 제거, 서버/클라이언트 위치 및 피해 일치.
