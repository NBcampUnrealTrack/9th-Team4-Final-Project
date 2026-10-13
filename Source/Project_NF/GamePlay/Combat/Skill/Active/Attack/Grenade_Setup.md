# 수류탄 설정 및 연결

작성: 스킬 담당 (구현: Codex) | 2026-10-12

## 구조

- `UNFGA_Grenade`: 서버에서 조준 계산, 준비 시간 대기, 비용/쿨타임 Commit, 액터 생성, GA 종료.
- `ANF_Grenade`: 서버에서 곡사 이동과 바운스, 투척 후 지연 폭발, 범위 GE 적용. 클라이언트는 복제 이동과 연출을 처리.
- 새 커스텀 컴포넌트, AttributeSet, 피해 GE, Gameplay Tag는 생성하지 않았다.

## 에디터 설정

1. `NF_Grenade`를 상속하는 `BP_NF_Grenade`를 생성한다.
   - Mesh에 수류탄 메시를 지정한다. 메시에는 물리 시뮬레이션을 켜지 않는다.
   - Projectile Movement에서 중력 배율, 바운스, 마찰을 설정한다.
   - Fuse Time(기본 3초), Explosion Radius(기본 400cm)를 설정한다.
   - On Explosion Cosmetic에서 전달된 위치에 독립적인 폭발 이펙트/사운드를 생성한다. 수류탄은 곧 제거되므로 이펙트를 수류탄에 붙이지 않는다.
   - Can Affect Target을 재정의해 팀과 사망 상태 등 게임 규칙을 검사한다. 기본 구현은 자기 자신을 제외하며 아군을 자동으로 구분하지 않는다.
2. `NFGA_Grenade`를 상속하는 `BP_NFGA_Grenade`를 생성한다.
   - Grenade Class에 `BP_NF_Grenade`를 지정한다.
   - 피해까지 테스트할 때는 Damage Effect Class에 지우 담당의 공통 피해 GE를 지정한다. 투척만 테스트할 때는 비워도 된다.
   - Damage Magnitude Tag에 그 GE가 읽는 SetByCaller 태그를 지정한다. 기존 태그를 확인하고 없는 경우 팀 태그 규칙에 따라 추가한다.
   - Damage는 양수 피해량(기본 50)으로 전달한다. GE의 Execution 또는 IncomingDamage 처리가 이 값을 피해로 해석해야 한다. Health에 양수 Additive로 직접 연결하면 회복되므로 그렇게 설정하지 않는다.
   - Cast Time은 투척 전 준비 시간이다. 준비 중 취소하면 비용/쿨타임을 적용하지 않는다.
   - Throw Speed(기본 1200cm/s), Throw Angle Offset(기본 위쪽 15도), Spawn Offset을 설정한다.
   - Cost/Cooldown Gameplay Effect Class는 필요한 전용 GE를 지정한다. Build Cost(기본 2)와 사용 비용은 별개다.
3. 테스트 캐릭터/Pawn BP의 Add Component에서 `NF Skill Test Component`를 추가하고, 해당 컴포넌트의 `NF > Skill Test > Test Skill Class`에 수류탄 GA BP를 지정한다.
   - 캐릭터 부모 클래스를 변경할 필요는 없다. GameMode가 스폰하는 캐릭터도 BP에 컴포넌트를 추가하면 된다.
   - 이 컴포넌트 자체가 테스트용 ASC이며 서버에서 해당 GA를 한 번 부여한다. 이미 ASC가 있는 캐릭터에 중복으로 추가하지 않는다.
   - 멀티플레이 테스트 Pawn은 Replicates를 켜고 PlayerController로 Possess해야 한다. E 키는 컴포넌트가 생성한 Enhanced Input Action/Mapping Context로 연결한다.
   - 수류탄은 E 키를 누르는 동안 예상 궤적을 표시하고, 떼면 TryActivateAbilityByClass로 서버에 투척을 요청한다. 다른 GA는 누르는 순간 발동한다.
   - 임시 테스트 경로이며 플레이 중 Test Skill Class 교체는 지원하지 않는다. 테스트 ASC에는 별도 AttributeSet이 없으므로 실제 피해 대상/자원에는 팀의 AttributeSet 연결이 필요하다.
   - Damage Effect Class 또는 Damage Magnitude Tag가 비어 있어도 투척/바운스/폭발은 실행한다. 유효한 피해 Spec이 없으면 폭발 시 피해 적용만 건너뛴다.
   - 이 GA의 실행 흐름은 C++가 담당하므로 BP Activate Ability에 투척을 중복 구현하지 않는다.

## 조준과 판정

- 서버 Avatar의 GetActorEyesViewPoint를 사용한다. TPS의 로컬 스프링암 카메라와 정확히 같은 화면 중앙 조준을 보장하지는 않는다. 이후 클라이언트 TargetData와 서버 검증을 연결할 수 있다.
- 서버 시점에서 조준 지점을 Trace하고, 생성 위치에서 그 지점을 향한 방향에 투척 각도를 더한다.
- 생성 위치까지 Sphere Sweep이 막히면 비용을 소모하지 않고 종료한다. 수류탄 충돌 설정과 Visibility Trace 응답을 함께 확인한다.
- Throw Socket Name이 있으면 Character Mesh의 소켓 위치를 사용한다. Dedicated Server에서 소켓을 사용할 때는 해당 메시의 서버 포즈 갱신 정책도 확인한다. 기본값은 소켓 없이 눈 위치 기준 오프셋을 사용한다.
- 폭발은 Pawn/WorldDynamic/PhysicsBody 오브젝트를 검색한다. 다른 오브젝트 채널의 피해 대상은 검색 채널 확장이 필요하다.
- 한 ASC에는 폭발당 GE를 한 번만 적용한다. 반경 안에서는 같은 기본 피해량을 사용하며 거리 감쇠는 없다.
- Require Line Of Sight가 켜져 있으면 Visibility(변경 가능) Trace로 가려진 대상을 제외한다.
- GA에서 생성한 EffectSpec과 시전자 Context를 액터가 보관하므로, 투척 GA가 종료돼도 폭발 피해 처리가 가능하다.
- 서버에서만 피해를 적용하며 Multicast는 폭발 연출만 수행한다.

