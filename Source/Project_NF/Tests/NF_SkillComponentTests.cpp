#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/SphereComponent.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GAS/Attributes/NF_DeployableHealthAttribute.h"
#include "GamePlay/Combat/Components/NF_ApplyEffectComponent.h"
#include "GamePlay/Combat/Components/NF_TargetQueryComponent.h"
#include "GamePlay/Combat/Components/NF_HitEffectComponent.h"

// @Owner: 스킬 담당 (구현: Codex) | 2026-10-14
// 목적: 실제 ASC/GE로 효과 정리·명중 중복과 실제 월드 overlap으로 대상 탐색을 검증한다.
namespace
{
	struct FNF_TestWorld
	{
		UWorld* World;
		FNF_TestWorld()
		{
			const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
				.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		}
		~FNF_TestWorld() { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); }
		AActor* Actor(FVector Location)
		{
			AActor* Result = World->SpawnActor<AActor>();
			USphereComponent* Sphere = NewObject<USphereComponent>(Result);
			Result->AddInstanceComponent(Sphere); Result->SetRootComponent(Sphere);
			Sphere->SetSphereRadius(12.0f); Sphere->SetCollisionObjectType(ECC_Pawn);
			Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Sphere->SetCollisionResponseToAllChannels(ECR_Block);
			Sphere->RegisterComponent(); Result->SetActorLocation(Location);
			return Result;
		}
	};
	UAbilitySystemComponent* AddASC(AActor* Owner)
	{
		auto* ASC = NewObject<UAbilitySystemComponent>(Owner); Owner->AddInstanceComponent(ASC); ASC->RegisterComponent();
		ASC->InitAbilityActorInfo(Owner, Owner); return ASC;
	}
	FGameplayEffectSpecHandle MakeSpec(UAbilitySystemComponent* Source, FGameplayAttribute Attribute, float Amount, EGameplayEffectDurationType Duration)
	{
		auto* Definition = NewObject<UGameplayEffect>(); Definition->DurationPolicy = Duration;
		FGameplayModifierInfo Modifier; Modifier.Attribute = Attribute; Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Amount)); Definition->Modifiers.Add(Modifier);
		return FGameplayEffectSpecHandle(new FGameplayEffectSpec(Definition, Source->MakeEffectContext(), 1.0f));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_EffectAndHitTest, "NF.SkillComponents.EffectsAndHits", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_EffectAndHitTest::RunTest(const FString& Parameters)
{
	FNF_TestWorld Scope;
	AActor* Source = Scope.Actor(FVector(-1000, 0, 0)); AActor* Target = Scope.Actor(FVector(100, 0, 0));
	auto* SourceASC = AddASC(Source); auto* TargetASC = AddASC(Target);
	auto* Attributes = NewObject<UNF_DeployableHealthAttribute>(Target); TargetASC->AddAttributeSetSubobject(Attributes);
	auto* Apply = NewObject<UNF_ApplyEffectComponent>(Source); Source->AddInstanceComponent(Apply); Apply->RegisterComponent();
	const auto Buff = MakeSpec(SourceASC, UNF_DeployableHealthAttribute::GetMaxHealthAttribute(), 25.0f, EGameplayEffectDurationType::Infinite);
	TestTrue(TEXT("Owned buff submitted"), Apply->ApplySpecToTarget(Target, Buff, FVector::ZeroVector, FHitResult(), true));
	TestEqual(TEXT("Buff increased maximum"), Attributes->GetMaxHealth(), 125.0f);
	TestTrue(TEXT("Repeat does not duplicate owned buff"), Apply->ApplySpecToTarget(Target, Buff, FVector::ZeroVector, FHitResult(), true));
	TestEqual(TEXT("Maximum remains 125"), Attributes->GetMaxHealth(), 125.0f);
	const auto Independent = MakeSpec(SourceASC, UNF_DeployableHealthAttribute::GetMaxHealthAttribute(), 10.0f, EGameplayEffectDurationType::Infinite);
	TestTrue(TEXT("Independent buff submitted"), Apply->ApplySpecToTarget(Target, Independent, FVector::ZeroVector, FHitResult(), false));
	Apply->StopEffects();
	TestEqual(TEXT("Cleanup removes only owned buff"), Attributes->GetMaxHealth(), 110.0f);
	const auto Damage = MakeSpec(SourceASC, UNF_DeployableHealthAttribute::GetIncomingDamageAttribute(), 35.0f, EGameplayEffectDurationType::Instant);
	const FHitResult Hit(Target, Cast<UPrimitiveComponent>(Target->GetRootComponent()), Target->GetActorLocation(), FVector::ForwardVector);
	TestTrue(TEXT("Damage submitted"), UNF_ApplyEffectComponent::SubmitSpec(Source, Target, Damage, Source->GetActorLocation(), Hit));
	TestEqual(TEXT("Actual health decreased"), Attributes->GetHealth(), 65.0f);
	TestTrue(TEXT("Source context was not modified"), Damage.Data->GetContext().GetHitResult() == nullptr);
	Apply->TargetMode = ENF_EffectTargetMode::Hit; Apply->Effects.AddDefaulted_GetRef().EffectClass = UGameplayEffect::StaticClass();
	TestTrue(TEXT("Hit effect prepared"), Apply->PrepareEffects(Source));
	auto* Hits = NewObject<UNF_HitEffectComponent>(Source); Source->AddInstanceComponent(Hits); Hits->RegisterComponent();
	TestTrue(TEXT("Hit listener ready"), Hits->StartListening(Source));
	TestEqual(TEXT("First hit submitted"), Hits->SubmitHit(Target, Hit), 1);
	TestEqual(TEXT("Same target hit ignored"), Hits->SubmitHit(Target, Hit), 0);
	TestEqual(TEXT("Source hit ignored"), Hits->SubmitHit(Source, Hit), 0);
	Hits->StopListening();
	TestEqual(TEXT("Stopped listener ignores hits"), Hits->SubmitHit(Target, Hit), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_TargetQueryTest, "NF.SkillComponents.TargetQuery", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_TargetQueryTest::RunTest(const FString& Parameters)
{
	FNF_TestWorld Scope;
	AActor* Source = Scope.Actor(FVector(-1000, 0, 0));
	AActor* Near = Scope.Actor(FVector(100, 0, 0));
	Scope.Actor(FVector(300, 0, 0)); Scope.Actor(FVector(-100, 0, 0));
	auto* Extra = NewObject<USphereComponent>(Near); Near->AddInstanceComponent(Extra);
	Extra->SetupAttachment(Near->GetRootComponent()); Extra->SetSphereRadius(8);
	Extra->SetCollisionObjectType(ECC_Pawn); Extra->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Extra->RegisterComponent();
	Scope.World->Tick(LEVELTICK_All, 0.016f);
	FNF_TargetQuerySettings Settings; Settings.Shape.Radius = 500; Settings.bRequireLineOfSight = false;
	TArray<FNF_TargetQueryResult> Results; const FNF_AreaDamageTargetFilter Filter;
	TestEqual(TEXT("Multiple colliders return one actor"), UNF_TargetQueryComponent::Query(Scope.World, Source, Source, FTransform::Identity, Settings, Filter, Results), 3);
	Settings.Shape.Shape = ENF_AreaDamageShape::Sector; Settings.Shape.SectorHalfAngle = 45;
	TestEqual(TEXT("Sector excludes rear target"), UNF_TargetQueryComponent::Query(Scope.World, Source, Source, FTransform::Identity, Settings, Filter, Results), 2);
	Settings.MaxTargets = 1;
	TestEqual(TEXT("Target limit respected"), UNF_TargetQueryComponent::Query(Scope.World, Source, Source, FTransform::Identity, Settings, Filter, Results), 1);
	if (!Results.IsEmpty()) { TestTrue(TEXT("Nearest target selected"), Results[0].Target == Near); }
	Settings.Filter.Team = ENF_TargetTeamFilter::Enemies;
	TestEqual(TEXT("Unknown team is not assumed enemy"), UNF_TargetQueryComponent::Query(Scope.World, Source, Source, FTransform::Identity, Settings, Filter, Results), 0);
	return true;
}
#endif
