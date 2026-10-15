
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "UI/ViewModel/Base/NF_GASViewModelBase.h"
#include "NF_BuffDebuffViewModel.generated.h"

class UTexture2D;
class UWorld;

UCLASS(BlueprintType, Blueprintable)
class PROJECT_NF_API UNF_BuffDebuffViewModel : public UNF_GASViewModelBase
{
    GENERATED_BODY()

protected:

    virtual void BindAbilitySystemDelegates() override;
    virtual void RefreshFromAbilitySystem() override;

    // TODO: 열쇠 담당자가 확정한 '열쇠 보유 상태' 태그를 설정.
    // 열쇠를 떨어뜨리면 이 태그도 제거되어야 한다.
    UPROPERTY(EditDefaultsOnly, Category = "BuffDebuff|Tags")
    FGameplayTag KeyHolderTag;

    // TODO: GE 담당자가 확정한 '최대 실드 감소' 태그를 설정.
    // 남은 시간 조회를 위해 해당 GE의 owning tag에도 포함되어야 한다.
    UPROPERTY(EditDefaultsOnly, Category = "BuffDebuff|Tags")
    FGameplayTag ShieldPenaltyTag;

    // 아이콘은 이 ViewModel의 블루프린트에서 지정.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BuffDebuff|Icons")
    TObjectPtr<UTexture2D> CooldownReductionIcon;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BuffDebuff|Icons")
    TObjectPtr<UTexture2D> DamageReductionIcon;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BuffDebuff|Icons")
    TObjectPtr<UTexture2D> MaxShieldPenaltyIcon;

private:
    void HandleEffectTagChanged(const FGameplayTag ChangedTag, int32 NewCount);

    // true일 때 열쇠소지로 인한 쿨타임 감소·피격 데미지 감소 아이콘 둘 다 표시.
    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "BuffDebuff|State",meta = (AllowPrivateAccess = "true"))
    bool bHasKeyBuff = false;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "BuffDebuff|State",meta = (AllowPrivateAccess = "true"))
    bool bHasMaxShieldPenalty = false;
    
};