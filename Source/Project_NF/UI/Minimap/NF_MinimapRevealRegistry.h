#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/NetSerialization.h"
#include "NF_MinimapRevealRegistry.generated.h"

class APlayerController;
class AActor;
class APlayerState;

// @owner: 이정준 (구현: Codex) | 2026-10-15
// 목적: 천리안/열쇠가 공유하는 수신자별 공개 목록. UObject 자체는 복제하지 않는다.
UENUM(BlueprintType)
enum class ENF_MinimapRevealKind : uint8 { Clairvoyance, EscapeKey };

USTRUCT(BlueprintType)
struct PROJECT_NF_API FNF_MinimapRevealMarker
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "Minimap")
	FName TargetId;
	UPROPERTY(BlueprintReadOnly, Category = "Minimap")
	FVector_NetQuantize WorldLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category = "Minimap")
	ENF_MinimapRevealKind Kind = ENF_MinimapRevealKind::Clairvoyance;
	// Team metadata only accompanies an authorized reveal snapshot; no extra enemy positions are sent.
	UPROPERTY(BlueprintReadOnly, Category = "Minimap")
	int32 PlayerTeamID = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly, Category = "Minimap")
	TObjectPtr<APlayerState> PlayerState = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNF_MinimapRevealsChanged);

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_MinimapRevealRegistry : public UObject
{
	GENERATED_BODY()
public:
	void Initialize(APlayerController* InRecipient);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Minimap|Reveal")
	bool RegisterReveal(FGuid SourceId, AActor* Target, ENF_MinimapRevealKind Kind, float Lifetime = 0.0f);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Minimap|Reveal")
	void RemoveReveal(FGuid SourceId, AActor* Target);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Minimap|Reveal")
	void RemoveRevealSource(FGuid SourceId);
	// 해당 출처의 집합만 교체. 다른 출처나 열쇠 공개는 유지한다.
	void UpdateRevealSource(FGuid SourceId, const TArray<AActor*>& Targets, ENF_MinimapRevealKind Kind, float Lifetime);
	void BuildServerSnapshot(TArray<FNF_MinimapRevealMarker>& OutMarkers);
	void ReceiveSnapshot(const TArray<FNF_MinimapRevealMarker>& Markers, float ValidForSeconds);
	// ViewModel은 이 목록과 변경 이벤트를 사용한다. 위젯 재생성 시에도 현재 목록 조회 가능.
	UFUNCTION(BlueprintPure, Category = "NF|Minimap|Reveal")
	TArray<FNF_MinimapRevealMarker> GetVisibleMarkers() const;
	UPROPERTY(BlueprintAssignable, Category = "NF|Minimap|Reveal")
	FNF_MinimapRevealsChanged OnRevealsChanged;
	void Clear();
	void ExpireClientSnapshot();
private:
	struct FReveal
	{
		FGuid SourceId;
		TWeakObjectPtr<AActor> Target;
		FName TargetId;
		ENF_MinimapRevealKind Kind;
		double ExpiresAt = 0.0;
	};
	bool IsServer() const;
	double Now() const;
	TWeakObjectPtr<APlayerController> Recipient;
	TArray<FReveal> Reveals;
	UPROPERTY(Transient)
	TArray<FNF_MinimapRevealMarker> ReceivedMarkers;
	double ReceivedUntil = 0.0;
	uint64 NextTargetId = 1;
};
