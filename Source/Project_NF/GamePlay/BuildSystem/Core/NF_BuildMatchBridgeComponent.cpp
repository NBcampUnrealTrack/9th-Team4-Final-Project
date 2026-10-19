#include "GamePlay/BuildSystem/Core/NF_BuildMatchBridgeComponent.h"

#include "Core/Component/NF_BuildReadyComponent.h"
#include "Core/NF_GameMode.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"
#include "Player/State/NF_PlayerState.h"

UNF_BuildMatchBridgeComponent::UNF_BuildMatchBridgeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNF_BuildMatchBridgeComponent::BeginPlay()
{
	Super::BeginPlay();
	ANF_GameMode* GameMode = Cast<ANF_GameMode>(GetOwner());
	if (!IsValid(GameMode) || !GameMode->HasAuthority() || GetWorld()->GetAuthGameMode() != GameMode)
	{
		return;
	}
	UNF_BuildReadyComponent* Ready = GameMode->GetBuildReadyComponent();
	if (!IsValid(Ready) || Ready->ApplyFallbackBuild.IsBound())
	{
		UE_LOG(LogTemp, Error, TEXT("기본 빌드 연동 실패: Core 컴포넌트가 없거나 다른 객체가 이미 바인딩했습니다."));
		return;
	}
	Ready->ApplyFallbackBuild.BindUObject(this, &ThisClass::HandleApplyFallbackBuild);
	BoundReadyComponent = Ready;
}

bool UNF_BuildMatchBridgeComponent::HandleApplyFallbackBuild(ANF_PlayerState* PlayerState)
{
	bool bSucceeded = false;
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority() &&
		IsValid(PlayerState) && PlayerState->HasAuthority() && PlayerState->GetWorld() == GetWorld())
	{
		APawn* Pawn = PlayerState->GetPawn();
		if (IsValid(Pawn) && Pawn->GetPlayerState() == PlayerState)
		{
			if (UNF_BuildComponent* Build = Pawn->FindComponentByClass<UNF_BuildComponent>())
			{
				bSucceeded = Build->InitializeAbilitySystem(PlayerState->GetAbilitySystemComponent()) &&
					Build->ApplyFallbackBuildForMatchStart();
			}
		}
	}
	if (!bSucceeded)
	{
		UE_LOG(LogTemp, Warning, TEXT("기본 빌드 적용 실패: %s. Pawn, ASC, Rules와 기본 구성을 확인하세요."),
			*GetNameSafe(PlayerState));
	}
	OnFallbackBuildCompleted.Broadcast(PlayerState, bSucceeded);
	return bSucceeded;
}

void UNF_BuildMatchBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoundReadyComponent.IsValid() && BoundReadyComponent->ApplyFallbackBuild.IsBoundToObject(this))
	{
		BoundReadyComponent->ApplyFallbackBuild.Unbind();
	}
	BoundReadyComponent.Reset();
	Super::EndPlay(EndPlayReason);
}
