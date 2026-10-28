#include "Player/Component/NF_MatchResultPresenterComponent.h"

#include "Core/NF_GameState.h"
#include "Core/Match/NF_MatchmakingSubsystem.h"
#include "Data/Enum/NF_MatchPhase.h"
#include "Engine/GameInstance.h"
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
	if (bReturnPending || bPresentationCompleted || bEndingPlay) return;

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
	bEndingPlay = true;
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
	if (bReturnPending || bEndingPlay) return;

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
	if (bReturnPending || bPresentationCompleted || bEndingPlay) return;

	UWorld* World = GetWorld();
	ANF_GameState* GameState = nullptr;
	if (IsValid(World))
	{
		GameState = World->GetGameState<ANF_GameState>();
	}

	// 버튼은 현재 표시 중인 경기의 확정 결과를 사용한다.
	if (!IsValid(GameState) || GameState->GetMatchPhase() != ENF_MatchPhase::Finished)
	{
		NotifyReturnFailed();
		return;
	}

	const FNF_MatchResult Result = GameState->GetMatchState().Result;
	RequestReturnToMatchmaking(Result);
}

void UNF_MatchResultPresenterComponent::RequestReturnToMatchmaking(const FNF_MatchResult& Result)
{
	if (bReturnPending || bPresentationCompleted || bEndingPlay) return;

	APlayerController* Controller = Cast<APlayerController>(GetOwner());
	UWorld* World = GetWorld();
	if (!IsValid(Controller) || !Controller->IsLocalController() ||
		!IsValid(World) || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	UNF_MatchmakingSubsystem* Match = nullptr;
	if (IsValid(GameInstance))
	{
		Match = GameInstance->GetSubsystem<UNF_MatchmakingSubsystem>();
	}
	if (!IsValid(Match))
	{
		UE_LOG(LogTemp, Warning, TEXT("[NF MatchResult] 매칭 Subsystem을 찾을 수 없어 복귀 요청을 거절합니다."));
		NotifyReturnFailed();
		return;
	}

	// 자동 RPC는 GameState 결과 복제보다 먼저 도착할 수 있다.
	// 전달받은 결과를 복사하고 Finished 조회 없이 공통 API에 전달한다.
	const FNF_MatchResult ResultCopy = Result;
	const bool bWasTickEnabled = IsComponentTickEnabled();
	bReturnPending = true;
	SetComponentTickEnabled(false);
	if (IsValid(ResultWidget))
	{
		ResultWidget->SetReturnPending(true);
	}

	// API 내부에서 맵 이동 및 EndPlay가 실행될 수 있다.
	TWeakObjectPtr<UNF_MatchResultPresenterComponent> WeakPresenter(this);
	const bool bAccepted = Match->RequestReturnToMatchmaking(ResultCopy);
	UNF_MatchResultPresenterComponent* Presenter = WeakPresenter.Get();
	if (!IsValid(Presenter) || Presenter->bEndingPlay) return;

	if (bAccepted)
	{
		Presenter->NotifyReturnCompleted();
		return;
	}

	Presenter->NotifyReturnFailed();
	Presenter->SetComponentTickEnabled(bWasTickEnabled);
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
	SetComponentTickEnabled(false);
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
