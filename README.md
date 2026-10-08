# Gameplay Tag, 폴더 구조 등 프로젝트 협업 규약

## 목차

0. [UE 매크로를 활용한 선언과 정의 방법](#0-ue-매크로를-활용한-선언과-정의-방법)

1. [태그 이름은 "용도.분류.이름"](#1-태그-이름은-용도분류이름)
2. [만들 때 목적과 작성자를 남긴다](#2-만들-때-목적과-작성자를-남긴다)
3. [배포된 태그는 고치지 않고 추가만 한다](#3-배포된-태그는-고치지-않고-추가만-한다)
4. [사용하지 않는 태그는 릴리즈 이후 한 번에 삭제](#4-사용하지-않는-태그는-릴리즈-이후-한-번에-삭제)
5. [폴더 구조](#5-폴더-구조)
 
## 0. UE 매크로를 활용한 선언과 정의 방법

1. 도메인마다 h/cpp를 따로 두고, 같은 태그를 두 파일에 정의하지 않는다.<br>

2. `.h`에는 `UE_DECLARE_GAMEPLAY_TAG_EXTERN(이름)`으로 **선언**한다.<br>
3. `.cpp`에는 `UE_DEFINE_GAMEPLAY_TAG_COMMENT(이름, "태그", "설명")`으로 **정의**한다.<br>
4. .h와 .cpp는 같은 namespace로 감싸고, namespace 이름은 파일 이름과 같게 한다.<br>
5. .h 선언 앞에는 `프로젝트명_API`(`해당 예시는 TEST_API`)를 붙인다.<br> 
(태그가 어느 프로젝트 소속인지 분류하고, 다른 모듈에서도 링크 에러 없이 안전하게 쓰기 위해)

```cpp
// TestTag.h
namespace TestTag
{
	TEST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
}
```

```cpp
// TestTag.cpp
namespace TestTag
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "사망 상태")
}
```

## 1. 태그 이름은 `용도.분류.이름`

1. `용도`: 태그를 어디에 쓰는지. 맨 앞에 둔다.<br>
  ➡️예: `Ability`, `State`, `Event`, `Input`, `Data`, `Cooldown`, `GameplayCue`
  <br><br>
2.  `분류`: 부모 태그로 묶어서 검사할 일이 있을 때만 넣는다. 없으면 생략한다.<br>
  ➡️예: `State.Debuff`로 검사하면 그 아래 모든 디버프가 한 번에 걸린다.
  <br><br>
3. `이름`: 구체적인 대상. **PascalCase** 영어로 쓴다.
<br>
<br>

추가 규칙

1. 주체(`Character` 등)는 꼭 구분이 필요할 때만 넣는다. 태그는 이미 그 액터에 붙어 있다.<br>

2. C++ 전용 사용 이름은 `.`을 `_`로 바꾼다.

```cpp
//용도.분류.이름
UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Debuff_Stun, "State.Debuff.Stun", "기절 상태")
//용도.이름 (분류 생략)
UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "사망 상태")
```

## 2. 만들 때 목적과 작성자를 남긴다

1. 내가 새로운 Tag를 만들기전에 내가 만들려는 Tag가 남이 만든 Tag에 있는지 확인한 뒤 생성한다.<br>

2. 나중에 보는 사람이 "왜 만들었고 누구에게 물어보면 되는지" 알 수 있게 하려는 규칙이다.<br>
3. 목적은 설명란에, 작성자와 날짜는 주석에 적는다.


```cpp
// TestTag.h
namespace TestTag
{
	// @Owner: 명현 | 2026-10-20
	TEST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Attacking);
}
```

```cpp
// TestTag.cpp
namespace TestTag
{
	// @Owner: 명현 | 2026-10-08
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Attacking, "Character.State.Attacking", "일반 공격 몽타주 재생 중")
}
```

## 3. 배포된 태그는 고치지 않고 추가만 한다

1. 머지 전 태그는 나만 쓰는 개발 중인 태그라서 자유롭게 고쳐도 된다.<br>
2. 머지된 태그는 다른 사람이 쓰고 있을 수 있는 공용 태그라서, 수정하거나 삭제하지 않고 새 태그를 추가한다.<br>

3. 대체된 옛 태그에는 담당자가 **"사용하지 않는 태그"**라고 표시해서, 다른 사람이 새로 쓰지 않게 한다.

```cpp
// TestTag.cpp
namespace TestTag
{
	// @Owner: 명현 | 2026-10-20
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_NormalAttacking, "Character.State.NormalAttacking", "일반 공격 몽타주 재생 중")

	// @Owner: 명현 | 2026-10-08
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Attacking, "Character.State.Attacking", "Owner가 사용하지 않는 태그로 삭제 예정.
		대신 Character.State.NormalAttacking 사용중")
}
```

## 4. 사용하지 않는 태그는 릴리즈 이후 한 번에 삭제

1. 릴리즈 이후 `사용하지 않는 태그` 는 모아서 한번에 검토 후 삭제한다.<br>

2. 삭제 전 에디터 `태그 매니저`의 `Search for References`로 에셋 참조가 없는지 확인한다.<br>
3. 어느 곳에도 `tag 사용이 없고` `파일에 선언 및 정의가 없다면` 해당 .h / .cpp는 함께 지운다. <br>
<br>

**주의사항**

- 에셋이 쓰던 태그를 지우면 컴파일 에러 없이 조용히 깨진다.<br>
- 태그 삭제는 에디터를 재시작해야 반영된다.

## 5. 폴더 구조
```
Source/
│  ├─ Core/
│  │  ├─ BuildSystem/
│  │  ├─ KeyAndEscape/
│  │  └─ Match/
│  │
│  ├─ Data/
│  │
│  ├─ GAS/
│  │  ├─ Abilities/
│  │  ├─ Attributes/
│  │  ├─ Foundation/
│  │  │  └─ NF_AbilitySystemComponent.h
│  │  ├─ Cue/
│  │  ├─ Effects/
│  │  ├─ Tags/
│  │  └─ Tasks/
│  │
│  ├─ GamePlay/
│  │  ├─ BuildShop/
│  │  │
│  │  ├─ Combat/
│  │  │  ├─ Armor/
│  │  │  │  ├─ Bottom/
│  │  │  │  ├─ Head/
│  │  │  │  └─ Upper/
│  │  │  │
│  │  │  ├─ Projectiles/
│  │  │  │
│  │  │  ├─ Skill/
│  │  │  │  ├─ Active/
│  │  │  │  │  ├─ Attack/
│  │  │  │  │  │  ├─ GA_AirStrike.cpp
│  │  │  │  │  │  ├─ GA_Grenade.cpp
│  │  │  │  │  │  ├─ GA_RollingBomb.cpp
│  │  │  │  │  │  └─ GA_SlayerMode.cpp
│  │  │  │  │  │
│  │  │  │  │  ├─ Movement/
│  │  │  │  │  │  ├─ GA_Acceleration.cpp
│  │  │  │  │  │  ├─ GA_Dash.cpp
│  │  │  │  │  │  ├─ GA_HighJump.cpp
│  │  │  │  │  │  └─ GA_Portal.cpp
│  │  │  │  │  │
│  │  │  │  │  ├─ Search/
│  │  │  │  │  │  ├─ GA_Clairvoyance.cpp
│  │  │  │  │  │  └─ GA_Sonar.cpp
│  │  │  │  │  │
│  │  │  │  │  └─ Utility/
│  │  │  │  │     ├─ GA_Barrier.cpp
│  │  │  │  │     ├─ GA_EMP.cpp
│  │  │  │  │     └─ GA_Grapple.cpp
│  │  │  │  │
│  │  │  │  └─ Passive/
│  │  │  │     ├─ Attack/
│  │  │  │     │  ├─ GA_AttackUp.cpp
│  │  │  │     │  ├─ GA_Berserker.cpp
│  │  │  │     │  ├─ GA_IncendiaryRound.cpp
│  │  │  │     │  ├─ GA_MagicBullet.cpp
│  │  │  │     │  └─ GA_ShieldBreaker.cpp
│  │  │  │     │
│  │  │  │     └─ Utility/
│  │  │  │        ├─ GA_DefenseUp.cpp
│  │  │  │        ├─ GA_DoubleJump.cpp
│  │  │  │        ├─ GA_HealthUp.cpp
│  │  │  │        ├─ GA_SpikeArmor.cpp
│  │  │  │        └─ GA_Undying.cpp
│  │  │  │
│  │  │  └─ Weapons/
│  │  │     ├─ AR/
│  │  │     ├─ Pistol/
│  │  │     ├─ SG/
│  │  │     ├─ SMG/
│  │  │     └─ SR/
│  │  │
│  │  ├─ ConsumableItems/
│  │  │
│  │  └─ Survival/
│  │     ├─ Death/
│  │     └─ Downed/
│  │
│  ├─ Monster/
│  │  └─ AI/
│  │
│  ├─ Player/
│  │
│  └─ UI/
```
```
Content/
│  ├─ Dev
│  ├─ Minwoong
│  ├─ Myunghyun
│  ├─ ...
│  ├─ ...
│  ├─ ...
│  ├─ ...
│  ├─ ...
```
