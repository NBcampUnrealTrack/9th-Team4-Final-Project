#include "Core/Component/NF_EscapeKeyComponent.h"

#include "AbilitySystemComponent.h"
#include "Data/Struct/NF_BossDefeatResult.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "Core/NF_GameState.h"
#include "GAS/Tags/NFTag_LifeState.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "GamePlay/EscapeKey/NF_EscapeKey.h"
#include "Player/State/NF_PlayerState.h"

UNF_EscapeKeyComponent::UNF_EscapeKeyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UNF_EscapeKeyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InitialAssignmentRetryTimerHandle);
	}

	CleanupKey();
	OnInitialKeyAssignmentFailed.Clear();
	OnKeyOperationFailed.Clear();
	OnKeyStateChanged.Clear();
	
	Super::EndPlay(EndPlayReason);
}

void UNF_EscapeKeyComponent::BeginInitialAssignment(const FNF_BossDefeatResult& DefeatResult)
{
	if (!HasServerAuthority() ||bKeyCleanupComplete ||!IsEscapePhase())return;

	// 결과를 반복 전달해도 절차를 다시 시작하지 않는다.
	if (InitState != ENF_KeyInitState::NotStarted)return;

	InitialRecipient = DefeatResult.LastHitPlayerState.Get();
	InitialAssignmentAttempts = 0;

	InitState = ENF_KeyInitState::Retrying;

	ANF_BossBase* Boss = DefeatResult.DefeatedBoss.Get();

	//보스위치 저장
	bHasInitialDropLocation =IsValid(Boss) &&Boss->GetWorld() == GetWorld();

	if (bHasInitialDropLocation)
	{
		InitialDropLocation = Boss->GetActorLocation();
		bHasInitialDropLocation =!InitialDropLocation.ContainsNaN();
	}
	
	TryInitialAssignment();
}

void UNF_EscapeKeyComponent::CleanupKey()
{
	if (!HasServerAuthority() || bKeyCleanupComplete){return;}

	// GE 제거 중 발생하는 콜백의 새 요청부터 차단.
	bKeyCleanupComplete = true;

	GetWorld()->GetTimerManager().ClearTimer(HolderMonitorTimerHandle);

	ANF_EscapeKey* PreviousPickup = DroppedKeyActor.Get();

	DroppedKeyActor = nullptr;
	KeyState = ENF_KeyState::Removed;

	if (IsValid(PreviousPickup))
	{
		PreviousPickup->Destroy();
	}
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InitialAssignmentRetryTimerHandle);
	}

	if (InitState ==ENF_KeyInitState::Retrying)
	{
		InitState = ENF_KeyInitState::Cancelled;
	}

	InitialRecipient.Reset();

	UAbilitySystemComponent* PreviousASC =KeyHolderEffectASC.Get();

	const FActiveGameplayEffectHandle PreviousHandle =KeyHolderEffectHandle;

	KeyHolder.Reset();
	KeyHolderEffectASC.Reset();
	KeyHolderEffectHandle.Invalidate();

	if (IsValid(PreviousASC) && PreviousHandle.IsValid())
	{
		PreviousASC->RemoveActiveGameplayEffect(PreviousHandle);
	}

	PublishKeyState();
}


ENF_KeyInitState UNF_EscapeKeyComponent::GetInitState() const
{
	return InitState;
}

FNF_KeyPublicState UNF_EscapeKeyComponent::GetPublicState() const
{
	FNF_KeyPublicState Result;
	Result.State = KeyState;
	if (KeyState == ENF_KeyState::Held)
	{
		Result.Holder = KeyHolder.Get();
	}
	return Result;
}

void UNF_EscapeKeyComponent::PublishKeyState()
{
	if (!HasServerAuthority()) { return; }

	LastPublishedHolderPawn.Reset();
	if (KeyState == ENF_KeyState::Held)
	{
		if (ANF_PlayerState* Holder = KeyHolder.Get())
		{
			LastPublishedHolderPawn = Holder->GetPawn();
		}
	}

	// 전달 중 재진입하더라도 각 구독자가 받는 스냅샷은 유지한다.
	const FNF_KeyPublicState PublicState = GetPublicState();
	OnKeyStateChanged.Broadcast(PublicState);
}

