# 프로젝트 지침

- 이 프로젝트는 Unreal Engine 5.7을 사용한다.
- 이 프로젝트는 데디케이티드 서버(Dedicated Server) 방식으로 구현한다.
- 클래스 네이밍 규칙으로 클래스 이름 앞에 `NF_`를 붙인다.
- 파스칼 명명법 사용
- 이 프로젝트는 8명이 개발하는 팀 프로젝트다.
- 프로젝트 전반에 Gameplay Ability System(GAS)을 적용하므로, 사용자의 질문에 답할 때 항상 GAS를 고려한다.
- 사용자에게는 항상 한국어로 답한다.
- 질문을 받았을 때 코드를 임의로 변경하지 않는다. 변경이 필요하다면 변경할 예시 코드를 설명과 함께 먼저 보여준다.
- 프로젝트 전반에 대한 참고가 필요할 경우, 최상위 폴더의 `ProjectNF_Game_Design_Document.md` 기획서를 참고한다.

## 팀원별 담당 파트

- 사용자: Core, 최종 탈출 관련 로직, UI
- 명현: 소모 아이템, 플레이어(근접 공격 등), 레벨 디자인 전체 베이스
- 경환: 몬스터 전체 베이스
- 명우: 매치메이킹 시스템, 보스
- 정준: 스킬 전체의 베이스
- 용준: 무기, 방어구
- 준현: 스킬 장비 빌드 교체 장소, 스킬 장비 빌드 시스템
- 지우: 게임 이펙트(Gameplay Effect, GE) 관리 총괄, 사망, 기절




## GamePlay Tag 매크로 선언 관련
- UE 매크로 선언과 정의 규칙은 다음과 같다.
- 도메인 마다 h/cpp를 따로 두고, 같은 태그를 두 파일에 정의하지 않는다.
- .h 에는 UE_DECLARE_GAMEPLAY_TAG_EXTERN(이름) 으로 선언한다.
- .cpp 에는 UE_DEFINE_GAMEPLAY_TAG_COMMENT(이름,"태그", "설명") 으로 정의한다.
- .h와 .cpp는 같은 namespace로 감싸고 namespace이름은 파일 이름과 같게 한다.
- .h 선언 앞에는 NF_API를 붙인다.
    ````
    예시
    //TestTag.h
    namespace TestTag
    {
    NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
    }

    //TestTag.cpp
    namespace TestTag
    {
    NF_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead,"State.Dead","사망 상태");
    }
    ````

- 태그 이름 규칙은 다음과 같다
- 용도.분류.이름 으로 작성한다.
1. 용도 : 태그를 어디에 쓰는지 이다. 맨앞에 둔다
예) Ability, State, Event, Input, Data, CoolDown,GameplayCue
2. 분류 : 부모 태그로 묶어서 검사할 일이 있을 때만 넣는다. 없을시 생략한다.
3. 이름: 구체적인 대상 Pascal Case로 작성
4. 만들 때 목적과 작성자를 남길것.
5. 태그의 목적은 설명란에, 작성자와 날짜는 주석에 적을 것
   ````예시)
   //TestTag.h
   namespace TestTag
   {// @owner : 강명우 | 2026-10-12
   NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Attacking);
   }
   
   //TestTag.cpp
   namespace TestTag
   {
   NF_API UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Attacking,"Character.State.Attacking","일반 공격 몽타주 재생");
   }
   ````
6. GIT을 통해 배포된 태그는 고치지않고 추가만 할것
7. 사용하지 않는 태그는 릴리즈 이후 한 번에 삭제

## 코드 컨벤션

<일반네이밍룰> 
- "모든 클래스의 맨 앞에는 프로젝트이니셜 넣기"
- NF_AbilitySystemComponent (O)
- PlayerController (X)
- NF_PlayerController (O)
- NF_ddAttack (O)

<GAS 관련네이밍룰>
- NFGA_
- NFGE_
- NFGC_
- NF_XXXXAttribute

