#include "UI/ViewModel/GamePlay/NF_MinimapViewModel.h"

#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpectatorPawn.h"
#include "Player/Controller/NF_PlayerController.h"
#include "Player/State/NF_PlayerState.h"
#include "UI/Minimap/NF_MinimapRevealRegistry.h"

void UNF_MinimapViewModel::OnInitializeViewModel()
{
    Super::OnInitializeViewModel();

    RefreshSnapshot();
    StartRefresh();
}

void UNF_MinimapViewModel::OnViewModelContextChanged()
{
    StopRefresh();

    Super::OnViewModelContextChanged();

    RefreshSnapshot();
    StartRefresh();
}

void UNF_MinimapViewModel::OnDeinitializeViewModel()
{
    StopRefresh();

    Snapshot = FNF_MinimapSnapshot{};
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Snapshot);

    Super::OnDeinitializeViewModel();
}

void UNF_MinimapViewModel::StartRefresh()
{
    StopRefresh();

    APlayerController* Controller =GetViewModelContext().PlayerController.Get();

    UWorld* World = nullptr;

    if (IsValid(Controller))
    {
        World = Controller->GetWorld();
    }

    if (!IsValid(World) || !Controller->IsLocalController())return;
    

    RefreshWorld = World;

    World->GetTimerManager().SetTimer(
        RefreshTimerHandle,
        this,
        &ThisClass::RefreshSnapshot,
        0.05f,
        true);
}

void UNF_MinimapViewModel::StopRefresh()
{
    if (UWorld* World = RefreshWorld.Get())
    {
        World->GetTimerManager().ClearTimer(RefreshTimerHandle);
    }

    RefreshWorld.Reset();
}

void UNF_MinimapViewModel::RefreshSnapshot()
{
    FNF_MinimapSnapshot Next;

    APlayerController* Controller =GetViewModelContext().PlayerController.Get();

    UWorld* World = nullptr;

    if (IsValid(Controller))
    {
        World = Controller->GetWorld();
    }

    if (IsValid(Controller) &&Controller->IsLocalController() &&IsValid(World))
    {
        APawn* LocalPawn = Controller->GetPawn();

        const ANF_PlayerState* LocalState =Controller->GetPlayerState<ANF_PlayerState>();

        if (!IsValid(LocalState) && IsValid(LocalPawn))
        {
            LocalState = LocalPawn->GetPlayerState<ANF_PlayerState>();
        }

        if (IsValid(LocalPawn))
        {
            Next.bHasView = true;
            Next.ViewLocation = LocalPawn->GetActorLocation();
            Next.ViewYaw = Controller->GetControlRotation().Yaw;
        }

        int32 LocalTeamID = INDEX_NONE;

        if (IsValid(LocalState))
        {
            LocalTeamID = LocalState->NF_TeamID;
        }

        // 실제 Pawn 위치로 이미 표시한 팀원.
        // 공개 목록의 팀원과 중복 표시하지 않기 위해 사용한다.
        TSet<const APlayerState*> DisplayedTeammates;

        AGameStateBase* GameState = World->GetGameState();

        if (LocalTeamID >= 1 && IsValid(GameState))
        {
            for (const auto& StateEntry : GameState->PlayerArray)
            {
                ANF_PlayerState* MemberState =Cast<ANF_PlayerState>(StateEntry.Get());

                if (!IsValid(MemberState) ||MemberState == LocalState ||MemberState->NF_TeamID != LocalTeamID)continue;
                
                APawn* MemberPawn = MemberState->GetPawn();

                if (!IsValid(MemberPawn) ||
                    MemberPawn->IsActorBeingDestroyed() ||
                    MemberPawn->IsA<ASpectatorPawn>() ||
                    MemberPawn->GetWorld() != World)
                {
                    continue;
                }

                DisplayedTeammates.Add(MemberState);

                FNF_MinimapMarker& Marker =Next.Markers.AddDefaulted_GetRef();

                Marker.Id = FName(*FString::Printf(TEXT("Team_%s"),*MemberState->GetName()));

                Marker.WorldLocation = MemberPawn->GetActorLocation();
                Marker.Type = ENF_MinimapMarkerType::Teammate;
                Marker.Size = 20.0f;
            }
        }

        ANF_PlayerController* NFController =Cast<ANF_PlayerController>(Controller);

        UNF_MinimapRevealRegistry* Registry = nullptr;

        if (IsValid(NFController))
        {
            Registry = NFController->GetMinimapRevealRegistry();
        }

        if (IsValid(Registry))
        {
            for (const FNF_MinimapRevealMarker& Reveal :Registry->GetVisibleMarkers())
            {
                if (Reveal.TargetId.IsNone())continue;

                const bool bTeammate =LocalTeamID >= 1 &&Reveal.PlayerTeamID == LocalTeamID;

                // 기존 미니맵의 중복 제거 규칙 유지:
                // 열쇠 목적지 표시는 팀원 마커와 별도로 유지한다.
                if (bTeammate &&
                    Reveal.Kind == ENF_MinimapRevealKind::Clairvoyance &&
                    DisplayedTeammates.Contains(Reveal.PlayerState.Get()))
                {
                    continue;
                }

                FNF_MinimapMarker& Marker =Next.Markers.AddDefaulted_GetRef();

                Marker.Id = 
                    FName(*FString::Printf(TEXT("Reveal_%d_%s"),static_cast<int32>(Reveal.Kind),*Reveal.TargetId.ToString()));

                Marker.WorldLocation = Reveal.WorldLocation;
                Marker.Size = 20.0f;

                if (Reveal.Kind == ENF_MinimapRevealKind::EscapeKey)
                {
                    Marker.Type = ENF_MinimapMarkerType::Objective;
                }
                else
                {
                    if (bTeammate)
                    {
                        Marker.Type = ENF_MinimapMarkerType::Teammate;
                    }
                    else
                    {
                        Marker.Type = ENF_MinimapMarkerType::RevealedEnemy;
                    }
                }
            }
        }
    }

    // 배열 비교 연산자를 추가하지 않고 전체 Snapshot을 전달하는 예시.
    Snapshot = MoveTemp(Next);
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Snapshot);
}