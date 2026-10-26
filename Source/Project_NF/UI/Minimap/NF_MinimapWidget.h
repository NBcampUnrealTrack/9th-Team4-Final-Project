#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Minimap/NF_MinimapTypes.h"
#include "UI/Data/NF_MinimapSnapshot.h"
#include "NF_MinimapWidget.generated.h"

class APlayerState;

// @owner: 이정준 (구현: Codex) | 2026-10-15
// 목적: 북쪽=+X / 동쪽=+Y 지도 배경·좌표 변환·아이콘 표시. 탐색/복제/GAS는 외부에서 담당한다.
UCLASS(BlueprintType, Blueprintable)
class PROJECT_NF_API UNF_MinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UNF_MinimapWidget(const FObjectInitializer& ObjectInitializer);

	// @Owner: 이정준 (구현: Codex) | 2026-10-26
	// Full replacement from the VM. Once used, legacy marker/view writers are ignored.
	UFUNCTION(BlueprintCallable, Category = "NF|Minimap")
	void SetSnapshot(const FNF_MinimapSnapshot& InSnapshot);

	// Legacy manual mode only, before the first SetSnapshot. Lifetime 0 means no expiry.
	UFUNCTION(BlueprintCallable, Category = "NF|Minimap")
	bool UpsertMarker(const FNF_MinimapMarker& Marker, float Lifetime = 0.0f);
	UFUNCTION(BlueprintCallable, Category = "NF|Minimap")
	bool RemoveMarker(FName Id);
	UFUNCTION(BlueprintCallable, Category = "NF|Minimap")
	void ClearMarkers();
	UFUNCTION(BlueprintCallable, Category = "NF|Minimap")
	void ClearMarkersByType(ENF_MinimapMarkerType Type);
	// Legacy manual view input, ignored after the first SetSnapshot.
	UFUNCTION(BlueprintCallable, Category = "NF|Minimap")
	void SetViewLocation(FVector Location, float ViewYaw = 0.0f);
	UFUNCTION(BlueprintCallable, Category = "NF|Minimap")
	void SetVisibleWorldSize(float SizeInCentimeters);

	// WidgetSize 안에 중앙 정사각형 지도를 가정. 반환값=false여도 바깥 좌표를 출력한다.
	// UMG 로컬 좌표이므로 화면 픽셀/DPI를 별도로 곱하지 않는다. Z는 사용하지 않는다.
	UFUNCTION(BlueprintPure, Category = "NF|Minimap")
	static bool ProjectWorldToMinimap(FVector WorldLocation, FVector ViewLocation,
		float WorldSize, FVector2D WidgetSize, FVector2D& OutLocalPosition);
	// Compatibility utility for existing callers/tests; never used by the display path.
	static bool ArePlayerStatesTeammates(const APlayerState* Viewer, const APlayerState* Other);

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
	// Compatibility only: data collection is now owned by the VM.
	UPROPERTY(BlueprintReadWrite, Category = "NF|Minimap|View", meta = (DeprecatedProperty, DeprecationMessage = "Ignored. Bind ViewModel Snapshot to SetSnapshot."))
	bool bFollowOwningPawn = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Minimap|View")
	bool bShowLocalPlayer = true;
	// @owner: 이정준 (구현: Codex) | 2026-10-15
	// Serialized compatibility only. All-player debug display is no longer used.
	UPROPERTY(BlueprintReadWrite, Category = "NF|Minimap|Temporary", meta = (DeprecatedProperty, DeprecationMessage = "Ignored. Authorized markers are supplied by ViewModel Snapshot."))
	bool bTemporaryShowAllPlayers = false;
	// @Owner: 이정준 (구현: Codex) | 2026-10-21
	UPROPERTY(BlueprintReadWrite, Category = "NF|Minimap|View", meta = (DeprecatedProperty, DeprecationMessage = "Ignored. Teammates are supplied by the ViewModel."))
	bool bShowTeammates = true;
	UPROPERTY(BlueprintReadWrite, Category = "NF|Minimap|Style", meta = (DeprecatedProperty, DeprecationMessage = "Ignored. Marker.Size supplied by the ViewModel is used."))
	float TeammateMarkerSize = 20.0f;
	// @owner: 이정준 (구현: Codex) | 2026-10-15
	// Serialized compatibility only; the widget no longer queries the registry.
	UPROPERTY(BlueprintReadWrite, Category = "NF|Minimap|View", meta = (DeprecatedProperty, DeprecationMessage = "Ignored. Authorized markers are supplied by the ViewModel."))
	bool bShowRegisteredReveals = true;
	UPROPERTY(BlueprintReadWrite, Category = "NF|Minimap|Style", meta = (DeprecatedProperty, DeprecationMessage = "Ignored. Marker.Size supplied by the ViewModel is used."))
	float RevealMarkerSize = 20.0f;
	// @owner: 이정준 (구현: Codex) | 2026-10-15
	// Serialized compatibility only. Snapshot Marker.Size controls marker size.
	UPROPERTY(BlueprintReadWrite, Category = "NF|Minimap|Temporary", meta = (DeprecatedProperty, DeprecationMessage = "Ignored. Use Marker.Size in ViewModel Snapshot."))
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
	friend class FNF_MinimapSnapshotTest;
	FLinearColor GetMarkerColor(ENF_MinimapMarkerType Type) const;
	UPROPERTY(Transient)
	TArray<FNF_MinimapEntry> Entries;
	bool bSnapshotDriven = false;
	FVector Center = FVector::ZeroVector;
	float CenterYaw = 0.0f;
	bool bHasView = false;
};
