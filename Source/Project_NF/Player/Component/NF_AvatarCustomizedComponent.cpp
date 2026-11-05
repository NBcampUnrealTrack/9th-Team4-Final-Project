#include "NF_AvatarCustomizedComponent.h"

#include "Net/UnrealNetwork.h"
#include "Player/Character/NF_Player.h"
#include "Player/Data/NF_PlayerDataType.h"
#include "Player/Data/NF_CustomVisualMeshDataAsset.h"
#include "Player/Data/NF_PlayerDataAsset.h"
#include "Player/State/NF_PlayerState.h"


UNF_PlayerVisualMeshCustomizedComponent::UNF_PlayerVisualMeshCustomizedComponent()
{
	SetIsReplicatedByDefault(true);
}

void UNF_PlayerVisualMeshCustomizedComponent::TryChangeVisualMesh(int32 Index)
{
	ENF_CustomVisualMeshType NF_CustomVisualMeshType = ENF_CustomVisualMeshType::None;

	switch (Index)
	{
		default:
			NF_CustomVisualMeshType = ENF_CustomVisualMeshType::Girl_01;
		break;
		
		case 0:
			NF_CustomVisualMeshType = ENF_CustomVisualMeshType::Girl_01;
		break;
		
		case 1:
			NF_CustomVisualMeshType = ENF_CustomVisualMeshType::Girl_02;
		break;
		
		case 2:
			NF_CustomVisualMeshType = ENF_CustomVisualMeshType::Girl_03;
		break;
		
		case 3:
			NF_CustomVisualMeshType = ENF_CustomVisualMeshType::Girl_04;
		break;
		
		case 4:
			NF_CustomVisualMeshType = ENF_CustomVisualMeshType::Boy_01;
		break;
		
		case 5:
			NF_CustomVisualMeshType = ENF_CustomVisualMeshType::Boy_02;
		break;
		
		case 6:
			NF_CustomVisualMeshType = ENF_CustomVisualMeshType::Boy_03;
		break;
		
		case 7:
			NF_CustomVisualMeshType = ENF_CustomVisualMeshType::Boy_04;
		break;
	}
		
	ServerRPC_TryChangeVisualMesh(NF_CustomVisualMeshType);
}

void UNF_PlayerVisualMeshCustomizedComponent::GetLifetimeReplicatedProps(
	TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, CurrentCustomVisualMeshDataAsset);
}


void UNF_PlayerVisualMeshCustomizedComponent::ServerRPC_TryChangeVisualMesh_Implementation(ENF_CustomVisualMeshType VisualMeshType)
{
	auto* NF_PlayerDataAsset = Cast<UNF_PlayerDataAsset>(GetNF_PlayerData(ENF_PlayerDataType::NF_PlayerDataAsset));
	if (IsValid(NF_PlayerDataAsset) == false) return;
	
	UNF_CustomVisualMeshDataAsset* CustomVisualMeshDataAsset = NF_PlayerDataAsset->CustomVisualMeshMap.FindRef(VisualMeshType);
	if (IsValid(CustomVisualMeshDataAsset) == false) return;
	
	CurrentCustomVisualMeshDataAsset = CustomVisualMeshDataAsset;
	
	ApplyCustomVisualMesh();
}

void UNF_PlayerVisualMeshCustomizedComponent::OnRep_CurrentCustomVisualMeshDataAsset()
{
	ApplyCustomVisualMesh();
}

void UNF_PlayerVisualMeshCustomizedComponent::ApplyCustomVisualMesh()
{
	if (IsValid(CurrentCustomVisualMeshDataAsset) == false) return;
	
	auto* NF_Player = GetOwner<ANF_Player>();
	if (IsValid(NF_Player) == false) return;
	
	auto& VisualMesh = NF_Player->VisualMesh;
	if (IsValid(VisualMesh) == false) return;
		
	VisualMesh->SetSkeletalMeshAsset(CurrentCustomVisualMeshDataAsset->VisualSkeletalMesh);
	VisualMesh->SetAnimInstanceClass(CurrentCustomVisualMeshDataAsset->VisualAnimClass);
	VisualMesh->SetRelativeScale3D(CurrentCustomVisualMeshDataAsset->VisualMeshScale);
}

UObject* UNF_PlayerVisualMeshCustomizedComponent::GetNF_PlayerData(ENF_PlayerDataType DataType)
{
	auto* NF_Player = GetOwner<ANF_Player>();
	if (IsValid(NF_Player) == false) return nullptr;
	
	auto* NF_PlayerState = NF_Player->GetPlayerState<ANF_PlayerState>();
	if (IsValid(NF_PlayerState) == false) return nullptr;
	
	if (DataType == ENF_PlayerDataType::NF_Player) return NF_Player;
	
	if (DataType == ENF_PlayerDataType::NF_PlayerState) return NF_PlayerState;
	
	if (DataType == ENF_PlayerDataType::NF_PlayerDataAsset) return NF_PlayerState->NF_PlayerDataAsset;
	
	return nullptr;
}
