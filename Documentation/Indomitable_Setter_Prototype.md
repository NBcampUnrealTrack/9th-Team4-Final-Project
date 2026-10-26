# 불굴: GE 적용 전 차단

작성: 이정준 (구현: Codex), 2026-10-26

이 문서는 이전 Health Setter 실험을 대체한다. 파일 이름은 기존 링크 호환을 위해 유지했다.

## 현재 방식

`NFGA_Indomitable`은 장착 중 체력을 가진 ASC의 `GameplayEffectApplicationQueries`에 검사를 등록한다.
엔진이 GE를 실행하기 전에 검사에서 false를 반환하면 해당 GE 전체의 적용을 거부한다.
플레이어/무기/공통 피해 계산 코드는 수정하지 않았다. 기존 `DamagedCheck` 구독과 체력 Setter 호출은 제거했다.

1. 비치명 피해: 통과.
2. 치명 피해 + 쿨타임 완료: GE 거부, 기존 체력/쉴드 유지, 보호 시간 및 쿨타임 시작.
3. 보호 시간 중: 같은 지원 경로의 피해 GE를 거부. 쉴드만 감소할 공격도 포함.
4. 보호 종료 후 쿨타임 중: 피해 정상 적용.
5. 해제/아바타 변경: 자기 검사, 연결 재시도 및 연출 타이머, 지속 Cue 제거.

## 피해 예측과 지원 범위

지원 대상은 `NFGE_CommonDamage` 또는 그 자식 중 아래 구성이 유지된 GE이다.

- Instant / 주기 없음 / 스택 수 1.
- 직접 Modifiers 없음.
- Execution이 정확히 하나이고 계산 클래스가 네이티브 `NFExecCalc_CommonDamage`.

검사 시 Spec을 복사하고 대상의 Attribute와 태그를 캡처한 뒤 기존 계산기를 실행해
출력 Modifier만 읽는다. 결과를 실제 Attribute에 적용하지 않으며 임의의 다른 계산기를 실행하지 않는다.
따라서 방어력/쉴드/받는 피해 감소율은 기존 계산을 사용하고 별도 복제 수식을 만들지 않는다.
장비 부위 방어력처럼 Spec 생성 전에 반영된 피해량도 그 Spec 그대로 예측한다.

GE **전체**를 거부하므로 첫 치명 공격의 체력 피해뿐 아니라 해당 GE의 쉴드 피해·연출·부가 효과도 적용되지 않는다.
무기 명중 디버그 로그는 GE 거부 여부와 무관하게 출력될 수 있다.
다른 GE 클래스, 직접 Health Setter, 이미 적용된 주기 GE의 반복 피해, 강제 사망은 이 검사로 막지 않는다.
CommonDamage 자식에 다른 계산기/Modifier/주기 등을 추가하면 지원 대상에서 제외한다.
GE 적용 전 전역 훅이나 다른 커스텀 시스템이 이후 Spec을 바꾸는 경우 예측과 실제 결과가 달라질 수 있다.
이 검사는 엔진의 GE CanApply 판정보다 먼저 실행되므로 다른 면역 등으로 어차피 거부될 공격에도 쿨타임을 소비할 수 있다.

## BP 설정

- 기존 `NFGA_Indomitable` 부모 BP를 그대로 사용한다. Native 클래스 이름은 바꾸지 않았다.
- Passive / OnGranted, `Activate Passive On Grant`는 false.
- `Protection Duration` 기본 3초, `Cooldown Duration` 기본 120초.
- `HealthBuffer`는 더 이상 사용하지 않는다. 기존 직렬화 호환용 Deprecated 필드만 유지한다.
- Passive Event Tags나 Ability Triggers를 별도로 추가하지 않는다.
- Cooldown Gameplay Effect Class는 사용하지 않는다. 내부 서버 시간으로 관리한다.
- Activation Cue Tag / Active Cue Tag에 직접 만든 Cue를 지정할 수 있다.
  ASC가 검사 배열을 순회하는 동안 BP/Cue가 구독을 변경하지 않도록 연출은 다음 틱에 실행한다.
- 같은 인스턴스의 아바타 변경에는 쿨타임 유지. 제거 후 새로 부여하면 쿨타임 초기화.

## 초기화 및 로그

정식 PlayerASC 초기화가 늦으면 0.1초 간격 최대 50회 연결을 재시도한다.
NF_SkillTest에 부여된 경우에도 캐릭터의 체력 ASC를 별도로 찾아 차단 검사를 등록한다.

- `[Indomitable][연결 대기]`: 초기화 실패 사유와 ASC/Avatar/체력 등록 정보.
- `[Indomitable][연결 성공]`: GE 적용 전 검사 등록 완료.
- `[Indomitable][불굴 발동]`: 체력, 예상 체력/쉴드 피해, 보호 시간, 쿨타임, GE 거부.
- `[Indomitable][보호 중 차단]`: 보호 중 후속 공격 GE 거부.

## 검증 상태

`NF.Skills.Indomitable.ApplicationQuery` 테스트 소스에 실제 CommonDamage GE 적용을 통한
방어력/피해 감소/쉴드 예측, 첫 치명 공격, 후속 공격, 회복 허용, 보호 만료/쿨타임,
중복 등록 방지, 해제 시 자기 검사만 제거하는 검증을 추가했다.
빌드/자동화 테스트 실행/PIE 확인은 수행하지 않았다.
컴파일 후 우선 체력 10, 쉴드 0에서 피해 10을 주고 체력 10 유지 및 사망 GA 미실행을 확인한다.
