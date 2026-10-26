# HUD 연동 확인 요청 — 팀원 체력·실드 복제

작성일: 2026-10-26
대상: 명현님(플레이어·PlayerState·ASC), 지우님(GAS·GE·피해·회복·사망 처리 협의)

## 목적

다른 팀원의 체력·최대 체력·실드·최대 실드를 로컬 클라이언트의 팀원 HUD에 표시해야 합니다. 기존 복제 경로가 이 용도로 동작하는지 확인 요청드립니다.

## 현재 확인한 구조

- `ANF_PlayerState`는 ASC와 `UNF_PlayerStatusAttribute`를 소유하고, `GetAbilitySystemComponent()`로 ASC를 제공합니다.
- ASC에는 `SetIsReplicated(true)`와 `EGameplayEffectReplicationMode::Mixed`가 설정되어 있습니다.
- `Health`, `MaxHealth`, `Shield`, `MaxShield`는 이미 `COND_None`, `REPNOTIFY_Always`로 복제 등록되어 있습니다.
- 각 Attribute의 OnRep에서 `GAMEPLAYATTRIBUTE_REPNOTIFY`를 호출합니다.
- `UNF_TeamMemberViewModel`은 해당 팀원의 PlayerState에서 ASC를 가져오고, 기존 GAS ViewModel을 통해 네 Attribute 변경을 구독합니다.

작성일의 C++ 소스 기준입니다. 이미 복제 코드가 있으므로 새 시스템을 중복 구현해달라는 요청이 아닙니다. Mixed 설정만으로 다른 플레이어의 Attribute가 수신되지 않는다고 단정할 수 없으며, 실제 클라이언트 수신 여부를 확인해야 합니다.

## 명현님 확인 사항

1. 클라이언트에서 다른 팀원의 PlayerState, ASC, AttributeSet에 접근할 수 있는지 확인해주세요.
2. 본인 Pawn이 아닌 다른 팀원의 ASC에서도 네 Attribute 현재값을 조회할 수 있는지 확인해주세요.
3. ASC·AttributeSet이 늦게 준비되는 경우 준비 완료를 구독할 수 있는 기존 이벤트나 재연결 시점을 알려주세요.
4. 리스폰 시 PlayerState·ASC를 유지하는지 교체하는지, UI가 다시 연결해야 하는 대상을 알려주세요.
5. Pawn이 멀리 있거나 사망 후 제거되어도 팀원 HUD용 상태 조회가 유지되는지 확인해주세요.

## 지우님 확인 사항

1. 서버에서 피해·회복·실드 감소·회복이 적용된 뒤 다른 팀원의 클라이언트에도 최종 Attribute 값이 전달되는지 확인해주세요.
2. 수신한 클라이언트에서 `GetGameplayAttributeValueChangeDelegate()`의 변경 알림이 발생하는지 확인해주세요.
3. 최대 체력·최대 실드 변경도 현재값과 함께 관찰 가능한지 확인해주세요.
4. 사망·부활·리스폰 시 네 Attribute의 초기화 및 유지 규칙을 알려주세요.

기존 경로로 정상 동작한다면 확인 결과와 조회·준비 시점만 공유해주세요. 누락이 재현된다면 관련 담당 파트에서 원인을 확인하고 필요한 범위만 보완해주세요.

## UI 담당 범위

UI가 팀원별 ASC 변경 알림을 구독·해제하고, 현재값 조회와 체력·실드 비율 계산 및 WBP 바인딩을 담당합니다. ASC가 교체되거나 준비 완료 알림이 도착하면 해당 팀원의 ViewModel 연결을 갱신합니다.

레벨은 별도의 진행 데이터 요청 문서에서 다룹니다. 이번 요청은 체력·실드 네 Attribute에 한정합니다. GE나 AttributeSet에서 Widget 또는 ViewModel을 직접 호출할 필요는 없습니다.

## 전달받고 싶은 내용

- 다른 클라이언트에서 팀원의 Attribute 조회와 변경 알림이 가능한지 확인 결과
- ASC·AttributeSet 준비 시점과 알림 API
- 리스폰·사망·부활 시 객체 유지 여부 및 수치 초기화 규칙
- 문제가 있다면 재현 조건과 보완 담당

## 완료 확인

- [ ] 데디케이티드 서버에 서로 다른 클라이언트 A·B로 접속합니다.
- [ ] A가 같은 팀 B의 체력·실드 현재값과 최대값을 조회할 수 있습니다.
- [ ] B의 피해·회복·실드 변화가 A의 ASC Attribute 변경 구독에 전달됩니다.
- [ ] 최대값 변경과 HUD 지연 생성 시에도 올바른 현재값을 조회할 수 있습니다.
- [ ] 사망·부활·리스폰 및 멀리 떨어진 팀원의 상태 조회를 확인합니다.