bool UNF_EscapeKeyComponent::TransferKey(ANF_PlayerState* NewKeyHolder)
{
	if (!HasServerAuthority() ||bKeyCleanupComplete ||bChangingKeyHolder ||!IsEscapePhase()){return false;}

	if (!IsValid(NewKeyHolder) ||NewKeyHolder->GetWorld() != GetWorld()){return false;}
	
	if (!CanReceiveKey(NewKeyHolder, false)){return false;}
	
	UAbilitySystemComponent* NewASC =NewKeyHolder->GetAbilitySystemComponent();

	if (!IsValid(NewASC)){return false;}

	if (KeyHolder.Get() == NewKeyHolder &&
		KeyHolderEffectASC.Get() == NewASC &&
		NewASC->GetActiveGameplayEffect(KeyHolderEffectHandle))
	{
		return true;
	}

	TGuardValue<bool> ChangeGuard(bChangingKeyHolder, true);

	FActiveGameplayEffectHandle NewEffectHandle;

	if (!ApplyKeyHolderEffect(NewKeyHolder, NewEffectHandle)){return false;}

	// 적용 콜백에서 정리되거나 효과가 즉시 제거된 경우에는
	// 새 소유권을 확정하지 않는다.
	if (bKeyCleanupComplete ||
	!IsEscapePhase() ||
	!CanReceiveKey(NewKeyHolder, false) ||
	!NewASC->GetActiveGameplayEffect(NewEffectHandle))
	{
		NewASC->RemoveActiveGameplayEffect(NewEffectHandle);
		return false;
	}

	UAbilitySystemComponent* PreviousASC =KeyHolderEffectASC.Get();

	const FActiveGameplayEffectHandle PreviousHandle =KeyHolderEffectHandle;

	KeyHolder = NewKeyHolder;
	KeyHolderEffectASC = NewASC;
	KeyHolderEffectHandle = NewEffectHandle;
	KeyState = ENF_KeyState::Held;
	bHolderLeaving = false;
	
	StartHolderMonitoring();
	
	if (IsValid(PreviousASC) && PreviousHandle.IsValid())
	{
		PreviousASC->RemoveActiveGameplayEffect(PreviousHandle);
	}

	PublishKeyState();
	return !bKeyCleanupComplete && KeyState == ENF_KeyState::Held && KeyHolder.Get() == NewKeyHolder;
}

bool UNF_EscapeKeyComponent::DropKey()
{
	if (KeyState != ENF_KeyState::Held){return false;}

	FVector DropLocation = LastHolderLocation;

	if (ANF_PlayerState* Holder = KeyHolder.Get())
	{
		if (APawn* Pawn = Holder->GetPawn())
		{
			DropLocation = Pawn->GetActorLocation();
		}
	}

	return CreateDroppedKeyAt(DropLocation);
}

bool UNF_EscapeKeyComponent::TryPickup(ANF_PlayerState* PlayerState, ANF_EscapeKey* Pickup)
{
	if (!HasServerAuthority() ||bKeyCleanupComplete ||bProcessingPickup ||bChangingKeyHolder ||!IsEscapePhase()){return false;}

	if (KeyState != ENF_KeyState::Dropped ||!IsValid(Pickup) ||DroppedKeyActor.Get() != Pickup ||!Pickup->IsPickupEnabled()){return false;}

	if (!CanReceiveKey(PlayerState, true)){return false;}

	APawn* Pawn = PlayerState->GetPawn();

	if (FVector::DistSquared(Pawn->GetActorLocation(),Pickup->GetActorLocation()) >FMath::Square(PickupDistance)){return false;}

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Pawn);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Pawn->GetPawnViewLocation(),
		Pickup->GetActorLocation(),
		ECC_Visibility,
		Params);

	if (bHit && Hit.GetActor() != Pickup){return false;}

	TGuardValue<bool> PickupGuard(bProcessingPickup, true);

	if (!TransferKey(PlayerState)){return false;}

	if (bKeyCleanupComplete || !IsKeyHolder(PlayerState)){return false;}

	DroppedKeyActor = nullptr;
	Pickup->Destroy();

	return true;	
}

