#include "GamePlay/ConsumableItems/NF_ConsumablePickup.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GamePlay/ConsumableItems/Data/NF_ConsumableItemDataAsset.h"
#include "GamePlay/ConsumableItems/NF_ConsumableComponent.h"
#include "GAS/Tags/NFTag_LifeState.h"
#include "GAS/Tags/NFTag_Player.h"
#include "Net/UnrealNetwork.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"
#include "Project_NF.h"

ANF_ConsumablePickup::ANF_ConsumablePickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(35.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Collision->SetGenerateOverlapEvents(false);
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(Collision);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ANF_ConsumablePickup::BeginPlay()
{
	Super::BeginPlay();
	Collision->SetSimulatePhysics(bSimulateDropPhysics);
}

void ANF_ConsumablePickup::NF_Interaction(const APlayerController* InteractedController)
{
	UE_LOG(LogProject_NF, Warning, TEXT("[ConsumablePickup][상호작용 도착] 픽업=%s | 컨트롤러=%s | 서버=%d"),
		*GetName(), *GetNameSafe(InteractedController), HasAuthority());
	TryPickup(InteractedController);
}

bool ANF_ConsumablePickup::InitializePickup(UNF_ConsumableItemDataAsset* InItemData, int32 InQuantity)
{
	if (!HasAuthority() || bPickingUp || IsActorBeingDestroyed() || !IsValid(InItemData) || InQuantity <= 0) { return false; }
	ItemData = InItemData;
	Quantity = InQuantity;
	ForceNetUpdate();
	return true;
}

bool ANF_ConsumablePickup::TryPickup(const APlayerController* InteractedController)
{
	const auto Reject = [this](const TCHAR* Reason)
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[ConsumablePickup][획득 거절] 픽업=%s | 사유=%s | DA=%s | 수량=%d"),
			*GetName(), Reason, *GetNameSafe(ItemData.Get()), Quantity);
		return false;
	};
	if (!HasAuthority() || bPickingUp || IsActorBeingDestroyed() || !GetWorld()
		|| !IsValid(InteractedController) || !InteractedController->HasAuthority()
		|| InteractedController->GetWorld() != GetWorld() || !IsValid(ItemData) || Quantity <= 0
		|| !FMath::IsFinite(MaxPickupDistance) || MaxPickupDistance <= 0.f)
	{
		return Reject(TEXT("서버/컨트롤러/DA/수량/거리설정 확인 필요 또는 이미 획득 중"));
	}
	APawn* Pawn = InteractedController->GetPawn();
	if (!IsValid(Pawn) || Pawn->IsActorBeingDestroyed() || Pawn->GetController() != InteractedController
		|| Pawn->GetWorld() != GetWorld()) { return Reject(TEXT("컨트롤러가 조종하는 유효한 캐릭터 없음")); }
	const FVector PawnLocation = Pawn->GetActorLocation();
	const FVector PickupLocation = GetActorLocation();
	if (PawnLocation.ContainsNaN() || PickupLocation.ContainsNaN()
		|| FVector::DistSquared(PawnLocation, PickupLocation) > FMath::Square(static_cast<double>(MaxPickupDistance)))
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[ConsumablePickup][거리 초과] 현재=%.1fcm | 허용=%.1fcm"),
			FVector::Dist(PawnLocation, PickupLocation), MaxPickupDistance);
		return Reject(TEXT("획득 거리 초과 또는 위치 오류"));
	}
	auto* Inventory = Pawn->FindComponentByClass<UNF_ConsumableComponent>();
	if (!IsValid(Inventory)) { return Reject(TEXT("캐릭터에 NF_ConsumableComponent가 없음")); }
	if (auto* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn, true))
	{
		if (ASC->HasMatchingGameplayTag(NFTag_Player::Player_State_Death)
			|| ASC->HasMatchingGameplayTag(NFTag_LifeState::State_Life_Downed)
			|| ASC->HasMatchingGameplayTag(NFTag_LifeState::State_Life_Dead)) { return Reject(TEXT("사망 또는 기절 상태")); }
		if (ASC->GetSet<UNF_PlayerStatusAttribute>()
			&& ASC->GetNumericAttribute(UNF_PlayerStatusAttribute::GetHealthAttribute()) <= 0.f) { return Reject(TEXT("현재 체력이 0 이하")); }
	}
	if (bRequireLineOfSight)
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		Pawn->GetActorEyesViewPoint(EyeLocation, EyeRotation);
		if (EyeLocation.ContainsNaN()) { return Reject(TEXT("눈 위치 오류")); }
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ConsumablePickup), false, Pawn);
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, PickupLocation, ECC_Visibility, Params)
			&& Hit.GetActor() != this)
		{
			UE_LOG(LogProject_NF, Warning, TEXT("[ConsumablePickup][가림] 장애물=%s | 컴포넌트=%s"),
				*GetNameSafe(Hit.GetActor()), *GetNameSafe(Hit.GetComponent()));
			return Reject(TEXT("서버 시야 검사에서 다른 물체에 막힘"));
		}
	}

	TGuardValue<bool> PickupGuard(bPickingUp, true);
	const int32 PickedQuantity = Quantity;
	if (!Inventory->TryAddItem(ItemData.Get(), PickedQuantity))
	{
		UE_LOG(LogProject_NF, Log, TEXT("[ConsumablePickup] 보관 실패, 월드 아이템 유지 | 캐릭터=%s | 아이템=%s | 수량=%d"),
			*GetNameSafe(Pawn), *GetNameSafe(ItemData.Get()), PickedQuantity);
		return false;
	}
	Quantity = 0;
	SetActorEnableCollision(false);
	UE_LOG(LogProject_NF, Log, TEXT("[ConsumablePickup] 획득 성공 | 캐릭터=%s | 아이템=%s | 수량=%d"),
		*GetNameSafe(Pawn), *GetNameSafe(ItemData.Get()), PickedQuantity);
	Destroy();
	return true;
}

void ANF_ConsumablePickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANF_ConsumablePickup, ItemData);
	DOREPLIFETIME(ANF_ConsumablePickup, Quantity);
}
