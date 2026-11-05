
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/Data/NF_CustomVisualMeshType.h"
#include "NF_AvatarCustomizedComponent.generated.h"


class UNF_CustomVisualMeshDataAsset;
enum class ENF_PlayerDataType : uint8;
class ANF_PlayerState;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_PlayerVisualMeshCustomizedComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_PlayerVisualMeshCustomizedComponent();
	
	void TryChangeVisualMesh(int32 Index);
	
	
protected:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentCustomVisualMeshDataAsset)
	TObjectPtr<UNF_CustomVisualMeshDataAsset> CurrentCustomVisualMeshDataAsset;
	
	UFUNCTION(Server, Reliable)
	void ServerRPC_TryChangeVisualMesh(ENF_CustomVisualMeshType VisualMeshType);
	
	UFUNCTION()
	void OnRep_CurrentCustomVisualMeshDataAsset();
	
	void ApplyCustomVisualMesh();
	
private:
	UObject* GetNF_PlayerData(ENF_PlayerDataType DataType);

};
