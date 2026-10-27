#include "NF_PlayerLifeComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/Tags/NFTag_LifeState.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffect.h"
#include "Core/NF_GameMode.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"
#include "GameFramework/Pawn.h"
#include "Player/State/NF_PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GAS/Tags/NFTag_Player.h"
#include "Data/HelperFunction/NF_GASFunctionLibrary.h"


UNF_PlayerLifeComponent::UNF_PlayerLifeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	SetIsReplicatedByDefault(true);
}

void UNF_PlayerLifeComponent::BeginPlay()
{
	Super::BeginPlay();
}

// 컴포가 종료되면 실행 중인 타이머 및 생존 시스템 잠금 정리
void UNF_PlayerLifeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		if (bLifeBuildLockApplied)
		{
			UAbilitySystemComponent* ASC =
				UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
			
			if (IsValid(ASC))
			{
				const FGameplayTag BuildLockedTag =
					FGameplayTag::RequestGameplayTag(
						FName(TEXT("State.Build.Locked")));
				
				ASC->RemoveLooseGameplayTag(BuildLockedTag);
			}
			
			bLifeBuildLockApplied = false;
		}
	}
	
	// 캐릭이 사라지면 Health감지 해제
	StopHealthZeroDetection();
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DownedTimerHandle);
	}
	
	Super::EndPlay(EndPlayReason);
}

void UNF_PlayerLifeComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// 현재 생존 상태를 클라에 전달
	DOREPLIFETIME(UNF_PlayerLifeComponent, LifeState);
}

ENFPlayerLifeState UNF_PlayerLifeComponent::GetLifeState() const
{
	return LifeState;
}

bool UNF_PlayerLifeComponent::EnterDowned()
{
	// 살아 있는 상태에서만 기절됨
	if (LifeState != ENFPlayerLifeState::Alive)
	{
		return false;
	}
	
	return ChangeLifeState(ENFPlayerLifeState::Downed);
}

bool UNF_PlayerLifeComponent::EnterDead()
{
	// 사망한 상태면 중복 처리x
	if (LifeState == ENFPlayerLifeState::Dead)
	{
		return false;
	}
	
	return ChangeLifeState(ENFPlayerLifeState::Dead);
}

// Health를 회복한 뒤에 Alive로 변경
bool UNF_PlayerLifeComponent::Revive()
{
	// 빈사 상태에서만 생존 상태로
	if (LifeState != ENFPlayerLifeState::Downed)
	{
		return false;
	}
	
	// 서버에서만 소생 처리
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		return false;
	}
	
	// 소생 회복 GE가 지정되지 않았다면 실패
	if (!ReviveHealthEffectClass)
	{
		return false;
	}
	
	// 소생 대상 캐릭 ASC 찾기
	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	
	if (!IsValid(ASC))
	{
		return false;
	}
	
	// 현재 플레이 중인 캐릭이 아니면 소생 처리x
	if (ASC->GetAvatarActor() != GetOwner())
	{
		return false;
	}
	
	// 회복 GE 실행 준비
	FGameplayEffectSpecHandle HealSpec =
		ASC->MakeOutgoingSpec(
			ReviveHealthEffectClass,
			1.0f,
			ASC->MakeEffectContext());
	
	if (!HealSpec.Data.IsValid())
	{
		return false;
	}
	
	// 소생 대상에게 Health 회복 GE 적용
	ASC->ApplyGameplayEffectSpecToSelf(
		*HealSpec.Data.Get());
	
	// 실제로 체력이 회복 됐는지
	const float CurrentHealth = ASC->GetNumericAttribute(
		UNF_PlayerStatusAttribute::GetHealthAttribute());
	
	// 체력 회복이 제대로 되지 않았다면 소생x / 기절 타이머도 그래도 흘러감
	if (!FMath::IsFinite(CurrentHealth) || CurrentHealth <= 0.0f)
	{
		return false;
	}
	
	return ChangeLifeState(ENFPlayerLifeState::Alive);
}

// 기절 제한 시간이 끝나면 자동으로 사망
void UNF_PlayerLifeComponent::OnDownedTimerExpired()
{
	// 이미 소생됐거나 사망했으면 추가 처리x
	if (LifeState != ENFPlayerLifeState::Downed)
	{
		return;
	}
	
	EnterDead();
}

