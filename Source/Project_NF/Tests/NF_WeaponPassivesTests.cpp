#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"
#include "Monster/NF_Monster.h"
#include "GAS/Attributes/NF_OffenseAttributeSet.h"
#include "GAS/Effects/NFGE_EMP.h"
#include "GamePlay/Combat/Damage/NF_WeaponDamageLibrary.h"
#include "GamePlay/Combat/Skill/Passive/Attack/NFGA_AttackPower.h"
#include "GamePlay/Combat/Skill/Passive/Attack/NFGA_MagicBullet.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"
#include "GamePlay/Combat/Projectiles/NF_WeaponProjectile.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_WeaponPassivesTest, "NF.Skills.WeaponPassives.AttackBonusLastRoundAndProjectileSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_WeaponPassivesTest::RunTest(const FString& Parameters)
{
	TGuardValue<bool> AllowActorCallbacks(GAllowActorScriptExecutionInEditor, true);
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ANF_Monster* Shooter = World->SpawnActor<ANF_Monster>();
	auto* ASC = Shooter->GetAbilitySystemComponent(); ASC->InitAbilityActorInfo(Shooter, Shooter);
	TestEqual(TEXT("Missing passives keep base damage"), UNF_WeaponDamageLibrary::CalculateShotDamage(Shooter, 30, true), 30.0f);
	const auto Attack = ASC->GiveAbility(FGameplayAbilitySpec(UNFGA_AttackPower::StaticClass(), 1));
	const auto Magic = ASC->GiveAbility(FGameplayAbilitySpec(UNFGA_MagicBullet::StaticClass(), 1));
	if (!TestNotNull(TEXT("Passive registers offense attributes"), ASC->GetSet<UNF_OffenseAttributeSet>()))
	{
		GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return false;
	}
	TestEqual(TEXT("Normal shot adds attack bonus"), UNF_WeaponDamageLibrary::CalculateShotDamage(Shooter, 30, false), 40.0f);
	TestEqual(TEXT("Last shot multiplies after attack bonus"), UNF_WeaponDamageLibrary::CalculateShotDamage(Shooter, 30, true), 80.0f);
	const auto EMP = ASC->MakeOutgoingSpec(UNFGE_EMP::StaticClass(), 1, ASC->MakeEffectContext());
	ASC->ApplyGameplayEffectSpecToSelf(*EMP.Data.Get());
	TestEqual(TEXT("EMP leaves passive weapon bonuses intact"), UNF_WeaponDamageLibrary::CalculateShotDamage(Shooter, 30, true), 80.0f);
	const auto ExtraAttack = ASC->GiveAbility(FGameplayAbilitySpec(UNFGA_AttackPower::StaticClass(), 1));
	TestEqual(TEXT("Independent attack grants add"), UNF_WeaponDamageLibrary::CalculateShotDamage(Shooter, 30, true), 100.0f);
	ASC->ClearAbility(ExtraAttack);
	TestEqual(TEXT("Removing one grant keeps the other"), UNF_WeaponDamageLibrary::CalculateShotDamage(Shooter, 30, true), 80.0f);
	const auto ExtraMagic = ASC->GiveAbility(FGameplayAbilitySpec(UNFGA_MagicBullet::StaticClass(), 1));
	TestEqual(TEXT("Independent magic grants compound"), UNF_WeaponDamageLibrary::CalculateShotDamage(Shooter, 30, true), 160.0f);
	ASC->ClearAbility(ExtraMagic);
	TestEqual(TEXT("Removing one magic grant restores the remaining multiplier"), UNF_WeaponDamageLibrary::CalculateShotDamage(Shooter, 30, true), 80.0f);
	// Exercise the actual weapon spawn loop and inspect the damage saved in each generated pellet.
	auto* Data = NewObject<UNF_WeaponDataAsset>(); Data->Damage = 30; Data->ProjectilesPerShot = 3;
	Data->ProjectileClass = ANF_WeaponProjectile::StaticClass();
	auto* Weapon = World->SpawnActor<ANF_WeaponBase>(); Weapon->WeaponData = Data;
	Weapon->SetOwner(Shooter); Weapon->SetInstigator(Shooter); Weapon->CurrentAmmo = 2;
	auto* DamageProperty = FindFProperty<FFloatProperty>(ANF_WeaponProjectile::StaticClass(), TEXT("ImpactDamage"));
	if (!TestNotNull(TEXT("Projectile damage property exists"), DamageProperty))
	{
		GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return false;
	}
	const auto CheckShot = [this, World, Weapon, DamageProperty](float Expected)
	{
		TSet<ANF_WeaponProjectile*> Before;
		for (TActorIterator<ANF_WeaponProjectile> It(World); It; ++It) { Before.Add(*It); }
		Weapon->LastFireTime = -1.e30;
		TestTrue(TEXT("Weapon fires"), Weapon->FireProjectile());
		TArray<ANF_WeaponProjectile*> Shot;
		for (TActorIterator<ANF_WeaponProjectile> It(World); It; ++It)
		{
			if (!Before.Contains(*It))
			{
				Shot.Add(*It);
				TestEqual(TEXT("All pellets use the shot snapshot"), DamageProperty->GetPropertyValue_InContainer(*It), Expected);
			}
		}
		TestEqual(TEXT("Three pellets spawned"), Shot.Num(), 3); return Shot;
	};
	CheckShot(40);
	TestEqual(TEXT("Ammo consumed once per shot"), Weapon->CurrentAmmo, 1);
	const auto LastShot = CheckShot(80);
	TestEqual(TEXT("Last shot consumes the last round"), Weapon->CurrentAmmo, 0);
	TestFalse(TEXT("Empty weapon cannot fire"), Weapon->FireProjectile());
	TestEqual(TEXT("Shared weapon data is unchanged"), Data->Damage, 30.0f);
	ASC->ClearAbility(Magic);
	for (auto* Projectile : LastShot)
	{
		TestEqual(TEXT("Unequipping does not change bullets already fired"), DamageProperty->GetPropertyValue_InContainer(Projectile), 80.0f);
	}
	TestEqual(TEXT("Removing magic leaves attack bonus"), UNF_WeaponDamageLibrary::CalculateShotDamage(Shooter, 30, true), 40.0f);
	ASC->ClearAbility(Attack);
	TestEqual(TEXT("Removing both restores base damage"), UNF_WeaponDamageLibrary::CalculateShotDamage(Shooter, 30, true), 30.0f);
	// Separate test ASC and production ASC on one avatar are combined once each.
	auto* TestASC = NewObject<UAbilitySystemComponent>(Shooter); Shooter->AddInstanceComponent(TestASC);
	TestASC->RegisterComponent(); TestASC->InitAbilityActorInfo(Shooter, Shooter);
	const auto TestAttack = TestASC->GiveAbility(FGameplayAbilitySpec(UNFGA_AttackPower::StaticClass(), 1));
	TestEqual(TEXT("Separate skill test ASC bonus is read"), UNF_WeaponDamageLibrary::CalculateShotDamage(Shooter, 30, false), 40.0f);
	TestASC->ClearAbility(TestAttack);
	ANF_Monster* Other = World->SpawnActor<ANF_Monster>(FVector(2000, 0, 100), FRotator::ZeroRotator);
	Other->GetAbilitySystemComponent()->InitAbilityActorInfo(Other, Other);
	TestEqual(TEXT("Other shooter does not inherit bonuses"), UNF_WeaponDamageLibrary::CalculateShotDamage(Other, 30, true), 30.0f);
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return true;
}
#endif
