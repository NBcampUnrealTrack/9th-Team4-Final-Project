#include "GamePlay/ConsumableItems/Items/NF_HealthPotion.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GAS/Effects/NFGE_HealthPotion.h"
#include "GAS/Tags/NFTag_Consumable.h"
#include "GAS/Tags/NFTag_LifeState.h"
#include "GAS/Tags/NFTag_Player.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"
#include "Project_NF.h"

bool ANF_HealthPotion::TryUse_Implementation(AActor* User)
{
	if (!HasAuthority() || !IsValid(User) || User->IsActorBeingDestroyed() || !User->HasAuthority()
		|| !FMath::IsFinite(HealAmount) || HealAmount <= 0.f) { return false; }
	auto* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(User, true);
	if (!ASC || !ASC->GetSet<UNF_PlayerStatusAttribute>())
	{
		const APawn* Pawn = Cast<APawn>(User);
		ASC = Pawn ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn->GetPlayerState()) : nullptr;
	}
	if (!ASC || ASC->GetAvatarActor_Direct() != User || !ASC->GetSet<UNF_PlayerStatusAttribute>()
		|| ASC->HasMatchingGameplayTag(NFTag_Player::Player_State_Death)
		|| ASC->HasMatchingGameplayTag(NFTag_LifeState::State_Life_Downed)
		|| ASC->HasMatchingGameplayTag(NFTag_LifeState::State_Life_Dead)) { return false; }
	const auto HealthAttribute = UNF_PlayerStatusAttribute::GetHealthAttribute();
	const float Before = ASC->GetNumericAttribute(HealthAttribute);
	const float Maximum = ASC->GetNumericAttribute(UNF_PlayerStatusAttribute::GetMaxHealthAttribute());
	if (!FMath::IsFinite(Before) || !FMath::IsFinite(Maximum) || Before <= 0.f || Before >= Maximum) { return false; }

	const float Requested = FMath::Min(HealAmount, Maximum - Before);
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UNFGE_HealthPotion::StaticClass(), 1.f, Context);
	if (!Spec.IsValid()) { return false; }
	Spec.Data->SetSetByCallerMagnitude(NFTag_Consumable::Data_Consumable_HealthRestore, Requested);
	const FActiveGameplayEffectHandle Applied = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	const float After = ASC->GetNumericAttribute(HealthAttribute);
	const bool bHealed = Applied.WasSuccessfullyApplied() && FMath::IsFinite(After) && After > Before;
	UE_LOG(LogProject_NF, Log, TEXT("[HealthPotion] 캐릭터=%s | 체력=%.2f -> %.2f | 회복요청=%.2f | 성공=%d"),
		*GetNameSafe(User), Before, After, Requested, bHealed);
	return bHealed;
}
