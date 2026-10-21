#include "NF_TargetActorTrace.h"

#include "Abilities/GameplayAbility.h"
#include "Kismet/KismetSystemLibrary.h"


ANF_TargetActorTrace::ANF_TargetActorTrace()
{
	
}


void ANF_TargetActorTrace::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);
	
	SourceActor = Ability->GetAvatarActorFromActorInfo();
}


void ANF_TargetActorTrace::ConfirmTargetingAndContinue()
{
	check(ShouldProduceTargetData() == true);
	if (IsConfirmTargetingAllowed() == true)
	{
		FHitResult HitResult = GetSphereTraceResult(SourceActor);
		FGameplayAbilityTargetDataHandle DataHandle(new FGameplayAbilityTargetData_SingleTargetHit(HitResult));
		TargetDataReadyDelegate.Broadcast(DataHandle);
	}
}


FHitResult ANF_TargetActorTrace::GetSphereTraceResult(AActor* InSourceActor)
{
	FHitResult TraceResult;
	
	if (IsValid(InSourceActor) == false) return TraceResult;
	
	const FVector Start = InSourceActor->GetActorLocation();
	const FVector End = (Start + (InSourceActor->GetActorForwardVector() * Range));
	
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
		Radius,
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

