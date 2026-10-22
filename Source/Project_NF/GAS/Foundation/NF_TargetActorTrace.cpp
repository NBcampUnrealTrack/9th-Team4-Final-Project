#include "NF_TargetActorTrace.h"

#include "Abilities/GameplayAbility.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Player/Character/NF_Player.h"
#include "Player/Data/NF_PlayerDataAsset.h"


ANF_TargetActorTrace::ANF_TargetActorTrace()
{
	ShouldProduceTargetDataOnServer = false;
	PrimaryActorTick.bCanEverTick = true;             
	PrimaryActorTick.TickGroup = TG_PostUpdateWork; 
}


void ANF_TargetActorTrace::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);
	
	SourceActor = Ability->GetAvatarActorFromActorInfo();
	WeaponMesh = GetWeaponMeshFromSourceActor(SourceActor);
	ResetMeleeTrace();
	
	auto* NF_PlayerDataAsset = Cast<UNF_PlayerDataAsset>(Ability->GetCurrentSourceObject()); // [추가] GA의 SourceObject = 데이터 에셋
	if (IsValid(NF_PlayerDataAsset) == false) return;
	
	MeleeTraceRadius = NF_PlayerDataAsset->MeleeTraceRadius;
}


void ANF_TargetActorTrace::ConfirmTargetingAndContinue()
{
	check(ShouldProduceTargetData() == true);
	if (IsConfirmTargetingAllowed() == true)
	{
		const TArray<FHitResult> CurrentHitResults = GetMeleeTraceResults(SourceActor); 
		
		FGameplayAbilityTargetDataHandle DataHandle;
		for (const FHitResult& HitResult : CurrentHitResults)
		{
			DataHandle.Add(new FGameplayAbilityTargetData_SingleTargetHit(HitResult));
		}
		
		TargetDataReadyDelegate.Broadcast(DataHandle);
	}
}

void ANF_TargetActorTrace::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (IsValid(OwningAbility) == false) return;
	if (ShouldProduceTargetData() == false) return;

	const TArray<FHitResult> CurrentHitResults = GetMeleeTraceResults(SourceActor);
	if (CurrentHitResults.Num() == 0) return;

	FGameplayAbilityTargetDataHandle DataHandle;
	for (const FHitResult& HitResult : CurrentHitResults)
	{
		DataHandle.Add(new FGameplayAbilityTargetData_SingleTargetHit(HitResult));
	}

	TargetDataReadyDelegate.Broadcast(DataHandle);
	
}


/*
FHitResult ANF_TargetActorTrace::GetTestTraceResult(AActor* InSourceActor)
{
	FHitResult TraceResult;
	
	if (IsValid(InSourceActor) == false) return TraceResult;
	
	const FVector Start = InSourceActor->GetActorLocation();
	const FVector End = (Start + (InSourceActor->GetActorForwardVector() * SphereTraceRange));
	
	EDrawDebugTrace::Type DebugDrawType = EDrawDebugTrace::None;
	if (bDebug == true)
	{
		DebugDrawType = EDrawDebugTrace::ForDuration; 
	}
	
	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(InSourceActor);
	
	UKismetSystemLibrary::SphereTraceSingleForObjects( 
		this,
		Start,
		End,
		SphereTraceRadius,
		{UEngineTypes::ConvertToObjectType(ECC_Pawn)}, //{}를 사용하면 해당 인자의 배열선언과 동일해짐
																		//모션매칭이 가지고있는 트래버셜 채널과 겹쳐서 pawn으로 바꿈
		false,
		IgnoreActors,
		DebugDrawType,
		TraceResult,
		true,
		FColor::Red,
		FColor::Green,
		5.0f);
	
	return TraceResult;
}
*/


void ANF_TargetActorTrace::ResetMeleeTrace()
{
	HitActors.Reset();
	bIsPreviousLocationValid = false;
}


UMeshComponent* ANF_TargetActorTrace::GetWeaponMeshFromSourceActor(AActor* InSourceActor) const
{
	if (IsValid(InSourceActor) == false) return nullptr;
	
	//TODO 항상 들고 있을 (짤주먹) 근접무기가 필요하고, 궁극기를 위한 검 타입도 필요함(이건 장착만하면 됨)
	auto* WeaponComponent = InSourceActor->FindComponentByClass<UNF_WeaponComponent>();
	if (IsValid(WeaponComponent) == false) return nullptr;

	ANF_WeaponBase* EquippedWeapon = WeaponComponent->GetEquippedWeapon();
	if (IsValid(EquippedWeapon) == false) return nullptr;

	return EquippedWeapon->GetWeaponMesh();
	
	//Test용
	/*auto* NF_Player = Cast<ANF_Player>(InSourceActor);
	if (IsValid(NF_Player) == false) return nullptr;
	
	NF_Player->MeleeAttackMeshComponent;

	return NF_Player->MeleeAttackMeshComponent;*/
}


TArray<FHitResult> ANF_TargetActorTrace::GetMeleeTraceResults(AActor* InSourceActor)
{
	TArray<FHitResult> NewHitResults;

	if (IsValid(WeaponMesh) == false) return NewHitResults;
	if (IsValid(InSourceActor) == false) return NewHitResults;
	
	const FVector CurrentStartLocation = WeaponMesh->GetSocketLocation(MeleeSocket_Start);
	const FVector CurrentEndLocation = WeaponMesh->GetSocketLocation(MeleeSocket_End);

	int32 StepCount = SubStepCount;
	if (StepCount < 1)
	{
		StepCount = 1;
	}

	// 첫 Tick은 이전 위치가 없으므로 이번 칼날만 1번
	if (bIsPreviousLocationValid == false)
	{
		PreviousStartLocation = CurrentStartLocation;
		PreviousEndLocation = CurrentEndLocation;
		bIsPreviousLocationValid = true;
		StepCount = 1;
	}

	EDrawDebugTrace::Type DebugDrawType = EDrawDebugTrace::None;
	if (bDebug == true)
	{
		DebugDrawType = EDrawDebugTrace::ForDuration;
	}

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(InSourceActor);

	const float StepSize = 1.0f / StepCount;

	for (int32 StepIndex = 1; StepIndex <= StepCount; ++StepIndex)
	{
		const float Alpha = StepSize * StepIndex;
		const FVector Start = FMath::Lerp(PreviousStartLocation, CurrentStartLocation, Alpha);
		const FVector End = FMath::Lerp(PreviousEndLocation, CurrentEndLocation, Alpha);

		TArray<FHitResult> StepHitResults;
		UKismetSystemLibrary::SphereTraceMultiForObjects(
			this,
			Start,
			End,
			MeleeTraceRadius,
			{UEngineTypes::ConvertToObjectType(ECC_Pawn)},
			false,
			IgnoreActors,
			DebugDrawType,
			StepHitResults,
			true,
			FColor::Red,
			FColor::Green,
			5.0f);

		for (const FHitResult& HitResult : StepHitResults)
		{
			AActor* HitActor = HitResult.GetActor();
			if (IsValid(HitActor) == false) continue;
			if (HitActors.Contains(HitActor) == true) continue;

			HitActors.Add(HitActor);
			IgnoreActors.Add(HitActor);   // 다음 스텝에서 다시 안 잡히게
			NewHitResults.Add(HitResult);
		}
	}

	PreviousStartLocation = CurrentStartLocation;
	PreviousEndLocation = CurrentEndLocation;

	return NewHitResults;
}

