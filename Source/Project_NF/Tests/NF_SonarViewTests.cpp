#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UI/Sonar/NF_SilhouetteView.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Materials/Material.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_SonarViewTest, "NF.Skills.Sonar.OverlappingRequestsAndOriginalMeshPreservation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNF_SonarViewTest::RunTest(const FString& Parameters)
{
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACharacter* Target = World->SpawnActor<ACharacter>(Params);
	ANF_SilhouetteView* View = World->SpawnActor<ANF_SilhouetteView>(Params);
	USkeletalMesh* Asset = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/DefaultResource/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
	if (!TestNotNull(TEXT("Test mesh is available"), Asset))
	{
		GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return false;
	}
	USkeletalMeshComponent* Original = Target->GetMesh();
	Original->SetSkeletalMeshAsset(Asset);
	Original->SetRenderCustomDepth(true); Original->SetCustomDepthStencilValue(17);
	UMaterialInterface* OriginalMaterial = Original->GetMaterial(0);
	const FGuid First = FGuid::NewGuid(), Second = FGuid::NewGuid();
	FNF_SilhouetteStyle Style;
	UMaterial* Material = UMaterial::GetDefaultMaterial(MD_PostProcess);
	const TArray<TObjectPtr<ACharacter>> Targets = { Target };
	View->UpdateRequest(First, Targets, 10.0, Material, Style);
	View->UpdateRequest(Second, Targets, 20.0, Material, Style);
	View->ReconcileMeshes();
	TestEqual(TEXT("Overlapping sources share a proxy"), View->Meshes.Num(), 1);
	if (View->Meshes.Num() == 1)
	{
		TestTrue(TEXT("Proxy is a separate mesh"), View->Meshes[0].Proxy != Original);
		TestEqual(TEXT("Proxy has the view's stencil"), View->Meshes[0].Proxy->CustomDepthStencilValue, 240);
		TestFalse(TEXT("Proxy is excluded from ordinary scene rendering"), View->Meshes[0].Proxy->bRenderInMainPass != 0);
	}
	View->RemoveRequest(First);
	TestEqual(TEXT("Removing one source preserves the other reveal"), View->Meshes.Num(), 1);
	View->RemoveRequest(Second);
	TestEqual(TEXT("Removing the final source removes the proxy"), View->Meshes.Num(), 0);
	TestTrue(TEXT("Original depth flag is unchanged"), Original->bRenderCustomDepth != 0);
	TestEqual(TEXT("Original stencil is unchanged"), Original->CustomDepthStencilValue, 17);
	TestTrue(TEXT("Original material is unchanged"), Original->GetMaterial(0) == OriginalMaterial);
	Style.FillOpacity = std::numeric_limits<float>::quiet_NaN();
	TestFalse(TEXT("Non-finite style values are rejected"), Style.IsValid());
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return true;
}
#endif