bool UNF_PlayerLifeComponent::ChangeLifeState(ENFPlayerLifeState NewState)
{
	// 서버에서만 생존 상태 변경
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}
	
	// 이미 같은 상태면 변경x
	if (LifeState == NewState)
	{
		return false;
	}
	
	// 타이머 관리할 월드 가져오기
	UWorld* World = GetWorld();
	
	if (!World)
	{
		return false;
	}
	
	// 이전에 실행 중이던 기절 타이머 취소
	World->GetTimerManager().ClearTimer(DownedTimerHandle);
	
	LifeState = NewState;
	
	// 체력 0 판정 대기 취소
	bPendingHealthZeroCheck = false;
	
	// 기절하면 제한 시간 시작
	if (LifeState == ENFPlayerLifeState::Downed)
	{
		const float Duration = FMath::Max(DownedDurationSeconds, 0.1f);
		
		World->GetTimerManager().SetTimer(
			DownedTimerHandle,
			this,
			&UNF_PlayerLifeComponent::OnDownedTimerExpired,
			Duration,
			false
			);
	}
	
	// 변경된 생존 상태에 맞춰 태그 갱신
	SyncLifeStateTags();
	
	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	
	// Build 잠금
	if (IsValid(ASC))
	{
		const FGameplayTag BuildLockedTag =
			FGameplayTag::RequestGameplayTag(FName(TEXT("State.Build.Locked")));
		
		const bool bShouldLockBuild =
			LifeState == ENFPlayerLifeState::Downed || LifeState == ENFPlayerLifeState::Dead;
		
		if (bShouldLockBuild && !bLifeBuildLockApplied)
		{
			ASC->AddLooseGameplayTag(BuildLockedTag);
			bLifeBuildLockApplied = true;
		}
		else if (!bShouldLockBuild && bLifeBuildLockApplied)
		{
			ASC->RemoveLooseGameplayTag(BuildLockedTag);
			bLifeBuildLockApplied = false;
		}
	}
	
	// Downed GA
	if (IsValid(ASC))
	{
		if (LifeState == ENFPlayerLifeState::Downed)
		{
			ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(NFTag_LifeState::Ability_Life_Downed));
		}
		else
		{
			FGameplayTagContainer DownedAbilityTag;
			DownedAbilityTag.AddTag(NFTag_LifeState::Ability_Life_Downed);
			
			ASC->CancelAbilities(&DownedAbilityTag);
		}
	}
	// 사망 상태가 확정되면 Death GA 실행 요청
	if (LifeState == ENFPlayerLifeState::Dead)
	{
		UNF_GASFunctionLibrary::NF_TryAbility(
			GetOwner(),
			NFTag_Player::Player_Ability_Death);
	}
	
	OnServerPlayerLifeStateChanged.Broadcast(this,LifeState);
	
	// 서버에서 생존 상태 변경 알림
	OnLifeStateChanged.Broadcast(this, LifeState);
	
	return true;
}

// 현재 생존 상태에 맞게 태그 변경
bool UNF_PlayerLifeComponent::SyncLifeStateTags()
{
	// 서버에서만 태그 변경
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		return false;
	}
	
	// 이 컴포가 붙어 있는 캐릭터의 ASC 찾기
	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(
			GetOwner());
	
	// ASC가 없으면 태그 변경없이 종료
	if (!IsValid(ASC))
	{
		return false;
	}
	
	// ASC가 현재 어떤 캐릭을 사용 중인지
	if (ASC->GetAvatarActor() != GetOwner())
	{
		return false;
	}
	
	// Downed 이면 1, 그 외 0
	const int32 DownedCount =
		LifeState == ENFPlayerLifeState::Downed ? 1 : 0;
	
	// Dead 1, 그 외 0
	const int32 DeadCount =
		LifeState == ENFPlayerLifeState::Dead ? 1 : 0;
	
	ASC->SetLooseGameplayTagCount(
		NFTag_LifeState::State_Life_Downed,
		DownedCount,
		EGameplayTagReplicationState::TagAndCountToAll);
	
	ASC->SetLooseGameplayTagCount(
		NFTag_LifeState::State_Life_Dead,
		DeadCount,
		EGameplayTagReplicationState::TagAndCountToAll);
	
	return true;
}

