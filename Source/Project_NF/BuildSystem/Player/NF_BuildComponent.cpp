#include "BuildSystem/Player/NF_BuildComponent.h"

#include "AbilitySystemComponent.h"
#include "BuildSystem/Core/NF_BuildValidator.h"
#include "BuildSystem/Data/NF_BuildItemDefinition.h"
#include "BuildSystem/Data/NF_BuildRules.h"
#include "BuildSystem/GAS/Tags/NFTag_Build.h"
#include "BuildSystem/GamePlay/BuildShop/NF_BuildShopZone.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"

UNF_BuildComponent::UNF_BuildComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

bool UNF_BuildComponent::InitializeAbilitySystem(UAbilitySystemComponent* InAbilitySystem)
{
	if (!GetOwner()->HasAuthority() || bApplyingBuild || !IsValid(InAbilitySystem) ||
		InAbilitySystem->GetAvatarActor() != GetOwner() ||
		(!AppliedItems.IsEmpty() && AbilitySystem != InAbilitySystem))
	{
		return false;
	}
	AbilitySystem = InAbilitySystem;
	return true;
}

bool UNF_BuildComponent::ConfigureRules(UNF_BuildRules* InRules)
{
	if (!GetOwner()->HasAuthority() || bApplyingBuild || !AppliedItems.IsEmpty() || !IsValid(InRules))
	{
		return false;
	}
	Rules = InRules;
	GetOwner()->ForceNetUpdate();
	return true;
}

bool UNF_BuildComponent::SetPlayerLevel(int32 NewLevel)
{
	int32 CostLimit = 0;
	if (!GetOwner()->HasAuthority() || bApplyingBuild || !Rules || !Rules->TryGetCostLimit(NewLevel, CostLimit))
	{
		return false;
	}
	if (!CurrentBuild.IsEmpty() &&
		FNF_BuildValidator::Validate(Rules, NewLevel, CurrentBuild).Result != ENF_BuildResult::Success)
	{
		return false;
	}
	PlayerLevel = NewLevel;
	GetOwner()->ForceNetUpdate();
	return true;
}

FNF_BuildValidation UNF_BuildComponent::PreviewBuild(const TArray<FNF_BuildSelection>& Selections) const
{
	return FNF_BuildValidator::Validate(Rules, PlayerLevel, Selections);
}

void UNF_BuildComponent::RequestApplyBuild(const TArray<FNF_BuildSelection>& Selections, ANF_BuildShopZone* ShopZone)
{
	if (GetOwner()->HasAuthority())
	{
		OnBuildRequestCompleted.Broadcast(ApplyBuildOnServer(Selections, ShopZone));
	}
	else
	{
		ServerApplyBuild(Selections, ShopZone);
	}
}

void UNF_BuildComponent::ServerApplyBuild_Implementation(
	const TArray<FNF_BuildSelection>& Selections, ANF_BuildShopZone* ShopZone)
{
	ClientBuildRequestCompleted(ApplyBuildOnServer(Selections, ShopZone));
}

void UNF_BuildComponent::ClientBuildRequestCompleted_Implementation(const FNF_BuildValidation& Validation)
{
	// The replicated CurrentBuild is the source of truth; RPC and property arrival order may differ.
	OnBuildRequestCompleted.Broadcast(Validation);
}

