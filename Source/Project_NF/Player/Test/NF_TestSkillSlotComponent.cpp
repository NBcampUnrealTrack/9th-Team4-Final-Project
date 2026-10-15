#include "NF_TestSkillSlotComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "TimerManager.h"
#include "NFGA_TestSkillPrint.h"


UNF_TestSkillSlotComponent::UNF_TestSkillSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	TestSlotAbilityArray.Add(UNFGA_TestSkillPrint::StaticClass());
	TestSlotAbilityArray.Add(UNFGA_TestSkillPrint::StaticClass());
}


void UNF_TestSkillSlotComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetOwner()->HasAuthority() == false) return;
	
	// 서버 BeginPlay 시점에는 아직 빙의 전이라 ASC(PlayerState)가 없을 수 있어서 0.2초마다 재시도
	GetWorld()->GetTimerManager().SetTimer(GrantRetryTimerHandle, this, &ThisClass::TryGrantTestSkills, 0.2f, true);
}


void UNF_TestSkillSlotComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(GrantRetryTimerHandle);
	
	ClearGrantedTestSkills();
	
	Super::EndPlay(EndPlayReason);
}


void UNF_TestSkillSlotComponent::PressTestSkillInput(int32 SlotIndex)
{
	const auto* OwnerPawn = Cast<APawn>(GetOwner());
	if (IsValid(OwnerPawn) == false) return;
	if (OwnerPawn->IsLocallyControlled() == false) return;
	if (SlotIndex < 0) return;
	if (SlotIndex >= MaxSlotCount) return;
	
	UAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent();
	if (IsValid(AbilitySystemComponent) == false) return;
	
	// NF_BuildComponent::PressActiveSkillInput과 같은 규칙 (상점 슬롯 배치 분기는 제외)
	AbilitySystemComponent->AbilityLocalInputPressed(SlotIndex + 1);
}


void UNF_TestSkillSlotComponent::ReleaseTestSkillInput(int32 SlotIndex)
{
	const auto* OwnerPawn = Cast<APawn>(GetOwner());
	if (IsValid(OwnerPawn) == false) return;
	if (OwnerPawn->IsLocallyControlled() == false) return;
	if (SlotIndex < 0) return;
	if (SlotIndex >= MaxSlotCount) return;
	
	UAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent();
	if (IsValid(AbilitySystemComponent) == false) return;
	
	AbilitySystemComponent->AbilityLocalInputReleased(SlotIndex + 1);
}


void UNF_TestSkillSlotComponent::TryGrantTestSkills()
{
	GrantRetryCount++;
	if (GrantRetryCount > MaxGrantRetryCount)
	{
		GetWorld()->GetTimerManager().ClearTimer(GrantRetryTimerHandle);
		UE_LOG(LogTemp, Error, TEXT("[TestSkillSlot] ASC를 찾지 못해 테스트 스킬 등록을 중단합니다."));
		return;
	}
	
	UAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent();
	if (IsValid(AbilitySystemComponent) == false) return;
	
	// InitAbilityActorInfo가 끝나야 아바타가 이 캐릭터로 잡힘
	if (AbilitySystemComponent->GetAvatarActor() != GetOwner()) return;
	
	GetWorld()->GetTimerManager().ClearTimer(GrantRetryTimerHandle);
	
	for (int32 SlotIndex = 0; SlotIndex < TestSlotAbilityArray.Num(); SlotIndex++)
	{
		if (SlotIndex >= MaxSlotCount) break;
		if (IsValid(TestSlotAbilityArray[SlotIndex]) == false) continue;
		
		// NF_BuildComponent::GrantItem과 같은 규칙: 액티브 슬롯 InputID = SlotIndex + 1
		const int32 InputID = SlotIndex + 1;
		const FGameplayAbilitySpecHandle AbilityHandle = AbilitySystemComponent->GiveAbility(
			FGameplayAbilitySpec(TestSlotAbilityArray[SlotIndex], 1, InputID, this));
		
		GrantedHandleArray.Add(AbilityHandle);
		
		UE_LOG(LogTemp, Warning, TEXT("[TestSkillSlot] %s 를 InputID %d 로 등록"), *GetNameSafe(TestSlotAbilityArray[SlotIndex]), InputID);
	}
}


void UNF_TestSkillSlotComponent::ClearGrantedTestSkills()
{
	if (GetOwner()->HasAuthority() == false) return;
	
	UAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent();
	if (IsValid(AbilitySystemComponent) == false) return;
	
	for (const FGameplayAbilitySpecHandle& AbilityHandle : GrantedHandleArray)
	{
		AbilitySystemComponent->ClearAbility(AbilityHandle);
	}
	
	GrantedHandleArray.Reset();
}


UAbilitySystemComponent* UNF_TestSkillSlotComponent::GetOwnerAbilitySystemComponent() const
{
	const auto* AbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner());
	if (AbilitySystemInterface == nullptr) return nullptr;
	
	return AbilitySystemInterface->GetAbilitySystemComponent();
}
