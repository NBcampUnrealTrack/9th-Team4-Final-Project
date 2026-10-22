#include "GamePlay/Combat/Damage/NF_WeaponDamageLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GAS/Attributes/NF_OffenseAttributeSet.h"

bool UNF_WeaponDamageLibrary::EnsureOffenseAttributes(UAbilitySystemComponent* ASC)
{
	if (!IsValid(ASC) || !ASC->IsOwnerActorAuthoritative()) { return false; }
	if (!ASC->GetSet<UNF_OffenseAttributeSet>())
	{
		AActor* Owner = ASC->GetOwner();
		if (!IsValid(Owner)) { return false; }
		ASC->AddAttributeSetSubobject(NewObject<UNF_OffenseAttributeSet>(Owner));
	}
	return ASC->GetSet<UNF_OffenseAttributeSet>() != nullptr;
}

float UNF_WeaponDamageLibrary::EvaluateDamage(float BaseDamage, double Bonus, double Multiplier, bool bLastRound)
{
	const double Base = FMath::IsFinite(BaseDamage) ? FMath::Max(0.0, static_cast<double>(BaseDamage)) : 0.0;
	Bonus = FMath::IsFinite(Bonus) ? FMath::Max(0.0, Bonus) : 0.0;
	Multiplier = FMath::IsFinite(Multiplier) ? FMath::Max(1.0, Multiplier) : 1.0;
	return static_cast<float>(FMath::Clamp((Base + Bonus) * (bLastRound ? Multiplier : 1.0), 0.0, static_cast<double>(MAX_flt)));
}

float UNF_WeaponDamageLibrary::CalculateShotDamage(AActor* Shooter, float BaseDamage, bool bLastRound)
{
	TArray<UAbilitySystemComponent*> Candidates;
	const auto Collect = [&Candidates](AActor* Actor)
	{
		if (!IsValid(Actor)) { return; }
		if (auto* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor)) { Candidates.AddUnique(ASC); }
		TArray<UAbilitySystemComponent*> Components; Actor->GetComponents(Components);
		for (auto* ASC : Components) { Candidates.AddUnique(ASC); }
	};
	Collect(Shooter);
	if (APawn* Pawn = Cast<APawn>(Shooter)) { Collect(Pawn->GetPlayerState()); }
	double Bonus = 0.0, Multiplier = 1.0;
	for (UAbilitySystemComponent* ASC : Candidates)
	{
		// The temporary skill test ASC can coexist with the production PlayerState ASC.
		// Count each ASC only once, and never take bonuses from a previous pawn/avatar.
		if (!IsValid(ASC) || ASC->GetAvatarActor_Direct() != Shooter) { continue; }
		if (const auto* Stats = ASC->GetSet<UNF_OffenseAttributeSet>())
		{
			Bonus += Stats->GetAttackDamageBonus();
			Multiplier *= Stats->GetLastRoundDamageMultiplier();
		}
	}
	return EvaluateDamage(BaseDamage, Bonus, Multiplier, bLastRound);
}