void UNF_EscapeKeyComponent::HandlePlayerLeaving(ANF_PlayerState* PlayerState, APawn* LastPawn)
{
	if (!HasServerAuthority() ||bKeyCleanupComplete ||!IsKeyHolder(PlayerState))return;

	if (IsValid(LastPawn))
	{
		LastHolderLocation = LastPawn->GetActorLocation();
	}

	bHolderLeaving = true;

	MonitorHolder();
}

bool UNF_EscapeKeyComponent::IsKeyHolder(const ANF_PlayerState* PlayerState) const
{
	if (!IsValid(PlayerState)){return false;}

	return KeyState == ENF_KeyState::Held &&KeyHolder.Get() == PlayerState;
}

ANF_PlayerState* UNF_EscapeKeyComponent::GetKeyHolder() const
{
	return KeyHolder.Get();
}

bool UNF_EscapeKeyComponent::ApplyKeyHolderEffect(
	ANF_PlayerState* PlayerState,FActiveGameplayEffectHandle& OutEffectHandle)
{
	OutEffectHandle.Invalidate();

	if (!IsValid(PlayerState) || !KeyHolderRewardEffectClass){return false;}

	UAbilitySystemComponent* ASC =PlayerState->GetAbilitySystemComponent();

	if (!IsValid(ASC)){return false;}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(KeyHolderRewardEffectClass,1.0f,Context);

	if (!Spec.IsValid()){return false;}

	OutEffectHandle =ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

	return OutEffectHandle.IsValid();
}

bool UNF_EscapeKeyComponent::ValidateInitialAssignmentSettings(FString& OutReason) const
{
	OutReason.Empty();
	if (!KeyHolderRewardEffectClass)
	{
		OutReason = TEXT("열쇠 보유 GE 클래스가 지정되지 않았습니다.");
		return false;
	}

	const UGameplayEffect* EffectDefinition = KeyHolderRewardEffectClass->GetDefaultObject<UGameplayEffect>();
	if (!IsValid(EffectDefinition) || EffectDefinition->DurationPolicy != EGameplayEffectDurationType::Infinite)
	{
		OutReason = TEXT("열쇠 보유 GE는 Infinite로 설정해야 합니다.");
		return false;
	}
	return true;
}

