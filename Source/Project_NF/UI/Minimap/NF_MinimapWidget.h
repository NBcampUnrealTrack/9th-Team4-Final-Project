#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Minimap/NF_MinimapTypes.h"
#include "NF_MinimapWidget.generated.h"

class APawn;

// @owner: 이정준 (구현: Codex) | 2026-10-15
// 목적: 북쪽=+X / 동쪽=+Y 지도 배경·좌표 변환·아이콘 표시. 탐색/복제/GAS는 외부에서 담당한다.
UCLASS(BlueprintType, Blueprintable)
class PROJECT_NF_API UNF_MinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UNF_MinimapWidget(const FObjectInitializer& ObjectInitializer);

	// Lifetime은 수신 시점부터 남은 로컬 초. 0은 영구, 재호출 시 만료 시각 갱신.
	UFUNCTION(BlueprintCallable, Category = "NF|Minimap")
	bool UpsertMarker(const FNF_MinimapMarker& Marker, float Lifetime = 0.0f);
	UFUNCTION(BlueprintCallable, Category = "NF|Minimap")
	bool RemoveMarker(FName Id);
	UFUNCTION(BlueprintCallable, Category = "NF|Minimap")
	void ClearMarkers();
	UFUNCTION(BlueprintCallable, Category = "NF|Minimap")
	void ClearMarkersByType(ENF_MinimapMarkerType Type);
	// Follow Owning Pawn이 꺼진 경우 수동으로 기준 위치/본인 화살표 방향을 제공한다.
	UFUNCTION(BlueprintCallable, Category = "NF|Minimap")
	void SetViewLocation(FVector Location, float ViewYaw = 0.0f);
	UFUNCTION(BlueprintCallable, Category = "NF|Minimap")
	void SetVisibleWorldSize(float SizeInCentimeters);

	// WidgetSize 안에 중앙 정사각형 지도를 가정. 반환값=false여도 바깥 좌표를 출력한다.
	// UMG 로컬 좌표이므로 화면 픽셀/DPI를 별도로 곱하지 않는다. Z는 사용하지 않는다.
	UFUNCTION(BlueprintPure, Category = "NF|Minimap")
	static bool ProjectWorldToMinimap(FVector WorldLocation, FVector ViewLocation,
		float WorldSize, FVector2D WidgetSize, FVector2D& OutLocalPosition);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;


	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	// 지도 이미지는 +X가 위, +Y가 오른쪽이 되도록 만든다. 범위는 이미지 전체에 대응한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Minimap|Map")
	TObjectPtr<UTexture2D> MapTexture = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Minimap|Map")
	FVector2D MapWorldMin = FVector2D(-5000.0, -5000.0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Minimap|Map")
	FVector2D MapWorldMax = FVector2D(5000.0, 5000.0);
	// 지도 한 변의 전체 월드 길이. 4000이면 중심에서 좌우/상하 각각 2000cm.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Minimap|Map", meta = (ClampMin = "1.0", Units = "cm"))
	float VisibleWorldSize = 4000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Minimap|View")
	bool bFollowOwningPawn = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Minimap|View")
	bool bShowLocalPlayer = true;
	// @owner: 이정준 (구현: Codex) | 2026-10-15
	// 목적: ViewModel 연결 전 테스트용. 로컬에 복제된 모든 플레이어를 팀 검사 없이 표시한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Minimap|Temporary")
	bool bTemporaryShowAllPlayers = false;
	// @owner: 이정준 (구현: Codex) | 2026-10-15
	// 목적: ViewModel 연결 전 수신한 공통 공개 목록을 직접 표시한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Minimap|View")
	bool bShowRegisteredReveals = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Minimap|Style", meta = (ClampMin = "1.0"))
	float RevealMarkerSize = 20.0f;
	// @owner: 이정준 (구현: Codex) | 2026-10-15
	// 목적: 임시 전체 플레이어 표시를 쉽게 식별하도록 크기를 별도 조절한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Minimap|Temporary", meta = (ClampMin = "1.0"))
	float TemporaryPlayerMarkerSize = 20.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Minimap|Style")
	FLinearColor PlayerColor = FLinearColor(1.0f, 0.85f, 0.05f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Minimap|Style")
	FLinearColor BackgroundColor = FLinearColor(0.025f, 0.025f, 0.025f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Minimap|Style")
	FLinearColor MapTint = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Minimap|Style")
	FLinearColor LocalPlayerColor = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Minimap|Style")
	FLinearColor TeammateColor = FLinearColor(0.1f, 0.6f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Minimap|Style")
	FLinearColor MonsterColor = FLinearColor(1.0f, 0.65f, 0.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Minimap|Style")
	FLinearColor EnemyColor = FLinearColor::Red;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Minimap|Style")
	FLinearColor ObjectiveColor = FLinearColor::Green;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Minimap|Style", meta = (ClampMin = "1.0"))
	float LocalPlayerSize = 12.0f;

private:
	void UpdateTemporaryPlayers(float DeltaTime);
	FLinearColor GetMarkerColor(ENF_MinimapMarkerType Type) const;
	UPROPERTY(Transient)
	TArray<FNF_MinimapEntry> Entries;
	// 외부 API 마커와 분리하여 임시 기능을 꺼도 외부 표시 데이터는 유지한다.
	UPROPERTY(Transient)
	TArray<FNF_MinimapMarker> TemporaryPlayerMarkers;
	TArray<TWeakObjectPtr<APawn>> TemporaryPlayerPawns;
	float PlayerRefreshCountdown = 0.0f;
	FVector Center = FVector::ZeroVector;
	float CenterYaw = 0.0f;
	bool bHasView = false;
};
