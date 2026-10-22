#include "GamePlay/EscapeKey/NF_EscapeKey.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/Component/NF_EscapeKeyComponent.h"
#include "GameFramework/PlayerController.h"
#include "Player/State/NF_PlayerState.h"

ANF_EscapeKey::ANF_EscapeKey()
{
	//디폴트신루트 이하에 콜리전과 메쉬 넣기로 바꾸기
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));

	SetRootComponent(Collision);

	Collision->InitSphereRadius(25.0f);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));

	VisualMesh->SetupAttachment(Collision);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ANF_EscapeKey::InitializePickup(UNF_EscapeKeyComponent* InKeyComponent)
{
	if (!HasAuthority() || !IsValid(InKeyComponent))return;

	KeyComponent = InKeyComponent;
	bPickupEnabled = false;
}

void ANF_EscapeKey::ActivatePickup()
{
	if (!HasAuthority() || !KeyComponent.IsValid())return;

	bPickupEnabled = true;
	ForceNetUpdate();
}

void ANF_EscapeKey::NF_Interaction(const APlayerController* InteractedController)
{
	if (!HasAuthority() ||!bPickupEnabled ||!IsValid(InteractedController) ||!KeyComponent.IsValid())return;

	ANF_PlayerState* PlayerState =InteractedController->GetPlayerState<ANF_PlayerState>();

	KeyComponent->TryPickup(PlayerState, this);
}

bool ANF_EscapeKey::IsPickupEnabled() const
{
	return bPickupEnabled;
}
