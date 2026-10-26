#pragma once

#include "CoreMinimal.h"
#include "NF_MinimapTypes.generated.h"

class UTexture2D;

// @owner: 이정준 (구현: Codex) | 2026-10-15
// 목적: GAS/Actor/ViewModel에 의존하지 않는 미니맵 표시 데이터.
UENUM(BlueprintType)
enum class ENF_MinimapMarkerType : uint8
{
	Teammate,
	Monster,
	RevealedEnemy,
	Objective,
	Player
};

USTRUCT(BlueprintType)
struct PROJECT_NF_API FNF_MinimapMarker
{
	GENERATED_BODY()

	// 같은 ID를 전달하면 기존 아이콘 위치/모양을 갱신한다. None은 사용하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FVector WorldLocation = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	ENF_MinimapMarkerType Type = ENF_MinimapMarkerType::RevealedEnemy;
	// 비워두면 종류별 색상의 사각형 표시. 아이콘은 회전하지 않는다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	TObjectPtr<UTexture2D> Icon = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap", meta = (ClampMin = "1.0"))
	float Size = 8.0f;
};

USTRUCT()
struct FNF_MinimapEntry
{
	GENERATED_BODY()
	UPROPERTY()
	FNF_MinimapMarker Marker;
	// 이 위젯이 속한 로컬 월드의 시간. 0은 만료 없음.
	double ExpiresAt = 0.0;
};