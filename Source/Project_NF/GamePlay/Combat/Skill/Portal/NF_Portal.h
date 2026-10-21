#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GamePlay/Combat/Components/NF_DeployableComponent.h"
#include "NF_Portal.generated.h"

class ACharacter;
class UBoxComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class ANF_PortalView;
class ULocalPlayer;

// @owner: 이정준 (구현: Codex) | 2026-10-15
// 목적: 단방향 포탈 쌍의 서버 접촉 이동·공간 검사·종료 연결. 수명은 공통 Deployable 사용.
UCLASS(Blueprintable)
class PROJECT_NF_API ANF_Portal : public AActor
{
	GENERATED_BODY()
public:
	ANF_Portal();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	bool HasValidSettings() const;
	void InitializePortal(ANF_Portal* Other, bool bIsExit, ACharacter* Caster, UAbilitySystemComponent* CasterASC = nullptr);
	bool CanPlaceFor(ACharacter* Character) const;
	bool FindArrivalLocation(ACharacter* Character, FVector& OutLocation) const;
	bool StartPortal();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Portal")
	void ClosePortalPair();
	UFUNCTION(BlueprintPure, Category = "NF|Portal")
	bool IsExitPortal() const { return bExitPortal; }
	UFUNCTION(BlueprintPure, Category = "NF|Portal")
	bool IsPortalActive() const { return bActive; }
	UFUNCTION(BlueprintPure, Category = "NF|Portal")
	ANF_Portal* GetLinkedPortal() const { return LinkedPortal; }
	FTransform GetViewTransform() const;
	UFUNCTION(BlueprintPure, Category = "NF|Portal|Visual")
	float GetVisualScaleAlpha() const;
	bool IsPortalVisualVisible() const { return bActive || GetVisualScaleAlpha() > KINDA_SMALL_NUMBER; }
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Portal")
	TObjectPtr<UBoxComponent> EntranceTrigger;
	// Attach decorative meshes here. Gameplay collision and arrival direction are independent.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Portal|Visual")
	TObjectPtr<USceneComponent> PortalVisualRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Portal")
	TObjectPtr<UStaticMeshComponent> PortalMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Portal")
	TObjectPtr<UNF_DeployableComponent> Deployment;
	// 출구 앞의 바닥을 찾고 그 위에 이용자의 캡슐을 배치한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal", meta = (ClampMin = "1.0", Units = "cm"))
	float ExitForwardOffset = 160.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal", meta = (ClampMin = "1.0", Units = "cm"))
	float ArrivalSearchHeight = 150.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal", meta = (ClampMin = "1.0", Units = "cm"))
	float ArrivalSearchDepth = 300.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal", meta = (ClampMin = "1.0", Units = "cm"))
	float GroundClearance = 5.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal", meta = (ClampMin = "0.1", Units = "s"))
	float ReentryCooldown = 0.75f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal")
	bool bOnlyCasterCanUse = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal")
	bool bAlignViewToExit = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal")
	bool bPreserveVelocity = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal|Visual")
	TObjectPtr<UMaterialInterface> EntranceMaterial;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal|Visual")
	TObjectPtr<UMaterialInterface> ExitMaterial;
	// @Owner: 이정준 (구현: Codex) | 2026-10-21
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal|Visual", meta = (ClampMin = "0.0", Units = "s"))
	float OpenScaleDuration = 0.3f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal|Visual", meta = (ClampMin = "0.0", Units = "s"))
	float CloseScaleDuration = 0.3f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal|View")
	bool bEnablePortalView = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal|View")
	TSubclassOf<ANF_PortalView> PortalViewClass;
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "NF|Portal|Visual")
	void PortalStateChanged(bool bIsExit, bool bIsActive);
private:
	friend class FNF_PortalVisualTest;
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* Component, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void HandleDeploymentEnded(ENF_DeploymentEndReason Reason);
	UFUNCTION()
	void OnRep_PortalState();
	void RefreshPortalState();
	void InitializeVisualRoot();
	void UpdateVisualScale();
	double GetVisualServerTime() const;
	void RetryOverlappingPlayers();
	void RefreshLocalViews();
	void ClearLocalViews();
	bool TryTeleport(ACharacter* Character);
	UPROPERTY(ReplicatedUsing = OnRep_PortalState)
	TObjectPtr<ANF_Portal> LinkedPortal;
	UPROPERTY(ReplicatedUsing = OnRep_PortalState)
	bool bExitPortal = false;
	UPROPERTY(ReplicatedUsing = OnRep_PortalState)
	bool bActive = false;
	UPROPERTY(ReplicatedUsing = OnRep_PortalState)
	double VisualTransitionTime = 0.0;
	UPROPERTY(ReplicatedUsing = OnRep_PortalState)
	float ClosingStartAlpha = 0.0f;
	FQuat VisualBaseRotation = FQuat::Identity;
	FVector VisualBaseScale = FVector::OneVector;
	FVector ViewAnchorLocal = FVector::ZeroVector;
	bool bVisualRootInitialized = false;
	TWeakObjectPtr<ACharacter> SourceCharacter;
	TMap<TWeakObjectPtr<ACharacter>, double> Cooldowns;
	FTimerHandle RetryTimer;
	bool bClosing = false;
	TMap<TWeakObjectPtr<ULocalPlayer>, TWeakObjectPtr<ANF_PortalView>> LocalViews;
};
