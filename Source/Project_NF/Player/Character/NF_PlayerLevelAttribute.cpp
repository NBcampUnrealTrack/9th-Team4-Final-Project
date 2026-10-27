#include "NF_PlayerLevelAttribute.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "Player/Data/NF_PlayerDataAsset.h"
#include "Player/Data/NF_PlayerLevelDataTable.h"
#include "Player/State/NF_PlayerState.h"

UNF_PlayerLevelAttribute::UNF_PlayerLevelAttribute()
{
	
}

void UNF_PlayerLevelAttribute::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
}

void UNF_PlayerLevelAttribute::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	//GE 데이터가 들어온 경험치 (IncomingExp)라면
	if (Data.EvaluatedData.Attribute == GetIncomingExpAttribute())
	{
		float ReceivedExp = GetIncomingExp(); //들어온 경험치 사전 저장
		SetIncomingExp(0.0f); //들어온 경험치 초기화
		if (GetCurrentLevel() >= GetMaxLevel()) return;
		
		SetCurrentExp(GetCurrentExp() + ReceivedExp);
		
		SetTargetExp(GetTargetExpFromLevel());
		if (GetTargetExp() <= 0.0f) return;
		
		if (GetCurrentExp() >= GetTargetExp() && GetCurrentLevel() < GetMaxLevel())
		{
			SetCurrentExp(GetCurrentExp() - GetTargetExp());   //목표치만큼 빼고 남은 경험치는 이월
			SetCurrentLevel(GetCurrentLevel() + 1.0f);          //1레벨 업
			OnLevelUp.Broadcast(FMath::FloorToInt32(GetCurrentLevel()));   //레벨은 소수점 내림으로 정수 유지
		}
	}
}

void UNF_PlayerLevelAttribute::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxLevel, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, CurrentLevel, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, CurrentExp, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, TargetExp, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, IncomingExp, COND_None, REPNOTIFY_Always);
}

void UNF_PlayerLevelAttribute::OnRep_MaxLevel(const FGameplayAttributeData& OldMaxLevel)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxLevel, OldMaxLevel);
}

void UNF_PlayerLevelAttribute::OnRep_CurrentLevel(const FGameplayAttributeData& OldCurrentLevel)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, CurrentLevel, OldCurrentLevel);
}

void UNF_PlayerLevelAttribute::OnRep_CurrentExp(const FGameplayAttributeData& OldCurrentExp)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, CurrentExp, OldCurrentExp);
}

void UNF_PlayerLevelAttribute::OnRep_TargetExp(const FGameplayAttributeData& OldTargetExp)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, TargetExp, OldTargetExp);
}

void UNF_PlayerLevelAttribute::OnRep_IncomingExp(const FGameplayAttributeData& OldIncomingExp)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, IncomingExp, OldIncomingExp);
}

float UNF_PlayerLevelAttribute::GetTargetExpFromLevel() const
{
	auto* NF_PlayerState = Cast<ANF_PlayerState>(GetOwningActor());
	if (IsValid(NF_PlayerState) == false) return 0.0f;
	if (IsValid(NF_PlayerState->NF_PlayerDataAsset) == false) return 0.0f;
	
	UDataTable* PlayerLevelDataTable = NF_PlayerState->NF_PlayerDataAsset->PlayerLevelDataTable;
	if (IsValid(PlayerLevelDataTable) == false) return 0.0f;
	
	TArray<FNF_PlayerLevelDataTable*> LevelRows;
	PlayerLevelDataTable->GetAllRows<FNF_PlayerLevelDataTable>(TEXT("GetTargetExpFromLevel"), LevelRows);
	
	int32 LevelIndex = FMath::FloorToInt32(GetCurrentLevel()) - 1;   //1레벨 → 0번째 행
	if (LevelRows.IsValidIndex(LevelIndex) == false) return 0.0f;
	
	return LevelRows[LevelIndex]->TargetExp;
}
