#include "Core/Component/NF_BossEncounterComponent.h"

#include "AbilitySystemComponent.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"

UNF_BossEncounterComponent::UNF_BossEncounterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UNF_BossEncounterComponent::HasServerAuthority() const
{
	const AActor* Owner = GetOwner();

	return IsValid(Owner) && Owner->HasAuthority();
}

ANF_BossBase* UNF_BossEncounterComponent::GetActiveBoss() const
{
	return ActiveBoss.Get();
}

void UNF_BossEncounterComponent::RequestSpawn()
{
	if (!HasServerAuthority() ||bSpawnRequested ||bCancelled) return;

	// 외부 콜백 전에 설정해 중복·재진입 요청을 차단
	bSpawnRequested = true;

	if (!RequestBossSpawn.IsBound())
	{
		FailSpawn(TEXT("보스 출현 시스템이 연결되지 않았습니다."));
		return;
	}

	ANF_BossBase* SpawnedBoss = RequestBossSpawn.Execute();

	// 출현 콜백 도중 매치가 종료 시
	if (bCancelled) return;
	

	if (!IsValid(SpawnedBoss) ||SpawnedBoss->GetWorld() != GetWorld() ||
		!SpawnedBoss->HasAuthority() ||SpawnedBoss->IsDefeated())
	{
		FailSpawn(TEXT("출현한 보스가 유효하지 않습니다."));
		return;
	}

	UAbilitySystemComponent* ASC =SpawnedBoss->GetAbilitySystemComponent();

	if (!IsValid(ASC) ||ASC->GetAvatarActor() != SpawnedBoss)
	{
		FailSpawn(TEXT("보스 ASC 초기화가 완료되지 않았습니다."));
		return;
	}

	ActiveBoss = SpawnedBoss;

	OnBossSpawnSucceeded.Broadcast(SpawnedBoss);
}

void UNF_BossEncounterComponent::FailSpawn(const FString& Reason)
{
	UE_LOG(LogTemp, Error, TEXT("%s"), *Reason);

	OnBossSpawnFailed.Broadcast(Reason);
}

void UNF_BossEncounterComponent::CancelEncounter()
{
	bCancelled = true;

	// 참조만 해제
	ActiveBoss.Reset();
}

void UNF_BossEncounterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelEncounter();

	RequestBossSpawn.Unbind();
	OnBossSpawnSucceeded.Clear();
	OnBossSpawnFailed.Clear();

	Super::EndPlay(EndPlayReason);
}

bool UNF_BossEncounterComponent::CanRegisterSpawnHandler() const
{
	return HasServerAuthority() && !bSpawnRequested && !bCancelled && !RequestBossSpawn.IsBound();
}