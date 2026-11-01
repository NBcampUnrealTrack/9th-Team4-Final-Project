# 빌드 규칙 DA 이행

C++ 컴파일과 Unreal Editor 재시작 후 Tools > Execute Python Script에서 `MigrateBuildShopRules.py`를 실행한다. 스크립트는 컴파일을 실행하지 않는다.

다음 두 상점 Blueprint의 Class Defaults > NF|BuildShop > Rules에 기존 DA_NF_BuildRules_Prototype을 연결하고 저장한다.

- `/Game/ProjectNF/Dev/GamePlay/BuildShop/BP_NF_BuildShopZone`
- `/Game/ProjectNF/Persnal/JunHyun/BP_NF_BuildShopZone`

이미 다른 Rules가 지정되어 있으면 중단한다. 레벨 인스턴스의 별도 오버라이드는 자동 변경하지 않는다. 구버전 네이티브 클래스에서는 새 속성이 없어 실행할 수 없다.

Rules 설정은 플레이어 BuildComponent에서 제거되었다. 기존 Blueprint의 ConfigureRules 호출이 있으면 제거하고 상점 기본값으로 옮긴다. 컴포넌트의 GetRules 조회는 현재 상점으로 전달되며 세션 밖에서는 nullptr다. 장착 항목만 별도로 플레이어에 복제하여 상점을 닫은 뒤 HUD를 유지한다.

개인별 초기 코스트·해금 ID는 BuildComponent에 유지한다. 공용 Catalog 전체를 자동 해금하지 않는다. 해금 ID는 상점 진입 전 지급할 수 있으며 Catalog 등록·원본 연결·코스트·슬롯·해금 여부는 서버 적용 시 검증한다.

기본 빌드는 해당 팀 거점의 Rules에서 가져온다. 같은 팀의 거점 상점은 같은 Rules를 사용해야 한다. 스킬·장비 담당 원본과 공용 ASC/GE 구현은 그대로 사용한다.

사용자 컴파일 후 NF.Build 테스트와 Dedicated Server 클라이언트 2명으로 개인별 목록, 상점 전환, 세션 종료 후 HUD, 기본 빌드, 복제를 확인한다.