bool UNF_PlayerLifeComponent::ConfirmTeamAssignment()
{
	// 서버에서만 팀 배정 완료 처리
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	
	if (!IsValid(OwnerPawn) || !OwnerPawn->HasAuthority())
	{
		return false;
	}
	
	// 현재 캐릭의 PlayerState 가져오기
	 const ANF_PlayerState* PlayerState =
	 	OwnerPawn->GetPlayerState<ANF_PlayerState>();
	
	if (!IsValid(PlayerState))
	{
		return false;
	}
	
	// 팀 ID가 1~4인지 확인
	if (PlayerState->NF_TeamID < 1 || PlayerState->NF_TeamID > 4)
	{
		return false;
	}
	
	// 실제 배정 완료 후 이 함수를 호출했다는 전제로
	bTeamAssignmentConfirmed = true;
	
	// 이전에 체력 0 판정이 보류됐으면 다시 검사
	RetryPendingHealthZeroCheck();
	
	return true;
}

// 보류중인 체력 0 판정 다시
bool UNF_PlayerLifeComponent::RetryPendingHealthZeroCheck()
{
	// 다시 확인할 판정이 없으면 실행x
	if (!bPendingHealthZeroCheck)
	{
		return false;
	}
	
	// 서버에서만 재판정
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		return false;
	}
	
	// 현재 체력을 감지하고 있는 ASC 가져옴
	UAbilitySystemComponent* ASC = ObserverASC.Get();
	
	if (!IsValid(ASC) || ASC->GetAvatarActor() != GetOwner())
	{
		return false;
	}
	
	// 이미 기절했거나 죽었으면 안 기다림
	if (LifeState != ENFPlayerLifeState::Alive)
	{
		bPendingHealthZeroCheck = false;
		return false;
	}
	
	// 현재 체력 확인
	const float CurrentHealth =
		ASC->GetNumericAttribute(
			UNF_PlayerStatusAttribute::GetHealthAttribute());
	
	// 체력 값이 잘못됐으면 임의로 상태 결정x
	if (!FMath::IsFinite(CurrentHealth))
	{
		return false;
	}
	
	// 체력이 회복됐으면 이전 판정 대기 취소
	if (CurrentHealth > 0.0f)
	{
		bPendingHealthZeroCheck = false;
		return false;
	}
	
	// 현재 살아 있는 팀원이 있는지
	const ENFTeammateCheckResult TeammateResult = CheckAliveTeammate();
	
	// 정보가 여전히 부족하면 다음 알림까지 대기
	if (TeammateResult == ENFTeammateCheckResult::Unknown)
	{
		return false;
	}
	
	// 살아 있는 팀원이 있으면 Downed로 전환
	if (TeammateResult == ENFTeammateCheckResult::AliveTeammate)
	{
		return EnterDowned();
	}
	
	// 살아 있는 팀원이 없다면 Dead로 전환
	return EnterDead();
}

bool UNF_PlayerLifeComponent::HandleHealthReachedZero()
{
	// 생존 상태는 서버에서만 결정
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}
	
	// 이미 기절 또는 사망 상태면 다시 판정x
	if (LifeState != ENFPlayerLifeState::Alive)
	{
		return false;
	}
	
	// 살아 있는 팀원이 있는지 확인
	switch (CheckAliveTeammate())
	{
	case ENFTeammateCheckResult::AliveTeammate:
		// 살아있는 팀원이 있으면 기절로
		return EnterDowned();
	
	case ENFTeammateCheckResult::NoAliveTeammate:
		// 없으면 사망
		return EnterDead();
		
	case ENFTeammateCheckResult::Unknown:
		// 팀원 정보를 아직 확인할 수 없으면 판정 보류
		return MarkHealthZeroCheckPending();
		
	default:
		return false;
	}
}

