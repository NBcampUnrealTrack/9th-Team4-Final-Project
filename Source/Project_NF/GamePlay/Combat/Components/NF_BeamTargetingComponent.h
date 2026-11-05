#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NF_BeamTargetingComponent.generated.h"

class UStaticMesh;
class UMaterialInterface;
class UStaticMeshComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class UDecalComponent;
struct FHitResult;

// @Owner: 스킬 담당 (구현: Codex) | 2026-10-14
// 목적: 서버 조준으로 빔의 시작/끝을 계산하고 복제된 결과를 클라이언트에서 표시한다.
UCLASS(ClassGroup = (NF), BlueprintType, meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_BeamTargetingComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UNF_BeamTargetingComponent();
	bool CalculateBeam(AActor* Avatar, FVector& Start, FVector& End, FHitResult* OutHit = nullptr) const;
	void ShowBeam(FVector Start, FVector End);
	void UpdateImpactDecal(bool bSurfaceHit, FVector Location, FVector Normal);
	void HideBeam();
	float GetHalfWidth() const { return HalfWidth; }
	bool HasValidSettings() const { return FMath::IsFinite(Range) && Range > 0.0f && FMath::IsFinite(HalfWidth) && HalfWidth > 0.0f && !ViewOffset.ContainsNaN(); }
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam", meta = (ClampMin = "1.0"))
	float Range = 2000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam", meta = (ClampMin = "0.1"))
	float HalfWidth = 20.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam")
	FVector ViewOffset = FVector::ZeroVector;
	// 관통 빔은 벽만 Block하는 채널을 지정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
	// 지정하면 메시 대신 Niagara를 표시한다. 시작/끝은 월드 좌표 Position 파라미터다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Visual")
	TObjectPtr<UNiagaraSystem> VisualNiagara;
	// 발사 방향에 대한 Niagara 로컬 회전 보정(도). X=Roll, Y=Pitch, Z=Yaw.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Visual", meta = (Units = "deg"))
	FVector VisualRotationOffset = FVector::ZeroVector;
	// 메시 Z 전체 길이 * 초기 Z 스케일(cm). SM_Cylinder 약 33cm, 초기 Z=10이면 약 330cm.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Visual", meta = (ClampMin = "0.01", Units = "cm"))
	float NiagaraReferenceLength = 330.0f;
	// 메시 원점이 중앙이고 파티클 위치 보정이 없는 에셋에서만 켠다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Visual")
	bool bNiagaraOriginAtCenter = false;
	// Particle Update의 Scale Mesh Size.Scale Factor에 연결한다. 로컬 Z가 길이 축이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Visual")
	FName BeamScaleParameter = TEXT("User.BeamScale");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Visual")
	FName BeamLengthParameter = TEXT("User.BeamLength");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Visual")
	FName BeamStartParameter = TEXT("User.BeamStart");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Visual")
	FName BeamEndParameter = TEXT("User.BeamEnd");
	// Niagara에서 사용하는 전체 폭(cm). 공격 판정의 HalfWidth * 2를 전달한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Visual")
	FName BeamWidthParameter = TEXT("User.BeamWidth");
	// 100cm 크기, 로컬 X축 방향의 메시를 기준으로 늘린다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Visual")
	TObjectPtr<UStaticMesh> VisualMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Visual")
	TObjectPtr<UMaterialInterface> VisualMaterial;
	// 고정된 표면에 클라이언트별로 생성하는 흔적. Deferred Decal 머티리얼을 지정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Impact Decal")
	TObjectPtr<UMaterialInterface> ImpactDecalMaterial;
	// 단일 머티리얼 내부 이미지 선택 수. N개이면 0~N-1 중 무작위 인덱스를 전달한다.
	// 머티리얼의 실제 선택 분기 수와 맞춰야 한다. 1이면 기존 머티리얼을 그대로 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Impact Decal", meta = (ClampMin = "1"))
	int32 ImpactDecalVariantCount = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Impact Decal")
	FName ImpactDecalVariantParameter = TEXT("VariantIndex");
	// 표면 위 회전 범위(도). X=최소, Y=최대. 같은 값으로 지정하면 고정 회전.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Impact Decal", meta = (Units = "deg"))
	FVector2D ImpactDecalRotationRange = FVector2D(0.0f, 360.0f);
	// 표면 크기 Y/Z에 동일한 랜덤 배율을 적용한다. X 투영 깊이는 유지한다.
	// X=최소, Y=최대. (1, 1)이면 기존 크기를 그대로 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Impact Decal", meta = (ClampMin = "0.01"))
	FVector2D ImpactDecalScaleRange = FVector2D(0.7f, 1.3f);
	// X=투영 깊이, Y/Z=표면 위 크기(cm).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Impact Decal", meta = (ClampMin = "0.1", Units = "cm"))
	FVector ImpactDecalSize = FVector(4.0f, 12.0f, 12.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Impact Decal", meta = (ClampMin = "0.1", Units = "s"))
	float ImpactDecalLifetime = 10.0f;
	// 0이면 즉시 표시. 머티리얼의 Decal Lifetime Opacity로 반영된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Impact Decal", meta = (ClampMin = "0.0", Units = "s"))
	float ImpactDecalFadeInDuration = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Impact Decal", meta = (ClampMin = "0.0", Units = "s"))
	float ImpactDecalFadeDuration = 2.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Impact Decal", meta = (ClampMin = "0.01", Units = "s"))
	float ImpactDecalInterval = 0.05f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Impact Decal", meta = (ClampMin = "0.1", Units = "cm"))
	float ImpactDecalSpacing = 6.0f;
	// 실행 액터 하나가 남길 수 있는 살아 있는 데칼 수. 초과 시 가장 오래된 것부터 제거한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Beam|Impact Decal", meta = (ClampMin = "1"))
	int32 MaxImpactDecals = 64;
private:
	TArray<TWeakObjectPtr<UDecalComponent>> ImpactDecals;
	FVector LastDecalLocation = FVector::ZeroVector;
	FVector LastDecalNormal = FVector::ZeroVector;
	double LastDecalTime = -1.0;
	bool bWasHittingSurface = false;
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> BeamNiagara;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> BeamMesh;
};
