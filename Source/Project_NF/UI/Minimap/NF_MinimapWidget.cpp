#include "UI/Minimap/NF_MinimapWidget.h"

#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Player/State/NF_PlayerState.h"

// @owner: 이정준 (구현: Codex) | 2026-10-15
// 목적: 북쪽 고정 미니맵의 로컬 표시. 외부 시스템이 허용한 좌표만 소비한다.
UNF_MinimapWidget::UNF_MinimapWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetClipping(EWidgetClipping::ClipToBounds);
}

// @Owner: 이정준 (구현: Codex) | 2026-10-26
void UNF_MinimapWidget::SetSnapshot(const FNF_MinimapSnapshot& InSnapshot)
{
	bSnapshotDriven = true;
	bHasView = InSnapshot.bHasView && !InSnapshot.ViewLocation.ContainsNaN()
		&& FMath::IsFinite(InSnapshot.ViewYaw);
	Center = bHasView ? InSnapshot.ViewLocation : FVector::ZeroVector;
	CenterYaw = bHasView ? InSnapshot.ViewYaw : 0.0f;
	Entries.Reset();
	if (bHasView)
	{
		Entries.Reserve(InSnapshot.Markers.Num());
		for (const FNF_MinimapMarker& Marker : InSnapshot.Markers)
		{
			if (Marker.Id.IsNone() || Marker.WorldLocation.ContainsNaN()
				|| !FMath::IsFinite(Marker.Size) || Marker.Size <= 0.0f) { continue; }
			FNF_MinimapEntry& Entry = Entries.AddDefaulted_GetRef();
			Entry.Marker = Marker;
			Entry.ExpiresAt = 0.0; // The VM owns snapshot marker lifetime.
		}
	}
	InvalidateLayoutAndVolatility();
}

bool UNF_MinimapWidget::UpsertMarker(const FNF_MinimapMarker& Marker, float Lifetime)
{
	if (bSnapshotDriven || Marker.Id.IsNone() || Marker.WorldLocation.ContainsNaN() || !FMath::IsFinite(Marker.Size)
		|| Marker.Size <= 0.0f || !FMath::IsFinite(Lifetime) || Lifetime < 0.0f
		|| (Lifetime > 0.0f && !GetWorld())) { return false; }
	FNF_MinimapEntry* Entry = Entries.FindByPredicate([&](const FNF_MinimapEntry& Existing) { return Existing.Marker.Id == Marker.Id; });
	if (!Entry) { Entry = &Entries.AddDefaulted_GetRef(); }
	Entry->Marker = Marker;
	Entry->ExpiresAt = Lifetime > 0.0f ? GetWorld()->GetTimeSeconds() + static_cast<double>(Lifetime) : 0.0;
	InvalidateLayoutAndVolatility();
	return true;
}

bool UNF_MinimapWidget::RemoveMarker(FName Id)
{
	if (bSnapshotDriven) { return false; }
	const bool bRemoved = Entries.RemoveAll([&](const FNF_MinimapEntry& Entry) { return Entry.Marker.Id == Id; }) > 0;
	if (bRemoved) { InvalidateLayoutAndVolatility(); }
	return bRemoved;
}

void UNF_MinimapWidget::ClearMarkers()
{
	if (bSnapshotDriven) { return; }
	Entries.Reset(); InvalidateLayoutAndVolatility();
}
void UNF_MinimapWidget::ClearMarkersByType(ENF_MinimapMarkerType Type)
{
	if (bSnapshotDriven) { return; }
	Entries.RemoveAll([&](const FNF_MinimapEntry& Entry) { return Entry.Marker.Type == Type; });
	InvalidateLayoutAndVolatility();
}
void UNF_MinimapWidget::SetViewLocation(FVector Location, float ViewYaw)
{
	if (bSnapshotDriven || Location.ContainsNaN() || !FMath::IsFinite(ViewYaw)) { return; }
	Center = Location; CenterYaw = ViewYaw; bHasView = true;
	InvalidateLayoutAndVolatility();
}
void UNF_MinimapWidget::SetVisibleWorldSize(float SizeInCentimeters)
{
	if (!FMath::IsFinite(SizeInCentimeters) || SizeInCentimeters <= 0.0f) { return; }
	VisibleWorldSize = SizeInCentimeters;
	InvalidateLayoutAndVolatility();
}

