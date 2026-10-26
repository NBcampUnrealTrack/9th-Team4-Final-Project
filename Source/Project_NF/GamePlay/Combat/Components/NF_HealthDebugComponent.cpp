#include "GamePlay/Combat/Components/NF_HealthDebugComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"

UNF_HealthDebugComponent::UNF_HealthDebugComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UNF_HealthDebugComponent::BeginPlay()
{
	Super::BeginPlay();
#if UE_BUILD_SHIPPING
	SetComponentTickEnabled(false);
#else
	if (GetNetMode() == NM_DedicatedServer) { SetComponentTickEnabled(false); return; }
	RefreshDisplay();
#endif
}

void UNF_HealthDebugComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshDisplay();
}

uint64 UNF_HealthDebugComponent::GetMessageKey() const
{
	return 0x4E46000000000000ULL | static_cast<uint64>(GetUniqueID());
}

UAbilitySystemComponent* UNF_HealthDebugComponent::FindHealthASC() const
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner)) { return nullptr; }
	const auto IsHealthASC = [Owner](UAbilitySystemComponent* ASC)
	{
		return IsValid(ASC) && ASC->GetAvatarActor() == Owner && ASC->GetSet<UNF_PlayerStatusAttribute>();
	};
	auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (IsHealthASC(ASC)) { return ASC; }
	// PlayerState 소유 ASC도 지원한다. 초기화가 늦으면 다음 갱신에서 다시 찾는다.
	if (const APawn* Pawn = Cast<APawn>(Owner))
	{
		ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn->GetPlayerState());
		if (IsHealthASC(ASC)) { return ASC; }
	}
	return nullptr;
}

void UNF_HealthDebugComponent::RefreshDisplay()
{
#if !UE_BUILD_SHIPPING
	if (!bShowOnScreen) { RemoveDisplay(); return; }
	if (!GEngine || !GetWorld() || GetNetMode() == NM_DedicatedServer || !IsValid(GetOwner())) { return; }
	const FString Label = FString::Printf(TEXT("[%s][%s][%s]"), *GetWorld()->GetName(),
		GetOwner()->HasAuthority() ? TEXT("Server") : TEXT("Client"), *GetOwner()->GetName());
	FString Message;
	FColor Color = TextColor;
	if (auto* ASC = FindHealthASC())
	{
		Message = FString::Printf(TEXT("%s HP %.1f / %.1f | Shield %.1f / %.1f"), *Label,
			ASC->GetNumericAttribute(UNF_PlayerStatusAttribute::GetHealthAttribute()),
			ASC->GetNumericAttribute(UNF_PlayerStatusAttribute::GetMaxHealthAttribute()),
			ASC->GetNumericAttribute(UNF_PlayerStatusAttribute::GetShieldAttribute()),
			ASC->GetNumericAttribute(UNF_PlayerStatusAttribute::GetMaxShieldAttribute()));
	}
	else
	{
		Message = Label + TEXT(" Waiting for ASC / NF_PlayerStatusAttribute");
		Color = FColor::Yellow;
	}
	// 하나의 컴포넌트가 같은 메시지를 갱신하므로 매 Tick마다 줄이 쌓이지 않는다.
	GEngine->AddOnScreenDebugMessage(GetMessageKey(), 0.35f, Color, Message);
#endif
}

void UNF_HealthDebugComponent::RemoveDisplay()
{
#if !UE_BUILD_SHIPPING
	if (GEngine) { GEngine->RemoveOnScreenDebugMessage(GetMessageKey()); }
#endif
}

void UNF_HealthDebugComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveDisplay();
	Super::EndPlay(EndPlayReason);
}