// 팀원의 생존 상태를 확인
ENFTeammateCheckResult UNF_PlayerLifeComponent::CheckAliveTeammate() const
{
	// 팀 배정이 확인되지 않으면 판단x
	if (!bTeamAssignmentConfirmed)
	{
		return ENFTeammateCheckResult::Unknown;
	}
	
	// 서버에서만 팀원 확인
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	
	if (!IsValid(OwnerPawn) || !OwnerPawn->HasAuthority())
	{
		return ENFTeammateCheckResult::Unknown;
	}
	
	const ANF_PlayerState* MyPlayerState =
		OwnerPawn->GetPlayerState<ANF_PlayerState>();
	
	if (!IsValid(MyPlayerState))
	{
		return ENFTeammateCheckResult::Unknown;
	}
	
	// 팀 ID가 1~4인지 확인
	const int32 MyTeamID = MyPlayerState->NF_TeamID;
	
	if (MyTeamID < 1 || MyTeamID > 4)
	{
		return ENFTeammateCheckResult::Unknown;
	}
	
	const UWorld* World = GetWorld();
	
	if (!IsValid(World))
	{
		return ENFTeammateCheckResult::Unknown;
	}
	
	// 현재 게임에 참가한 플레이어 목록을 가진 GameState 찾기
	const AGameStateBase* GameState =
		World->GetGameState<AGameStateBase>();
	
	if (!IsValid(GameState))
	{
		return ENFTeammateCheckResult::Unknown;
	}
	
	// 같은 팀 동료 찾기 (2인 1팀)
	ANF_PlayerState* TeammateState = nullptr;
	int32 TeammateCount = 0;
	
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		ANF_PlayerState* Candidate =
			Cast<ANF_PlayerState>(PlayerState);
		
		if (!IsValid(Candidate))
		{
			continue;
		}
		
		// 내 PlayerState는 제외
		if (Candidate == MyPlayerState)
		{
			continue;
		}
		
		// 다른 팀 플레이어는 제외
		if (Candidate->NF_TeamID != MyTeamID)
		{
			continue;
		}
		
		++TeammateCount;
		TeammateState = Candidate;
	}
	
	// 팀원이 1명인지 확인
	if (TeammateCount != 1 || !IsValid(TeammateState))
	{
		return ENFTeammateCheckResult::Unknown;
	}
	
	// 팀원의 현재 폰 가져오기
	APawn* TeammatePawn = TeammateState->GetPawn();
	
	if (!IsValid(TeammatePawn))
	{
		return ENFTeammateCheckResult::Unknown;
	}
	
	if (TeammatePawn->IsActorBeingDestroyed())
	{
		return ENFTeammateCheckResult::NoAliveTeammate;
	}
	
	// 팀원 캐릭터의 생존 상태 확인
	const UNF_PlayerLifeComponent* TeammateLife =
		TeammatePawn->FindComponentByClass<UNF_PlayerLifeComponent>();
	
	if (!IsValid(TeammateLife))
	{
		return ENFTeammateCheckResult::Unknown;
	}
	
	// 팀원도 팀 배정을 확인 했는지
	if (!TeammateLife->bTeamAssignmentConfirmed)
	{
		return ENFTeammateCheckResult::Unknown;
	}
	
	// 기절 또는 사망 상태의 팀원은 살아 있는 팀원으로 포함x
	if (TeammateLife->GetLifeState() != ENFPlayerLifeState::Alive)
	{
		return ENFTeammateCheckResult::NoAliveTeammate;
	}
	
	// 팀원의 ASC 확인
	UAbilitySystemComponent* TeammateASC =
		TeammateState->GetAbilitySystemComponent();
	
	if (!IsValid(TeammateASC))
	{
		return ENFTeammateCheckResult::Unknown;
	}
	
	// ASC가 현재 팀원 캐릭터를 사용 중인지 확인
	if (TeammateASC->GetAvatarActor() != TeammatePawn)
	{
		return ENFTeammateCheckResult::Unknown;
	}
	
	// 팀원의 실제 체력 확인
	const float TeammateHealth =
		TeammateASC->GetNumericAttribute(
			UNF_PlayerStatusAttribute::GetHealthAttribute());
	
	if (!FMath::IsFinite(TeammateHealth))
	{
		return ENFTeammateCheckResult::Unknown;
	}
	
	// Health가 1 이상일 때만 살아 있는 팀원
	if (TeammateHealth >= 1.0f)
	{
		return ENFTeammateCheckResult::AliveTeammate;
	}
	
	return ENFTeammateCheckResult::NoAliveTeammate;
}