bool UNF_MinimapWidget::ProjectWorldToMinimap(FVector Location, FVector ViewLocation,
	float WorldSize, FVector2D WidgetSize, FVector2D& Out)
{
	Out = FVector2D::ZeroVector;
	if (Location.ContainsNaN() || ViewLocation.ContainsNaN() || WidgetSize.ContainsNaN()
		|| !FMath::IsFinite(WorldSize) || WorldSize <= 0.0f || WidgetSize.GetMin() <= 0.0) { return false; }
	const double Side = WidgetSize.GetMin();
	const FVector Delta = Location - ViewLocation;
	const FVector2D Normalized(Delta.Y / WorldSize + 0.5, 0.5 - Delta.X / WorldSize);
	Out = (WidgetSize - FVector2D(Side)) * 0.5 + Normalized * Side;
	return Normalized.X >= 0.0 && Normalized.X <= 1.0 && Normalized.Y >= 0.0 && Normalized.Y <= 1.0;
}

void UNF_MinimapWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	// Only legacy manual markers use widget-owned expiry. No game-state collection.
	if (bSnapshotDriven) { return; }
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (Entries.RemoveAll([Now](const FNF_MinimapEntry& Entry) { return Entry.ExpiresAt > 0.0 && Entry.ExpiresAt <= Now; }) > 0)
	{
		InvalidateLayoutAndVolatility();
	}
}

bool UNF_MinimapWidget::ArePlayerStatesTeammates(const APlayerState* Viewer, const APlayerState* Other)
{
	const auto* Local = Cast<ANF_PlayerState>(Viewer);
	const auto* Target = Cast<ANF_PlayerState>(Other);
	return IsValid(Local) && IsValid(Target) && Local->NF_TeamID >= 1 && Local->NF_TeamID == Target->NF_TeamID;
}

FLinearColor UNF_MinimapWidget::GetMarkerColor(ENF_MinimapMarkerType Type) const
{
	switch (Type)
	{
	case ENF_MinimapMarkerType::Teammate: return TeammateColor;
	case ENF_MinimapMarkerType::Monster: return MonsterColor;
	case ENF_MinimapMarkerType::RevealedEnemy: return EnemyColor;
	case ENF_MinimapMarkerType::Objective: return ObjectiveColor;
	case ENF_MinimapMarkerType::Player: return PlayerColor;
	default: return FLinearColor::White;
	}
}