## 사용자 빌드 후 확인할 항목

- 서로 다른 클라이언트에서 동일한 수류탄 이동/폭발이 보이는지.
- 바닥/벽에 바운스하고 지정 시간이 지나면 폭발하는지.
- 범위 안/밖, 벽 뒤, 여러 충돌 컴포넌트를 가진 대상의 피해가 올바른지.
- 아군/자기 자신/사망 대상 처리가 팀의 규칙과 일치하는지.
- 준비 중 취소와 비용 부족 시 생성/소모가 발생하지 않는지.
- 투척 GA 종료 후에도 수류탄이 남아 폭발하며 피해 출처가 유지되는지.

이번 변경에서는 빌드 및 에디터 실행 검증을 수행하지 않았다.

## 예상 궤적 컴포넌트

- `NF_ProjectileTrajectoryComponent`는 생성 위치/초기 속도/충돌 반경/중력/예측 시간을 입력받아 첫 충돌까지의 경로를 표시한다. GA나 피해 GE에 의존하지 않는다.
- 테스트 컴포넌트는 소유 Pawn에 기존 궤적 컴포넌트가 있으면 사용하고, 없으면 로컬에서 자동 생성한다. 별도 컴포넌트 추가 없이 테스트 가능하다.
- 표시 색/선 두께/착지 마커 크기를 편집하려면 캐릭터 BP에 이 궤적 컴포넌트를 미리 추가하고 디테일에서 설정한다.
- 수류탄 GA의 `CalculateThrowParameters`를 표시와 실제 서버 투척이 공유한다. 중력, 충돌 반경과 예측 시간은 설정한 수류탄 액터 클래스의 기본값에서 가져온다.
- 경로는 로컬 플레이어에게만 그리는 한 프레임 디버그 선이다. PIE/개발 테스트용이며 Shipping 제품 표시가 필요하면 Spline/Niagara 등으로 교체한다.
- 첫 충돌 위치는 바운스 이후 최종 폭발 위치가 아니다. 바운스 후 경로는 이번 범위에 포함하지 않는다.
- 로컬 표시와 서버 투척의 계산식은 같지만 네트워크 지연, 움직이는 장애물, 서버 소켓 포즈, Cast Time 동안의 이동으로 결과가 달라질 수 있다. 표시는 서버 판정을 보장하지 않는다.
- E 입력이 취소되거나 Possess가 해제되면 표시를 종료하고 투척하지 않는다. 정상적으로 E를 떼면 표시를 종료하고 투척 요청을 보낸다.
- Output Log의 `[SkillTest]`로 입력/요청을, `[Grenade]`로 서버의 생성 위치 차단/Commit 실패/생성 완료를 확인할 수 있다.

## 재사용 범위 장판 컴포넌트

- `NF_AreaIndicatorComponent`는 `ShowArea(Location, Radius, SurfaceNormal)`과 `HideArea()`로 원형 범위를 표시한다. 입력값은 월드 위치, cm 단위 반경, 표면 법선이다.
- 수류탄 GA, 피해 GE, 궤적 계산과 독립적이다. 폭격 경고, 지점 선택 스킬 등에서 같은 API를 호출할 수 있다. 피해 판정 기능은 포함하지 않는다.
- 수류탄 테스트 중에는 궤적의 `GetPredictedImpact`에서 받은 첫 충돌 위치에 폭발 반경 크기로 표시한다. 예측 충돌이 없거나 생성 경로가 막히면 숨긴다.
- 테스트 컴포넌트는 기존 장판 컴포넌트를 찾아 사용하고, 없으면 로컬에서 자동 생성한다. 설정을 편집하려면 캐릭터 BP에 `NF Area Indicator Component`를 미리 추가한다.
- 기본값은 머티리얼 없이 보이는 원형 디버그 면과 테두리다. 접촉 지점의 평면 위에 표시하며 복잡한 지형의 굴곡을 따라가지 않는다. 디버그 모드는 PIE/개발 테스트용이다.
- 제품용 표시는 `Decal Material`에 원형 마스크를 가진 Deferred Decal 머티리얼을 지정한다. 투명 영역과 원형 마스크는 머티리얼에서 작성하며, 별도 에셋은 이번 변경에서 생성하지 않았다. 반경은 Decal Size에 적용한다.
- 위치/법선을 통해 표면 방향에 맞추며 Surface Offset으로 겹침을 줄인다. Projection Depth는 데칼 투영 깊이이고 피해 범위와 무관하다.
- E 해제/입력 취소/Possess 해제 시 궤적과 장판을 모두 숨긴다.
- 컴포넌트는 표시 전용으로 복제하지 않으며 Dedicated Server에서는 그리지 않는다. 모든 플레이어에게 보여야 하는 보스 경고 등은 서버 확정 위치/반경을 GameplayCue 또는 복제 액터로 전달하고 각 클라이언트에서 ShowArea를 호출한다.
- 현재 장판은 첫 충돌 위치의 참고 범위다. 수류탄이 바운스하면 실제 폭발 위치가 달라지고, 벽에 가려진 피해 제외도 표시 면에 반영되지 않는다.
