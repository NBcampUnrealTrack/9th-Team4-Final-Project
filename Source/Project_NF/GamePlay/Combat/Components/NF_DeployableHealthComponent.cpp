#include "GamePlay/Combat/Components/NF_DeployableHealthComponent.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "GAS/Attributes/NF_DeployableHealthAttribute.h"
#include "GamePlay/Combat/Components/NF_DeployableComponent.h"
#include "Project_NF.h"

UNF_DeployableHealthComponent::UNF_DeployableHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNF_DeployableHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!InitializeHealth()) { UE_LOG(LogProject_NF, Warning, TEXT("[Deployable] Health initialization failed on %s. Add an actor-owned ASC."), *GetNameSafe(GetOwner())); }
}

bool UNF_DeployableHealthComponent::InitializeHealth()
{
	if (bInitialized) { return true; }
	AActor* Owner = GetOwner();
	if (bEndingPlay || !IsValid(Owner) || !FMath::IsFinite(InitialMaxHealth) || InitialMaxHealth < 1.0f) { return false; }
	UAbilitySystemComponent* FoundASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner, true);
	// 설치물 자체 ASC만 허용한다. 시전자/PlayerState의 Attribute를 공유하지 않는다.
	if (!IsValid(FoundASC) || FoundASC->GetOwner() != Owner) { return false; }
	if (!FoundASC->GetAvatarActor()) { FoundASC->InitAbilityActorInfo(Owner, Owner); }
	const UNF_DeployableHealthAttribute* Existing = FoundASC->GetSet<UNF_DeployableHealthAttribute>();
	if (!Existing && Owner->HasAuthority())
	{
		// 런타임 Attribute는 서버에서만 생성한다. ASC가 동적 subobject로 클라이언트에 복제한다.
		UNF_DeployableHealthAttribute* Created = NewObject<UNF_DeployableHealthAttribute>(Owner);
		FoundASC->AddAttributeSetSubobject(Created);
	}
	ASC = FoundASC;
	if (Owner->HasAuthority() && !Existing)
	{
		FoundASC->SetNumericAttributeBase(UNF_DeployableHealthAttribute::GetMaxHealthAttribute(), InitialMaxHealth);
		FoundASC->SetNumericAttributeBase(UNF_DeployableHealthAttribute::GetHealthAttribute(), InitialMaxHealth);
	}
	HealthDelegate = FoundASC->GetGameplayAttributeValueChangeDelegate(UNF_DeployableHealthAttribute::GetHealthAttribute())
		.AddUObject(this, &UNF_DeployableHealthComponent::HandleAttributeChanged);
	MaxHealthDelegate = FoundASC->GetGameplayAttributeValueChangeDelegate(UNF_DeployableHealthAttribute::GetMaxHealthAttribute())
		.AddUObject(this, &UNF_DeployableHealthComponent::HandleAttributeChanged);
	bInitialized = true;
	NotifyHealth();
	return true;
}

const UNF_DeployableHealthAttribute* UNF_DeployableHealthComponent::FindAttributes() const
{
	return ASC.IsValid() ? ASC->GetSet<UNF_DeployableHealthAttribute>() : nullptr;
}
float UNF_DeployableHealthComponent::GetHealth() const
{
	const UNF_DeployableHealthAttribute* Attributes = FindAttributes();
	return Attributes ? Attributes->GetHealth() : 0.0f;
}
float UNF_DeployableHealthComponent::GetMaxHealth() const
{
	const UNF_DeployableHealthAttribute* Attributes = FindAttributes();
	return Attributes ? Attributes->GetMaxHealth() : 0.0f;
}
void UNF_DeployableHealthComponent::HandleAttributeChanged(const FOnAttributeChangeData& Data) { NotifyHealth(); }

void UNF_DeployableHealthComponent::NotifyHealth()
{
	if (!bInitialized || bEndingPlay || !IsValid(GetOwner()) || !FindAttributes()) { return; }
	const bool bDepleted = GetOwner()->HasAuthority() && GetHealth() <= 0.0f && !bDepletedNotified;
	// BPイベント内の回復/再入でも同じ設置物の破壊を重複発行しない。
	if (bDepleted) { bDepletedNotified = true; }
	OnHealthChanged.Broadcast(GetHealth(), GetMaxHealth());
	if (bDepleted && !bEndingPlay && IsValid(GetOwner()))
	{
		if (bEndDeploymentOnDepleted)
		{
			if (UNF_DeployableComponent* Deployment = GetOwner()->FindComponentByClass<UNF_DeployableComponent>())
			{
				Deployment->EndDeployment(ENF_DeploymentEndReason::Destroyed);
			}
		}
		if (!bEndingPlay && IsValid(GetOwner())) { OnHealthDepleted.Broadcast(); }
	}
}

void UNF_DeployableHealthComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	bEndingPlay = true;
	if (ASC.IsValid())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UNF_DeployableHealthAttribute::GetHealthAttribute()).Remove(HealthDelegate);
		ASC->GetGameplayAttributeValueChangeDelegate(UNF_DeployableHealthAttribute::GetMaxHealthAttribute()).Remove(MaxHealthDelegate);
	}
	Super::EndPlay(Reason);
}
