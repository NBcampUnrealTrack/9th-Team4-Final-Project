#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_AccelerationAfterimage.generated.h"

class ACharacter;
class USkeletalMeshComponent;
class UPoseableMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

USTRUCT(BlueprintType)
struct FNF_AccelerationAfterimageSettings
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> Material;
	// 실제로 표시되는 메시의 컴포넌트 이름. 없으면 Character.GetMesh()를 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName SourceMeshName = TEXT("VisualMesh");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.02", Units = "s"))
	float SpawnInterval = 0.08f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.01", Units = "s"))
	float Lifetime = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1", ClampMax = "32"))
	int32 MaxAfterimages = 6;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName OpacityParameter = TEXT("AfterimageOpacity");
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ColorParameter = TEXT("AfterimageColor");
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor Color = FLinearColor(0.1f, 0.5f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float MinTravelDistance = 5.0f;

	bool IsValid() const;
};

USTRUCT()
struct FNF_AccelerationAfterimageSlot
{
	GENERATED_BODY()
	UPROPERTY(Transient)
	TObjectPtr<UPoseableMeshComponent> Mesh;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;
	double BornAt = -1.0;
};

// 서버는 활성 상태만 복제하고 포즈 스냅샷은 각 클라이언트에서 생성한다.
UCLASS(NotBlueprintable)
class PROJECT_NF_API ANF_AccelerationAfterimage : public AActor
{
	GENERATED_BODY()
public:
	ANF_AccelerationAfterimage();
	void Initialize(ACharacter* Character, const FNF_AccelerationAfterimageSettings& InSettings);
	void StopAfterimages();
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	USkeletalMeshComponent* ResolveSourceMesh();
	void SpawnSnapshot(USkeletalMeshComponent* Source, double Now);
	UPROPERTY(Replicated)
	TObjectPtr<ACharacter> Target;
	UPROPERTY(Replicated)
	FNF_AccelerationAfterimageSettings Settings;
	UPROPERTY(Replicated)
	bool bActive = false;
	UPROPERTY(Transient)
	TArray<FNF_AccelerationAfterimageSlot> Slots;
	TWeakObjectPtr<USkeletalMeshComponent> SourceMesh;
	FVector LastSpawnLocation = FVector::ZeroVector;
	double LastSpawnTime = -1.0;
	int32 NextSlot = 0;
};
