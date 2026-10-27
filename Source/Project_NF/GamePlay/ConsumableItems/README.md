# 소모품 슬롯과 체력 물약

작성: 이정준 (구현: Codex), 2026-10-27

## 구성

- NF_ConsumableComponent: 5개 고정 슬롯(0~4), 서버 사용 요청 및 수량 관리.
- FNF_ConsumableSlot: ItemData(원본 DA 참조), Quantity(수량)만 저장.
- NF_ConsumableItemDataAsset: ItemId, DisplayName, Description, Icon, ItemClass 원본.
- NF_ConsumableItem: 아이템 효과의 공통 부모. TryUse(User)가 즉시 성공/실패를 반환한다.
- NF_HealthPotion: Health를 GE로 즉시 회복. 기본 HealAmount=30, 자식 BP에서 조절.
- NFGE_HealthPotion: Health에 Data.Consumable.HealthRestore 값을 AddBase로 더하는 Instant GE.
- NF_ConsumablePickup: 메시/충돌/DA/수량을 가진 월드 픽업. 기존 NF_InteractionInterface 구현.
- NF_UseItemInterface: 컴포넌트에 NF_UseItem(SlotIndex=0)을 호출하는 입력 인터페이스.

플레이어/컨트롤러/빌드/UI 코드는 수정하지 않았다. 월드 픽업은 기존 NF_Interaction 경로에 연결했다. F키 바인딩은 추가하지 않았다.

## 에셋 설정 및 테스트

컴파일 후:

1. NF_HealthPotion을 부모로 BP_HealthPotion을 만들고 Heal Amount를 설정한다.
   기본값 30을 사용할 경우 DA의 ItemClass에 네이티브 NF_HealthPotion을 직접 지정해도 된다.
2. NF_ConsumableItemDataAsset으로 DA_HealthPotion 생성.
   ItemId=HealthPotion, DisplayName=체력 물약, Description=체력을 회복합니다,
   Icon=사용할 Texture2D, ItemClass=BP_HealthPotion으로 지정한다.
3. 캐릭터 BP에 NF Consumable Component 추가.
4. 서버에서 SetSlot(0, DA_HealthPotion, 3)으로 테스트 수량 설정.
5. 입력 담당이 캐릭터의 이 컴포넌트를 찾아 NF_UseItem 인터페이스를 호출한다.

실제 BP/DA/아이콘 이미지 에셋은 생성하지 않았다. 네이티브 클래스와 설정 경로만 추가했다.

## 입력 담당 전달 함수

```cpp
if (auto* Items = Character->FindComponentByClass<UNF_ConsumableComponent>())
{
    INF_UseItemInterface::Execute_NF_UseItem(Items, 0);
}
```

BP에서는 소모품 컴포넌트 참조에 NF Use Item(인터페이스 메시지)을 호출한다.
SlotIndex는 0~4이며, 기본값 0은 첫 칸이다. 선택 슬롯을 저장하거나 자동으로 다음 물약을 찾는 기능은 없다.
클라이언트의 로컬 Pawn에서 호출하면 Reliable 서버 RPC로 전달한다. NF_UseItem은 요청 함수라 성공값을 반환하지 않는다.
성공 여부에 따른 표시 갱신은 슬롯 복제와 OnSlotsChanged를 사용한다.
서버 코드에서 즉시 결과가 필요하면 TryUseItem(SlotIndex)의 bool을 사용한다.

## 회복/차감 규칙

- 서버가 보관 중인 슬롯 DA의 ItemClass를 읽는다. 클라이언트가 회복량/클래스/수량을 전달하지 않는다.
- NF_ConsumableItem 자식 클래스만 허용하며 서버에서 일시적 액터를 생성한다.
- TryUse(User)를 실행하고 액터는 제거한다. 효과 로직은 BeginPlay가 아니라 TryUse 안에 구현한다.
  이 규약은 즉시 사용용이다. 홀딩/사용 시간/취소/효과 액터 유지가 필요한 아이템은 별도 설계가 필요하다.
- 체력 물약은 유효한 플레이어 Health ASC, 체력 >0, 사망/기절 태그 없음, 현재 체력 < 최대 체력을 요구한다.
- 회복량은 min(HealAmount, MaxHealth - Health)이며 기존 Attribute의 최대 체력 보정도 유지한다.
- GE가 적용되고 실제 Health가 증가했을 때만 true로 간주하여 1개 차감한다.
- 가득 찬 체력/사망/기절/ASC 없음/잘못된 클래스/회복 GE 거부 시 수량을 유지한다.
- 사용 중에는 중복 사용·슬롯 수정 요청을 막아 같은 수량의 중복 소비를 방지한다.
- 마지막 1개를 쓰면 DA 참조도 비운다. 쉴드는 회복하지 않는다.

## 슬롯 API와 UI 조회