void UNF_EscapeKeyComponent::TryInitialAssignment()
{
	if (!HasServerAuthority() ||bKeyCleanupComplete ||InitState != ENF_KeyInitState::Retrying)return;

	if (!IsEscapePhase())
	{
		InitState = ENF_KeyInitState::Cancelled;
		InitialRecipient.Reset();
		return;
	}

	const ANF_GameState* NFGameState =GetWorld()->GetGameState<ANF_GameState>();

	// 1. GE 설정을 먼저 검사한다.
	FString SettingsError;
	if (!ValidateInitialAssignmentSettings(SettingsError))
	{
		FailInitialAssignment(SettingsError);
		return;
	}

	// 2. 설정이 정상이면 지급 대상을 검사한다.
	ANF_PlayerState* Recipient = InitialRecipient.Get();

	if (!IsValid(Recipient) ||Recipient->GetWorld() != GetWorld() ||!NFGameState->PlayerArray.Contains(Recipient))
	{
		CompleteInitialAssignmentByDrop();
		return;
	}

	UAbilitySystemComponent* ASC =Recipient->GetAbilitySystemComponent();

	// 3. 완전 사망한 대상이면 대체 드롭한다.
	if (IsValid(ASC) &&ASC->HasMatchingGameplayTag(NFTag_LifeState::State_Life_Dead))
	{
		CompleteInitialAssignmentByDrop();
		return;
	}

	++InitialAssignmentAttempts;

	APawn* Pawn = Recipient->GetPawn();

	const bool bAbilitySystemReady =IsValid(ASC) &&IsValid(Pawn) &&ASC->GetAvatarActor() == Pawn;

	if (bAbilitySystemReady && TransferKey(Recipient))
	{
		// GE 적용 중 매치가 종료됐을 수 있다.
		if (bKeyCleanupComplete ||InitState != ENF_KeyInitState::Retrying)return;

		InitState = ENF_KeyInitState::Completed;
		InitialRecipient.Reset();

		GetWorld()->GetTimerManager().ClearTimer(InitialAssignmentRetryTimerHandle);

		return;
	}

	if (bKeyCleanupComplete ||InitState != ENF_KeyInitState::Retrying)return;
	
	// 4. 재시도 횟수를 소진하면 대체 드롭한다.
	if (InitialAssignmentAttempts >=FMath::Max(1, MaxInitialAssignmentAttempts))
	{
		CompleteInitialAssignmentByDrop();
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(
		InitialAssignmentRetryTimerHandle,
		this,
		&ThisClass::TryInitialAssignment,
		FMath::Max(0.1f, InitialAssignmentRetryInterval),
		false);
}

void UNF_EscapeKeyComponent::FailInitialAssignment(const FString& Reason)
{
	if (!HasServerAuthority() ||bKeyCleanupComplete ||InitState != ENF_KeyInitState::Retrying) return;

	GetWorld()->GetTimerManager().ClearTimer(InitialAssignmentRetryTimerHandle);

	InitState = ENF_KeyInitState::Failed;
	InitialRecipient.Reset();

	UE_LOG(LogTemp, Error, TEXT("[EscapeKey] %s"), *Reason);
	
	OnInitialKeyAssignmentFailed.Broadcast(Reason);
}

bool UNF_EscapeKeyComponent::IsEscapePhase() const
{
	const UWorld* World = GetWorld();

	if (!IsValid(World)){return false;}

	const ANF_GameState* NFGameState =World->GetGameState<ANF_GameState>();

	return IsValid(NFGameState) &&NFGameState->GetMatchPhase() == ENF_MatchPhase::Escape;
}

bool UNF_EscapeKeyComponent::CanReceiveKey(ANF_PlayerState* PlayerState, bool bRequireAlive) const
{
	if (!IsValid(PlayerState) ||PlayerState->GetWorld() != GetWorld()){return false;}

	const ANF_GameState* NFGameState =GetWorld()->GetGameState<ANF_GameState>();

	if (!IsValid(NFGameState) ||!NFGameState->PlayerArray.Contains(PlayerState)){return false;}

	APawn* Pawn = PlayerState->GetPawn();

	UAbilitySystemComponent* ASC =PlayerState->GetAbilitySystemComponent();

	if (!IsValid(Pawn) ||!IsValid(ASC) ||ASC->GetAvatarActor() != Pawn){return false;}

	if (ASC->HasMatchingGameplayTag(NFTag_LifeState::State_Life_Dead)){return false;}

	// 바닥 열쇠를 직접 줍는 행위는 기절 중에도 제한한다.
	if (bRequireAlive &&ASC->HasMatchingGameplayTag(NFTag_LifeState::State_Life_Downed)){return false;}

	return true;
}

bool UNF_EscapeKeyComponent::CreateDroppedKeyAt(const FVector& Location)
{
	if (!HasServerAuthority() ||
		bKeyCleanupComplete ||
		bChangingKeyHolder ||
		!IsEscapePhase() ||
		!KeyPickupClass ||
		Location.ContainsNaN())
	{
		return false;
	}

	if (KeyState != ENF_KeyState::Unassigned &&KeyState != ENF_KeyState::Held){return false;}

	if (IsValid(DroppedKeyActor)){return false;}

	TGuardValue<bool> ChangeGuard(bChangingKeyHolder, true);

	// 정적 바닥을 찾아 공중·지형 내부 배치를 피한다.
	FHitResult GroundHit;
	FCollisionQueryParams Params;

	if (ANF_PlayerState* Holder = KeyHolder.Get())
	{
		if (APawn* Pawn = Holder->GetPawn())
		{
			Params.AddIgnoredActor(Pawn);
		}
	}

	const FVector TraceStart = Location + FVector(0.0f, 0.0f, 100.0f);
	const FVector TraceEnd = Location - FVector(0.0f, 0.0f, 1500.0f);

	if (!GetWorld()->LineTraceSingleByChannel(GroundHit,TraceStart,TraceEnd,ECC_WorldStatic,Params)){return false;}

	const FVector SpawnLocation =GroundHit.ImpactPoint + FVector(0.0f, 0.0f, 35.0f);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride =ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ANF_EscapeKey* NewPickup =
		GetWorld()->SpawnActor<ANF_EscapeKey>(KeyPickupClass,SpawnLocation,FRotator::ZeroRotator,SpawnParameters);

	if (!IsValid(NewPickup)){return false;}

	NewPickup->InitializePickup(this);

	UAbilitySystemComponent* PreviousASC =KeyHolderEffectASC.Get();

	const FActiveGameplayEffectHandle PreviousHandle =KeyHolderEffectHandle;

	// 생성 성공 이후에만 상태를 확정
	DroppedKeyActor = NewPickup;
	KeyState = ENF_KeyState::Dropped;

	KeyHolder.Reset();
	KeyHolderEffectASC.Reset();
	KeyHolderEffectHandle.Invalidate();

	GetWorld()->GetTimerManager().ClearTimer(HolderMonitorTimerHandle);

	if (IsValid(PreviousASC) && PreviousHandle.IsValid())
	{
		PreviousASC->RemoveActiveGameplayEffect(PreviousHandle);
	}

	// GE 제거 콜백에서 정리됐을 수 있다.
	if (bKeyCleanupComplete ||DroppedKeyActor.Get() != NewPickup ||!IsValid(NewPickup)){return false;}

	NewPickup->ActivatePickup();
	PublishKeyState();
	return !bKeyCleanupComplete && KeyState == ENF_KeyState::Dropped && DroppedKeyActor.Get() == NewPickup;
}

void UNF_EscapeKeyComponent::CompleteInitialAssignmentByDrop()
{
	if (bKeyCleanupComplete ||InitState != ENF_KeyInitState::Retrying)return;

	if (!bHasInitialDropLocation ||!CreateDroppedKeyAt(InitialDropLocation))
	{
		FailInitialAssignment(TEXT("최초 지급 대체용 바닥 열쇠 생성에 실패했습니다."));
		return;
	}

	if (bKeyCleanupComplete ||InitState != ENF_KeyInitState::Retrying)return;

	InitState = ENF_KeyInitState::Completed;
	InitialRecipient.Reset();

	GetWorld()->GetTimerManager().ClearTimer(InitialAssignmentRetryTimerHandle);
}

void UNF_EscapeKeyComponent::StartHolderMonitoring()
{
	ANF_PlayerState* Holder = KeyHolder.Get();

	if (!IsValid(Holder) || !IsValid(Holder->GetPawn()))return;

	LastHolderLocation = Holder->GetPawn()->GetActorLocation();

	GetWorld()->GetTimerManager().SetTimer(
		HolderMonitorTimerHandle,
		this,
		&ThisClass::MonitorHolder,
		0.1f,
		true);
}

void UNF_EscapeKeyComponent::MonitorHolder()
{
	if (!HasServerAuthority() ||bKeyCleanupComplete ||KeyState != ENF_KeyState::Held ||bChangingKeyHolder)return;

	ANF_PlayerState* Holder = KeyHolder.Get();
	APawn* Pawn = nullptr;

	if (IsValid(Holder))
	{
		Pawn = Holder->GetPawn();
	}

	if (IsValid(Pawn))
	{
		LastHolderLocation = Pawn->GetActorLocation();
	}

	UAbilitySystemComponent* ASC = KeyHolderEffectASC.Get();

	const bool bDead = IsValid(ASC) &&ASC->HasMatchingGameplayTag(NFTag_LifeState::State_Life_Dead);

	if (!bHolderLeaving && IsValid(Holder) && IsValid(Pawn) && !bDead)
	{
		if (LastPublishedHolderPawn.Get() != Pawn)
		{
			PublishKeyState();
		}
		return;
	}
	
	if (!DropKey())
	{
		OnKeyOperationFailed.Broadcast(TEXT("열쇠 보유자 사망·이탈 후 드롭 생성에 실패했습니다."));
	}
}

bool UNF_EscapeKeyComponent::HasServerAuthority() const
{
	const AActor* OwnerActor = GetOwner();

	return IsValid(OwnerActor) && OwnerActor->HasAuthority();
}
