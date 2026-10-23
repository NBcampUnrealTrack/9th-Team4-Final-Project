#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Containers/Ticker.h"
#include "EngineUtils.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Monster/NF_Monster.h"
#include "TimerManager.h"

// Run on either a dedicated server or connected client:
// -ExecCmds="NF.Monster.VerifySpawn 9 60"
// Optional duration defaults to 20 seconds. Uses the engine ticker so client
// travel from the entry map does not destroy the verification timers.
static FAutoConsoleCommandWithWorldAndArgs VerifyMonsterSpawn(
	TEXT("NF.Monster.VerifySpawn"), TEXT("Verify monster count, floor and mesh. Args: expected-count duration-seconds."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World) { return; }
		const int32 Expected = Args.IsEmpty() ? 9 : FCString::Atoi(*Args[0]);
		const float Duration = Args.Num() > 1 ? FMath::Max(20.0f, FCString::Atof(*Args[1])) : 20.0f;
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
			[Expected, Duration, Elapsed = 0.0f, NextSample = 5.0f](float DeltaTime) mutable
			{
				Elapsed += DeltaTime;
				if (Elapsed >= Duration) { FPlatformMisc::RequestExit(false); return false; }
				if (Elapsed < NextSample) { return true; }
				NextSample += 5.0f;
				UWorld* World = nullptr;
				for (const FWorldContext& Context : GEngine->GetWorldContexts())
				{
					if (Context.World()) { World = Context.World(); break; }
				}
				if (!World) { return true; }
				const float Delay = Elapsed;
				int32 Count = 0, Supported = 0, Visible = 0;
				for (TActorIterator<ANF_Monster> It(World); It; ++It)
				{
					ANF_Monster* Monster = *It;
					++Count;
					FHitResult Hit;
					FCollisionQueryParams Params(SCENE_QUERY_STAT(MonsterSpawnVerify), false, Monster);
					const FVector Location = Monster->GetActorLocation();
					const bool bFloor = World->LineTraceSingleByChannel(Hit, Location,
						Location - FVector(0, 0, Monster->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 30), ECC_Pawn, Params);
					Supported += bFloor;
					const USkeletalMeshComponent* Mesh = Monster->GetMesh();
					Visible += !Monster->IsHidden() && Mesh && Mesh->IsVisible() && Mesh->GetSkeletalMeshAsset() != nullptr;
					UE_LOG(LogTemp, Display, TEXT("[MonsterSpawnVerify] t=%.0f mode=%d actor=%s location=%s floor=%s movement=%d"),
						Delay, int32(World->GetNetMode()), *Monster->GetName(), *Location.ToString(), *GetNameSafe(Hit.GetActor()), int32(Monster->GetCharacterMovement()->MovementMode));
				}
				UE_LOG(LogTemp, Display, TEXT("[MonsterSpawnVerify] t=%.0f mode=%d count=%d supported=%d visible=%d expected=%d result=%s"),
					Delay, int32(World->GetNetMode()), Count, Supported, Visible, Expected,
					Count == Expected && Supported == Expected && Visible == Expected ? TEXT("PASS") : TEXT("FAIL"));
				return true;
			}));
	}));
#endif
