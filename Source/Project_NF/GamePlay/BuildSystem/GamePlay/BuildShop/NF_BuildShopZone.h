#pragma once

#include "CoreMinimal.h"
#include "Data/Interface/NF_InteractionInterface.h"
#include "GameFramework/Actor.h"
#include "Data/Enum/NF_BuildShopType.h"
#include "NF_BuildShopZone.generated.h"

class UBoxComponent;

/** Place at spawn/base or field shops. Area eligibility is checked again on the server. */
UCLASS()
class PROJECT_NF_API ANF_BuildShopZone : public AActor, public INF_InteractionInterface
{
	GENERATED_BODY()

public:
	ANF_BuildShopZone();
	virtual void NF_Interaction(const APlayerController* InteractedController) override;

	UFUNCTION(BlueprintPure, Category = "NF|BuildShop")
	bool CanBuild(AActor* PlayerActor) const;

	/** @Owner: 황준현 | 2026-10-15. 사용 권한과 별개로 실제 거리 범위만 검사한다. */
	UFUNCTION(BlueprintPure, Category = "NF|BuildShop")
	bool IsActorInRange(AActor* PlayerActor) const;

	/** Team ownership and match phase restrictions belong to their respective systems. */
	UFUNCTION(BlueprintNativeEvent, Category = "NF|BuildShop")
	bool IsPlayerAllowed(AActor* PlayerActor) const;
	virtual bool IsPlayerAllowed_Implementation(AActor* PlayerActor) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|BuildShop")
	void SetShopEnabled(bool bEnabled);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|BuildShop")
	TObjectPtr<UBoxComponent> BuildArea;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "NF|BuildShop")
	bool bShopEnabled = true;

	/** Measured from the player to this actor. Interaction trace collision is separate from authorization. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|BuildShop", meta = (ClampMin = "0"))
	float InteractionDistance = 300.f;
	
	// @Owner: 황준현 | 2026-10-19
	// Core의 페이즈별 상점 접근 정책에 전달할 상점 종류.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|BuildShop")
	ENF_BuildShopType ShopType = ENF_BuildShopType::None;

	// @Owner: 황준현 | 2026-10-19
	// TeamBase 상점 소유 팀. INDEX_NONE이면 사용을 허용하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|BuildShop")
	int32 OwnerTeamId = INDEX_NONE;
};
