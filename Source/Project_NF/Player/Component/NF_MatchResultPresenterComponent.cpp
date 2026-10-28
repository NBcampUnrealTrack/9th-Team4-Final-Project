#include "Player/Component/NF_MatchResultPresenterComponent.h"

#include "Core/NF_GameState.h"
#include "Data/Enum/NF_MatchPhase.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Player/State/NF_PlayerState.h"
#include "UI/SubSystem/NF_UIManagerSubsystem.h"
#include "UI/Widget/NF_MatchResultWidget.h"

UNF_MatchResultPresenterComponent::UNF_MatchResultPresenterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = 0.25f;
	SetIsReplicatedByDefault(false);
}

void UNF_MatchResultPresenterComponent::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* Controller = Cast<APlayerController>(GetOwner());
	if (!IsValid(Controller) || !Controller->IsLocalController()) return;

	const UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetNetMode() == NM_DedicatedServer) return;
	

	if (!MatchResultWidgetClass || MatchResultWidgetClass->HasAnyClassFlags(CLASS_Abstract))
	{
		UE_LOG(LogTemp, Error, TEXT("[NF MatchResult] %s의 결과 WBP 클래스를 지정해야 합니다."), *GetNameSafe(GetOwner()));
		return;
	}

	SetComponentTickEnabled(true);
	RefreshResultScreen();
}

void UNF_MatchResultPresenterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetComponentTickEnabled(false);
	ReleaseResultWidget();
	ObservedGameState.Reset();
	Super::EndPlay(EndPlayReason);
}

void UNF_MatchResultPresenterComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshResultScreen();
}

void UNF_MatchResultPresenterComponent::RefreshResultScreen()
{
	APlayerController* Controller = Cast<APlayerController>(GetOwner());
	UWorld* World = GetWorld();
	if (!IsValid(Controller) || !Controller->IsLocalController() || !IsValid(World)) return;

	ANF_GameState* GameState = World->GetGameState<ANF_GameState>();
	// 복제 대상이 아직 준비되지 않았다면 다음 확인에서 재시도한다.
	if (!IsValid(GameState)) return;

	if (ObservedGameState.Get() != GameState)
	{
		ReleaseResultWidget();
		ObservedGameState = GameState;
		LastPresentedResult = FNF_MatchResult();
		LastPresentedTeamId = INDEX_NONE;
		bHasPresentedResult = false;
		bReturnPending = false;
		bPresentationCompleted = false;
	}

	if (bPresentationCompleted) return;

	const FNF_MatchState MatchState = GameState->GetMatchState();
	if (MatchState.Phase != ENF_MatchPhase::Finished) return;

	const FNF_MatchResult& Result = MatchState.Result;
	if (Result.Outcome == ENF_MatchOutcome::None) return;
	
	if (Result.Outcome == ENF_MatchOutcome::Victory && Result.WinningTeamId <= 0) return;

	int32 LocalTeamId = INDEX_NONE;
	if (const ANF_PlayerState* PlayerState = Controller->GetPlayerState<ANF_PlayerState>())
	{
		LocalTeamId = PlayerState->NF_TeamID;
	}

	// UIManager는 결과 위젯을 Root의 Overlay 자식으로 관리한다.
	const bool bWidgetAttached = IsValid(ResultWidget) && ResultWidget->GetParent() != nullptr;
	if (bWidgetAttached && !HasResultChanged(Result, LocalTeamId)) return;

	ULocalPlayer* LocalPlayer = Controller->GetLocalPlayer();
	if (!IsValid(LocalPlayer)) return;
	
	UNF_UIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UNF_UIManagerSubsystem>();
	if (!IsValid(UIManager)) return;
	
	UNF_MatchResultWidget* NewWidget = UIManager->ShowMatchResult(MatchResultWidgetClass, Result, LocalTeamId);
	if (!IsValid(NewWidget)) return;

	if (ResultWidget.Get() != NewWidget)
	{
		if (IsValid(ResultWidget))
		{
			ResultWidget->OnReturnRequested.RemoveDynamic(this, &ThisClass::HandleReturnRequested);
		}
		ResultWidget = NewWidget;
	}
	ResultWidget->OnReturnRequested.AddUniqueDynamic(this, &ThisClass::HandleReturnRequested);
	ResultWidget->SetReturnPending(bReturnPending);

	LastPresentedResult = Result;
	LastPresentedTeamId = LocalTeamId;
	bHasPresentedResult = true;
}

bool UNF_MatchResultPresenterComponent::HasResultChanged(const FNF_MatchResult& Result, const int32 LocalTeamId) const
{
	if (!bHasPresentedResult) {return true;}
	
	return LastPresentedTeamId != LocalTeamId ||
		LastPresentedResult.Outcome != Result.Outcome ||
		LastPresentedResult.EndReason != Result.EndReason ||
		LastPresentedResult.WinningTeamId != Result.WinningTeamId ||
		LastPresentedResult.Message != Result.Message;
}

void UNF_MatchResultPresenterComponent::HandleReturnRequested()
{
	if (bReturnPending || bPresentationCompleted) return;
	
	if (!OnReturnToMatchmakingRequested.IsBound())
	{
		UE_LOG(LogTemp, Warning, TEXT("[NF MatchResult] 실제 매칭 복귀 처리자가 연결되지 않았습니다."));
		NotifyReturnFailed();
		return;
	}

	// 콜백에서 동기적으로 실패를 알리거나 맵을 이동할 수 있으므로 먼저 기록한다.
	bReturnPending = true;
	OnReturnToMatchmakingRequested.Broadcast();
}

void UNF_MatchResultPresenterComponent::NotifyReturnFailed()
{
	if (bPresentationCompleted) return;
	
	bReturnPending = false;
	if (IsValid(ResultWidget))
	{
		ResultWidget->SetReturnPending(false);
	}
}

void UNF_MatchResultPresenterComponent::NotifyReturnCompleted()
{
	bPresentationCompleted = true;
	bReturnPending = false;
	ReleaseResultWidget();
}

void UNF_MatchResultPresenterComponent::ReleaseResultWidget()
{
	if (!IsValid(ResultWidget))
	{
		ResultWidget = nullptr;
		return;
	}
	ResultWidget->OnReturnRequested.RemoveDynamic(this, &ThisClass::HandleReturnRequested);

	if (APlayerController* Controller = Cast<APlayerController>(GetOwner()))
	{
		if (ULocalPlayer* LocalPlayer = Controller->GetLocalPlayer())
		{
			if (UNF_UIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UNF_UIManagerSubsystem>())
			{
				UIManager->RemoveWidget(ResultWidget.Get());
			}
		}
	}
	ResultWidget = nullptr;
}