int32 UNF_MinimapWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
	const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 LayerId,
	const FWidgetStyle& Style, bool bParentEnabled) const
{
	// 지도→대상 아이콘→본인 화살표 순으로 그리고 BP 자식 위젯은 그 위에 겹친다.
	const FVector2D Size = Geometry.GetLocalSize();
	const double Side = Size.GetMin();
	if (Size.ContainsNaN() || Side <= 0.0 || !FMath::IsFinite(VisibleWorldSize) || VisibleWorldSize <= 0.0f)
	{
		return Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, bParentEnabled);
	}
	const FVector2D Offset = (Size - FVector2D(Side)) * 0.5;
	const FLinearColor Tint = Style.GetColorAndOpacityTint();
	const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const auto Box = [&](FVector2D Position, FVector2D BoxSize, const FSlateBrush* Brush, FLinearColor Color, int32 Layer)
	{
		FSlateDrawElement::MakeBox(Elements, Layer, Geometry.ToPaintGeometry(FVector2f(BoxSize), FSlateLayoutTransform(FVector2f(Position))), Brush,
			ESlateDrawEffect::None, Color * Tint);
	};
	Box(Offset, FVector2D(Side), White, BackgroundColor, LayerId);
	const FVector2D MapSize = MapWorldMax - MapWorldMin;
	if (bHasView && MapTexture && !MapWorldMin.ContainsNaN() && !MapWorldMax.ContainsNaN() && MapSize.GetMin() > 0.0)
	{
		// 전체 지도 UV: U=+Y, V=-X. 이미지 밖 영역은 배경색으로 남긴다.
		const FVector2D UVCenter((Center.Y - MapWorldMin.Y) / MapSize.Y, (MapWorldMax.X - Center.X) / MapSize.X);
		const FVector2D UVSpan(VisibleWorldSize / MapSize.Y, VisibleWorldSize / MapSize.X);
		const FVector2D RawMin = UVCenter - UVSpan * 0.5;
		const FVector2D RawMax = UVCenter + UVSpan * 0.5;
		const FVector2D UVMin(FMath::Max(0.0, RawMin.X), FMath::Max(0.0, RawMin.Y));
		const FVector2D UVMax(FMath::Min(1.0, RawMax.X), FMath::Min(1.0, RawMax.Y));
		if (UVMax.X > UVMin.X && UVMax.Y > UVMin.Y)
		{
			FSlateBrush Brush; Brush.DrawAs = ESlateBrushDrawType::Image; Brush.SetResourceObject(MapTexture);
			Brush.SetUVRegion(FBox2D(UVMin, UVMax));
			Box(Offset + (UVMin - RawMin) / UVSpan * Side, (UVMax - UVMin) / UVSpan * Side, &Brush, MapTint, LayerId + 1);
		}
	}
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (bHasView)
	{
		const auto DrawMarker = [&](const FNF_MinimapMarker& Marker)
		{
			FVector2D Position;
			if (!ProjectWorldToMinimap(Marker.WorldLocation, Center, VisibleWorldSize, Size, Position)) { return; }
			// 아이콘이 프레임 밖으로 나가지 않게 중심을 반 크기만큼 안쪽으로 제한한다.
			const double IconSize = FMath::Min(static_cast<double>(Marker.Size), Side);
			Position.X = FMath::Clamp(Position.X, Offset.X + IconSize * 0.5, Offset.X + Side - IconSize * 0.5);
			Position.Y = FMath::Clamp(Position.Y, Offset.Y + IconSize * 0.5, Offset.Y + Side - IconSize * 0.5);
			FSlateBrush Brush; Brush.DrawAs = ESlateBrushDrawType::Image; Brush.SetResourceObject(Marker.Icon);
			Box(Position - FVector2D(IconSize * 0.5), FVector2D(IconSize), Marker.Icon ? &Brush : White, GetMarkerColor(Marker.Type), LayerId + 2);
		};
		for (const FNF_MinimapEntry& Entry : Entries)
		{
			if (!bSnapshotDriven && Entry.ExpiresAt > 0.0 && Entry.ExpiresAt <= Now) { continue; }
			DrawMarker(Entry.Marker);
		}
		if (bShowLocalPlayer && FMath::IsFinite(LocalPlayerSize) && LocalPlayerSize > 0.0f)
		{
			const double Radius = FMath::Min(static_cast<double>(LocalPlayerSize), Side) * 0.5;
			const double Radians = FMath::DegreesToRadians(static_cast<double>(CenterYaw));
			const FVector2D Forward(FMath::Sin(Radians), -FMath::Cos(Radians));
			const FVector2D Right(-Forward.Y, Forward.X);
			const FVector2D Middle = Size * 0.5;
			TArray<FVector2D> Points { Middle + Forward * Radius, Middle - Forward * Radius + Right * Radius * 0.7,
				Middle - Forward * Radius - Right * Radius * 0.7, Middle + Forward * Radius };
			FSlateDrawElement::MakeLines(Elements, LayerId + 3, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, LocalPlayerColor * Tint, true, 1.5f);
		}
	}
	return Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId + 4, Style, bParentEnabled);
}