GetSlotCount / GetSlots / GetSlot / SetSlot / RemoveFromSlot / ClearSlot / OnSlotsChanged는 유지한다.
TryAddItem(ItemData, Quantity)을 추가했다. 같은 DA는 기존 슬롯에 합치고 없으면 첫 빈칸을 사용한다.
모든 슬롯이 찼거나 int32 수량 범위를 초과하면 전량 실패하며 이전 슬롯은 그대로 유지한다.
사용자가 정한 별도 최대 중첩 수는 아직 없다. 다른 DA는 같은 ItemId여도 자동 합치지 않는다.
SetSlot은 명시적 서버 상태 설정용이며 자동 획득/중첩/교체 정책이 아니다.
OwnerOnly로 슬롯을 복제하므로 소유 캐릭터의 네트워크 복제와 소유 연결이 설정돼 있어야 한다.

아이콘 원본은 DA 한 곳에만 저장한다. UI는 최초 GetSlots 조회 및 OnSlotsChanged 구독 후:

```text
Slot.ItemData.DisplayName / Description / Icon
Slot.ItemData.GetItemIcon()  // BlueprintPure 아이콘 Getter
Slot.Quantity
```

를 읽으면 된다. 이름/설명/아이콘과 DA Getter는 Blueprint에서 읽을 수 있다.
컴포넌트가 ViewModel을 직접 호출하지 않으며 실제 VM 연결/빈 슬롯 아이콘 초기화는 UI 담당 범위다.
원본 DA/ItemClass/아이콘은 클라이언트 및 패키지에서 사용할 수 있도록 정적 참조를 설정해야 한다.

## 검증 상태

NF.Consumables.Slots.Storage: 기본 슬롯 경계/수량 검증.
NF.Consumables.HealthPotion.UseAndCharge: 실제 회복 GE, 최대 체력 제한,
인터페이스 사용 요청, 성공 시 1개 차감, 쉴드 유지, GE 거부/최대체력/사망/기절/잘못된 클래스에서 수량 유지,
마지막 수량 소진, DA 아이콘 Getter 조회를 검증하는 테스트 소스를 추가했다.
빌드/자동화 테스트 실행/PIE 네트워크 복제 및 H키 테스트는 수행하지 않았다.

## 월드 픽업 BP 설정

1. NF_ConsumablePickup을 부모로 BP_HealthPotionPickup을 생성한다.
2. VisualMesh에 물약 StaticMesh 지정. 메시는 충돌을 끄고 Sphere Collision을 검출/물리에 사용한다.
3. Item Data에 DA_HealthPotion, Quantity에 드랍 수량 지정.
4. 캐릭터 BP에 NF Consumable Component가 붙어 있는지 확인.
5. 레벨에 배치하거나 서버에서 Spawn Actor로 생성한 뒤 기존 F 상호작용을 사용한다.

DA.ItemClass에는 NF_HealthPotion(사용 효과)을 지정한다. NF_ConsumablePickup을 넣지 않는다.
드랍 액터를 런타임에 생성한다면 Spawn Actor의 ItemData/Quantity 핀 또는 서버 InitializePickup(DA, 수량)을 사용한다.
여기서는 드랍 시스템/몬스터 사망 코드에서 자동으로 Spawn하도록 연결하지 않는다.
메시는 임의 에셋을 자동 지정하지 않았으므로 BP에서 직접 지정해야 한다.

픽업 성공은 전량 획득 후 월드 액터 제거 방식이다. 전량 보관에 실패하면 월드 액터를 유지한다.
서버가 Controller/Pawn 소유 관계, 거리(기본 250cm), 눈 위치~아이템의 Visibility 가림,
기절/사망 상태 및 체력(Health ASC가 있을 경우)을 검사한다.
동일 액터의 중복 획득은 가드와 소진 수량/파괴 상태로 차단한다.
캐릭터 쪽 F 라인트레이스보다 서버 허용 거리가 짧으면 줍기에 실패할 수 있으므로 Max Pickup Distance를 맞춘다.
Sphere Collision은 기본 Visibility를 Block한다. 프로젝트 상호작용이 다른 채널이면 BP에서 그 채널도 Block해야 한다.

Simulate Drop Physics는 기본 false다. true이면 구체 물리로 떨어지고 Actor Movement 복제를 사용한다.
이 옵션과 상관없이 DA/수량/파괴는 서버가 관리한다. 실제 물리 드랍 및 네트워크 플레이는 별도 확인이 필요하다.

NF.Consumables.Pickup.InteractionAndStorage 테스트 소스에 기존 상호작용 진입,
슬롯 보관/중첩, 공간 부족 시 월드 아이템 유지, 거리/가림 검사, 중복 획득/수량 오버플로 방지를 추가했다.
이 작업에서 빌드/테스트 실행/PIE 플레이는 수행하지 않았다.
