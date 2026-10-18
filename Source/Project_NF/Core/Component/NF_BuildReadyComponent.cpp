#include "Core/Component/NF_BuildReadyComponent.h"

#include "AbilitySystemComponent.h"
#include "GAS/Tags/NFTag_Build.h"
#include "GameFramework/Actor.h"
#include "Player/State/NF_PlayerState.h"

UNF_BuildReadyComponent::UNF_BuildReadyComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UNF_BuildReadyComponent::HasServerAuthority() const
{
    const AActor* Owner = GetOwner();

    return IsValid(Owner) && Owner->HasAuthority();
}

void UNF_BuildReadyComponent::RegisterPlayer(ANF_PlayerState* PlayerState)
{
    if (!HasServerAuthority()) return;
    if (!IsValid(PlayerState)) return;
    if (PlayerState->GetWorld() != GetWorld()) return;

    const TWeakObjectPtr<ANF_PlayerState> Key(PlayerState);

    //  중복 구독을 방지
    if (BuildReadyBindings.Contains(Key)) return;
    
    UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();

    if (!IsValid(ASC))
    {
        UE_LOG(LogTemp,Error,TEXT("빌드 준비 상태 등록 실패: %s의 ASC 없음"),*GetNameSafe(PlayerState));
        return;
    }

    FNF_BuildReadyBinding Binding;
    Binding.AbilitySystem = ASC;

    Binding.DelegateHandle =
        ASC->RegisterGameplayTagEvent(
            NFTag_Build::State_Build_Ready,EGameplayTagEventType::NewOrRemoved).AddUObject(
            this,&ThisClass::HandleBuildReadyTagChanged,Key);

    BuildReadyBindings.Add(Key, Binding);

    // 바인딩 이전에 부여된 태그도 반영
    PlayerBuildReadyStates.Add(Key,ASC->HasMatchingGameplayTag(NFTag_Build::State_Build_Ready));
}

void UNF_BuildReadyComponent::UnregisterPlayer(ANF_PlayerState* PlayerState)
{
    if (!HasServerAuthority()) return;
    if (!IsValid(PlayerState)) return;

    const TWeakObjectPtr<ANF_PlayerState> Key(PlayerState);

    if (FNF_BuildReadyBinding* Binding =BuildReadyBindings.Find(Key))
    {
        if (UAbilitySystemComponent* ASC =Binding->AbilitySystem.Get())
        {
            ASC->RegisterGameplayTagEvent(
                NFTag_Build::State_Build_Ready,EGameplayTagEventType::NewOrRemoved).Remove(Binding->DelegateHandle);
        }
    }

    BuildReadyBindings.Remove(Key);
    PlayerBuildReadyStates.Remove(Key);
}

void UNF_BuildReadyComponent::HandleBuildReadyTagChanged(
    const FGameplayTag Tag,
    const int32 NewCount,
    const TWeakObjectPtr<ANF_PlayerState> PlayerState)
{
    if (!HasServerAuthority()) return;
    if (!PlayerState.IsValid()) return;
    if (Tag != NFTag_Build::State_Build_Ready) return;
    
    if (bool* CachedState =PlayerBuildReadyStates.Find(PlayerState))
    {
        *CachedState = NewCount > 0;
    }

    // 준비 상태만 갱신한다.
    // 전원이 준비되어도 제한시간 전에는 페이즈를 전환하지 않는다.
}

void UNF_BuildReadyComponent::RefreshReadyStates()
{
    for (auto& Pair : PlayerBuildReadyStates)
    {
        ANF_PlayerState* PlayerState = Pair.Key.Get();

        UAbilitySystemComponent* ASC = nullptr;

        if (IsValid(PlayerState))
        {
            ASC = PlayerState->GetAbilitySystemComponent();
        }

        Pair.Value =IsValid(ASC) &&ASC->HasMatchingGameplayTag(NFTag_Build::State_Build_Ready);
    }
}

bool UNF_BuildReadyComponent::AreAllPlayersBuildReady(const int32 RequiredPlayerCount) const
{
    if (RequiredPlayerCount <= 0)  {return false;}
    if (PlayerBuildReadyStates.Num() != RequiredPlayerCount) {return false;}

    for (const auto& Pair : PlayerBuildReadyStates)
    {
        if (!Pair.Key.IsValid() || !Pair.Value) {return false;}
    }

    return true;
}

bool UNF_BuildReadyComponent::ResolveUnreadyPlayers(
    const int32 RequiredPlayerCount)
{
    if (!HasServerAuthority())  {return false;}
    if (bResolvingBuilds)  {return false;}
    if (RequiredPlayerCount <= 0) {return false;}

    TGuardValue<bool> ResolveGuard(bResolvingBuilds, true);

    // 마감 시점의 실제 서버 상태를 읽는다.
    RefreshReadyStates();

    // 외부 콜백에서 Map이 변경될 수 있으므로 대상 목록을 복사한다.
    TArray<TWeakObjectPtr<ANF_PlayerState>> NotReadyPlayers;

    for (const auto& Pair : PlayerBuildReadyStates)
    {
        if (Pair.Key.IsValid() && !Pair.Value)
        {
            NotReadyPlayers.Add(Pair.Key);
        }
    }

    bool bAllRequestsSucceeded = true;

    for (const TWeakObjectPtr<ANF_PlayerState>& Player :NotReadyPlayers)
    {
        ANF_PlayerState* PlayerState = Player.Get();

        // 콜백 처리 중 퇴장했거나 등록 해제된 플레이어.
        if (!IsValid(PlayerState) || !PlayerBuildReadyStates.Contains(Player))
        {
            bAllRequestsSucceeded = false;
            continue;
        }

        // 앞선 콜백에서 준비 상태가 바뀌었을 가능성도 확인한다.
        UAbilitySystemComponent* ASC =PlayerState->GetAbilitySystemComponent();

        if (IsValid(ASC) &&ASC->HasMatchingGameplayTag(NFTag_Build::State_Build_Ready)) continue;
        

        if (!ApplyFallbackBuild.IsBound())
        {
            UE_LOG(LogTemp,Error,TEXT("기본 빌드 적용 실패: 연동 델리게이트 미바인딩"));

            bAllRequestsSucceeded = false;
            break;
        }

        if (!ApplyFallbackBuild.Execute(PlayerState))
        {
            UE_LOG(LogTemp,Error,TEXT("기본 빌드 적용 실패: %s"),*GetNameSafe(PlayerState));

            bAllRequestsSucceeded = false;
        }
    }

    // 기본 빌드 적용 결과까지 서버 ASC에서 최종 확인한다.
    RefreshReadyStates();

    return bAllRequestsSucceeded && AreAllPlayersBuildReady(RequiredPlayerCount);
}

void UNF_BuildReadyComponent::ClearBindings()
{
    for (const auto& Pair : BuildReadyBindings)
    {
        const FNF_BuildReadyBinding& Binding = Pair.Value;

        if (UAbilitySystemComponent* ASC =Binding.AbilitySystem.Get())
        {
            ASC->RegisterGameplayTagEvent(
                NFTag_Build::State_Build_Ready,EGameplayTagEventType::NewOrRemoved).Remove(Binding.DelegateHandle);
        }
    }

    BuildReadyBindings.Empty();
    PlayerBuildReadyStates.Empty();
    ApplyFallbackBuild.Unbind();
}

void UNF_BuildReadyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearBindings();

    Super::EndPlay(EndPlayReason);
}