FNF_BuildValidation UNF_BuildComponent::ApplyBuildOnServer(
	const TArray<FNF_BuildSelection>& Selections, ANF_BuildShopZone* ShopZone)
{
	FNF_BuildValidation Validation;
	auto Fail = [&Validation](ENF_BuildResult Result, const TCHAR* Message)
	{
		Validation.Result = Result;
		Validation.Message = FText::FromString(Message);
		return Validation;
	};

	if (!GetOwner()->HasAuthority())
	{
		return Fail(ENF_BuildResult::NotAuthority, TEXT("빌드 적용은 서버에서만 가능합니다."));
	}
	if (bApplyingBuild)
	{
		return Fail(ENF_BuildResult::BuildBlocked, TEXT("빌드 적용이 진행 중입니다."));
	}
	TGuardValue<bool> ApplyGuard(bApplyingBuild, true);
	if (!IsValid(AbilitySystem) || AbilitySystem->GetAvatarActor() != GetOwner())
	{
		return Fail(
			ENF_BuildResult::MissingAbilitySystem, TEXT("플레이어 ASC의 ActorInfo와 빌드 컴포넌트를 연결해야 합니다."));
	}
	if (AbilitySystem->HasMatchingGameplayTag(NFTag_Build::State_Build_Locked))
	{
		return Fail(ENF_BuildResult::BuildBlocked, TEXT("현재 상태에서는 빌드를 교체할 수 없습니다."));
	}
	if (!IsValid(ShopZone) || ShopZone->GetWorld() != GetWorld() || !ShopZone->CanBuild(GetOwner()))
	{
		return Fail(ENF_BuildResult::NotInBuildZone, TEXT("사용 가능한 빌드 교체 구역 안에 있어야 합니다."));
	}

	Validation = PreviewBuild(Selections);
	if (Validation.Result != ENF_BuildResult::Success)
	{
		return Validation;
	}

	TMap<FName, FNF_BuildGrantHandles> StagedItems;
	TSet<FName> SelectedIds;
	for (const FNF_BuildSelection& Selection : Selections)
	{
		SelectedIds.Add(Selection.ItemId);
		if (AppliedItems.Contains(Selection.ItemId))
		{
			// Keep handles to preserve running abilities and avoid resetting unchanged passives.
			continue;
		}
		UNF_BuildItemDefinition* Definition = nullptr;
		for (UNF_BuildItemDefinition* Candidate : Rules->Catalog)
		{
			if (Candidate->ItemId == Selection.ItemId)
			{
				Definition = Candidate;
				break;
			}
		}
		FNF_BuildGrantHandles Handles;
		if (!GrantItem(Definition, Selection.SlotIndex, Handles))
		{
			RevokeItem(Handles);
			for (const TPair<FName, FNF_BuildGrantHandles>& Staged : StagedItems)
			{
				RevokeItem(Staged.Value);
			}
			return Fail(ENF_BuildResult::GrantFailed, TEXT("GAS 부여에 실패했습니다. 기존 빌드를 유지합니다."));
		}
		StagedItems.Add(Selection.ItemId, MoveTemp(Handles));
	}

	for (auto Iterator = AppliedItems.CreateIterator(); Iterator; ++Iterator)
	{
		if (!SelectedIds.Contains(Iterator.Key()))
		{
			RevokeItem(Iterator.Value());
			Iterator.RemoveCurrent();
		}
	}
	for (TPair<FName, FNF_BuildGrantHandles>& Staged : StagedItems)
	{
		AppliedItems.Add(Staged.Key, MoveTemp(Staged.Value));
	}
	for (const FNF_BuildSelection& Selection : Selections)
	{
		const FNF_BuildGrantHandles& Handles = AppliedItems.FindChecked(Selection.ItemId);
		if (Handles.Definition->ItemType == ENF_BuildItemType::ActiveSkill)
		{
			if (FGameplayAbilitySpec* Spec = AbilitySystem->FindAbilitySpecFromHandle(Handles.AbilityHandle))
			{
				// Input IDs 1..N are reserved for active build slots. Slot movement keeps the spec.
				Spec->InputID = Selection.SlotIndex + 1;
				AbilitySystem->MarkAbilitySpecDirty(*Spec);
			}
		}
	}

	CurrentBuild = Selections;
	GetOwner()->ForceNetUpdate();
	OnBuildChanged.Broadcast();
	return Validation;
}

bool UNF_BuildComponent::GrantItem(
	UNF_BuildItemDefinition* Definition, int32 SlotIndex, FNF_BuildGrantHandles& OutHandles)
{
	if (!Definition)
	{
		return false;
	}
	OutHandles.Definition = Definition;
	if (Definition->AbilityClass)
	{
		const int32 InputId = Definition->ItemType == ENF_BuildItemType::ActiveSkill ? SlotIndex + 1 : INDEX_NONE;
		OutHandles.AbilityHandle = AbilitySystem->GiveAbility(
			FGameplayAbilitySpec(Definition->AbilityClass, Definition->AbilityLevel, InputId, Definition));
		if (!OutHandles.AbilityHandle.IsValid())
		{
			return false;
		}
	}

	for (const TSubclassOf<UGameplayEffect>& EffectClass : Definition->EquippedEffects)
	{
		for (const FGameplayModifierInfo& Modifier : EffectClass->GetDefaultObject<UGameplayEffect>()->Modifiers)
		{
			if (!Modifier.Attribute.IsValid() || !AbilitySystem->HasAttributeSetForAttribute(Modifier.Attribute))
			{
				return false;
			}
		}
		FGameplayEffectContextHandle Context = AbilitySystem->MakeEffectContext();
		Context.AddSourceObject(Definition);
		const FGameplayEffectSpecHandle Spec =
			AbilitySystem->MakeOutgoingSpec(EffectClass, Definition->AbilityLevel, Context);
		if (!Spec.IsValid())
		{
			return false;
		}
		const FActiveGameplayEffectHandle EffectHandle = AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		if (!EffectHandle.IsValid())
		{
			return false;
		}
		OutHandles.EffectHandles.Add(EffectHandle);
	}
	return true;
}

void UNF_BuildComponent::RevokeItem(const FNF_BuildGrantHandles& Handles)
{
	if (!IsValid(AbilitySystem))
	{
		return;
	}
	if (Handles.AbilityHandle.IsValid())
	{
		AbilitySystem->CancelAbilityHandle(Handles.AbilityHandle);
		AbilitySystem->ClearAbility(Handles.AbilityHandle);
	}
	for (const FActiveGameplayEffectHandle& EffectHandle : Handles.EffectHandles)
	{
		AbilitySystem->RemoveActiveGameplayEffect(EffectHandle);
	}
}

void UNF_BuildComponent::OnRep_CurrentBuild()
{
	OnBuildChanged.Broadcast();
}

void UNF_BuildComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner()->HasAuthority())
	{
		for (const TPair<FName, FNF_BuildGrantHandles>& Item : AppliedItems)
		{
			RevokeItem(Item.Value);
		}
		AppliedItems.Empty();
	}
	Super::EndPlay(EndPlayReason);
}

void UNF_BuildComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UNF_BuildComponent, CurrentBuild, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNF_BuildComponent, PlayerLevel, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UNF_BuildComponent, Rules, COND_OwnerOnly);
}
