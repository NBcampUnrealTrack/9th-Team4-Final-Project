#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GamePlay/Combat/Skill/Portal/NF_Portal.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_PortalVisualTest, "NF.Skills.Portal.VisualRotationAndScaleLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_PortalVisualTest::RunTest(const FString& Parameters)
{
	TGuardValue<bool> AllowActorCallbacks(GAllowActorScriptExecutionInEditor, true);
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Caster = World->SpawnActor<ACharacter>(Params);
	auto* ASC = NewObject<UAbilitySystemComponent>(Caster); Caster->AddInstanceComponent(ASC);
	ASC->RegisterComponent(); ASC->InitAbilityActorInfo(Caster, Caster);
	auto* Entrance = World->SpawnActor<ANF_Portal>(FVector(300, 0, 0), FRotator::ZeroRotator, Params);
	auto* Exit = World->SpawnActor<ANF_Portal>(FVector(1500, 0, 0), FRotator::ZeroRotator, Params);
	Entrance->OpenScaleDuration = Exit->OpenScaleDuration = 0.2f;
	Entrance->CloseScaleDuration = Exit->CloseScaleDuration = 0.8f;
	// Existing BP meshes may be attached directly to PortalRoot rather than the new visual root.
	auto* Decoration = NewObject<UStaticMeshComponent>(Exit); Exit->AddInstanceComponent(Decoration);
	Decoration->SetupAttachment(Exit->GetRootComponent()); Decoration->SetRelativeLocation(FVector(0, 50, 110));
	Decoration->SetCollisionEnabled(ECollisionEnabled::NoCollision); Decoration->RegisterComponent();
	Entrance->InitializePortal(Exit, false, Caster, ASC); Exit->InitializePortal(Entrance, true, Caster, ASC);
	Entrance->DispatchBeginPlay(); Exit->DispatchBeginPlay();
	TestTrue(TEXT("Additional root mesh is adopted by visual root"), Decoration->GetAttachParent() == Exit->PortalVisualRoot);
	const FTransform Anchor = Exit->GetViewTransform();
	const FVector TriggerScale = Exit->EntranceTrigger->GetComponentScale();
	TestTrue(TEXT("Exit starts"), Exit->StartPortal()); TestTrue(TEXT("Entrance starts"), Entrance->StartPortal());
	TestTrue(TEXT("Opening begins at zero"), FMath::IsNearlyZero(Exit->GetVisualScaleAlpha()));
	World->Tick(LEVELTICK_All, 0.1f);
	TestTrue(TEXT("Opening reaches intermediate scale"), Exit->GetVisualScaleAlpha() > 0 && Exit->GetVisualScaleAlpha() < 1);
	World->Tick(LEVELTICK_All, 0.15f);
	Entrance->Tick(0); Exit->Tick(0);
	TestTrue(TEXT("Opening reaches full size"), FMath::IsNearlyEqual(Exit->GetVisualScaleAlpha(), 1.0f));
	TestTrue(TEXT("Exit visuals rotate by 180 degrees"), Exit->PortalVisualRoot->GetForwardVector().Equals(-Exit->GetActorForwardVector(), 0.001f));
	TestTrue(TEXT("Entrance visuals keep their heading"), Entrance->PortalVisualRoot->GetForwardVector().Equals(Entrance->GetActorForwardVector(), 0.001f));
	TestTrue(TEXT("Visual animation does not move camera anchor"), Exit->GetViewTransform().Equals(Anchor, 0.001f));
	TestTrue(TEXT("Visual animation does not scale collision"), Exit->EntranceTrigger->GetComponentScale().Equals(TriggerScale));
	Entrance->ClosePortalPair();
	TestFalse(TEXT("Closing immediately disables gameplay"), Entrance->IsPortalActive());
	TestTrue(TEXT("Closing keeps visuals visible"), Exit->IsPortalVisualVisible());
	TestTrue(TEXT("Destruction waits for the configured close duration"), Exit->GetLifeSpan() >= 0.8f);
	World->Tick(LEVELTICK_All, 0.3f);
	TestTrue(TEXT("Closing interpolates down"), Exit->GetVisualScaleAlpha() > 0 && Exit->GetVisualScaleAlpha() < 1);
	World->Tick(LEVELTICK_All, 0.3f); World->Tick(LEVELTICK_All, 0.25f);
	TestTrue(TEXT("Closing reaches zero before destruction"), FMath::IsNearlyZero(Exit->GetVisualScaleAlpha()));
	TestFalse(TEXT("Finished closing hides portal visuals"), Exit->IsPortalVisualVisible());
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return true;
}
#endif