// 팀원 정보가 부족하면 체력 0 판정 보류
bool UNF_PlayerLifeComponent::MarkHealthZeroCheckPending()
{
	// 서버에서만 보류 처리
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		return false;
	}
	
	// 살아 있는 상태에서만 판정 보류
	if (LifeState != ENFPlayerLifeState::Alive)
	{
		return false;
	}
	
	// 현재 캐릭터의 ASC 가져오기
	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	
	if (!IsValid(ASC) || ASC->GetAvatarActor() != GetOwner())
	{
		return false;
	}
	
	// 보류 후 재판정에 사용할 Health 감지 연결 확인
	if (!StartHealthZeroDetection())
	{
		return false;
	}
	
	// 현재 Health 확인
	const float CurrentHealth =
		ASC->GetNumericAttribute(
			UNF_PlayerStatusAttribute::GetHealthAttribute());
	
	// 체력이 0이 아니면 판정 보류x
	if (!FMath::IsFinite(CurrentHealth) || CurrentHealth > 0.0f)
	{
		return false;
	}
	
	// 팀원 정보가 여전히 부족할 때만 보류
	if (CheckAliveTeammate() != ENFTeammateCheckResult::Unknown)
	{
		return false;
	}
	
	// 팀원 정보가 준비되면 다시 검사
	bPendingHealthZeroCheck = true;
	
	return true;
}

bool UNF_PlayerLifeComponent::StartHealthZeroDetection()
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		return false;
	}
	
	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	
	if (!IsValid(ASC))
	{
		return false;
	}
	
	// 현재 ASC가 사용하는 캐릭만 Health 감지
	if (ASC->GetAvatarActor() != GetOwner())
	{
		return false;
	}
	
	// 같은 ASC에 이미 연결됐으면 중복 등록x
	if (ObserverASC.Get() == ASC && HealthChangedHandle.IsValid())
	{
		return true;
	}
	
	// 이전 ASC에 연결된 감지 기능은 정리
	StopHealthZeroDetection();
	
	// Health 바뀔 때 OnHealthChanged 실행
	HealthChangedHandle =
		ASC->GetGameplayAttributeValueChangeDelegate(
			UNF_PlayerStatusAttribute::GetHealthAttribute()).AddUObject(this, &UNF_PlayerLifeComponent::OnHealthChanged);
	
	ObserverASC = ASC;
	
	return true;
}

// Health가 회복되면 보류 중인 체력 0 판정 취소
void UNF_PlayerLifeComponent::OnHealthChanged(const FOnAttributeChangeData& Data)
{
    if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
    {
        return;
    }
	
	// 현재 캐릭이 아니면 처리x
	UAbilitySystemComponent* ASC = ObserverASC.Get();
	
	if (!IsValid(ASC) || ASC->GetAvatarActor() != GetOwner())
	{
		return;
	}
	
	// 판정 보류 중 체력이 회복되면 대기 취소
	if (bPendingHealthZeroCheck && Data.NewValue > 0.0f)
	{
		bPendingHealthZeroCheck = false;
	}
}

void UNF_PlayerLifeComponent::StopHealthZeroDetection()
{
	if (UAbilitySystemComponent* ASC = ObserverASC.Get())
	{
		if (HealthChangedHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(
				UNF_PlayerStatusAttribute::GetHealthAttribute()).Remove(HealthChangedHandle);
		}
	}
	
	HealthChangedHandle.Reset();
	ObserverASC.Reset();
	
	// 감지 기능이 종료되면 보류 중인 판정 취소
	bPendingHealthZeroCheck = false;
}

void UNF_PlayerLifeComponent::OnRep_LifeState()
{
	// 클라에서 복제된 생존 상태 변경 알림
	OnLifeStateChanged.Broadcast(this, LifeState);
}
