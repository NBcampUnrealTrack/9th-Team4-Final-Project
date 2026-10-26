#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UI/Minimap/NF_MinimapWidget.h"
#include <limits>

// @owner: 이정준 (구현: Codex) | 2026-10-15
// 목적: 북쪽 고정 좌표 변환의 축 방향·축척·비정사각 영역·범위 경계를 검증한다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_MinimapProjectionTest, "NF.UI.Minimap.Projection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_MinimapProjectionTest::RunTest(const FString& Parameters)
{
	FVector2D Position;
	const FVector Center(1000, -2000, 300);
	const auto Project = [&](FVector Offset, FVector2D Size = FVector2D(200, 200))
	{
		return UNF_MinimapWidget::ProjectWorldToMinimap(Center + Offset, Center, 4000.0f, Size, Position);
	};
	TestTrue(TEXT("Center visible"), Project(FVector::ZeroVector));
	TestTrue(TEXT("Center position"), Position.Equals(FVector2D(100, 100)));
	TestTrue(TEXT("North visible"), Project(FVector(1000, 0, 0)));
	TestTrue(TEXT("Positive X is up"), Position.Equals(FVector2D(100, 50)));
	TestTrue(TEXT("East visible"), Project(FVector(0, 1000, 0)));
	TestTrue(TEXT("Positive Y is right"), Position.Equals(FVector2D(150, 100)));
	TestTrue(TEXT("Height ignored"), Project(FVector(0, 0, 10000)));
	TestTrue(TEXT("Height has no effect"), Position.Equals(FVector2D(100, 100)));
	TestTrue(TEXT("Boundary included"), Project(FVector(2000, 0, 0)));
	TestFalse(TEXT("Outside hidden"), Project(FVector(2001, 0, 0)));
	TestTrue(TEXT("Wide slot supported"), Project(FVector::ZeroVector, FVector2D(300, 200)));
	TestTrue(TEXT("Square centered in slot"), Position.Equals(FVector2D(150, 100)));
	TestFalse(TEXT("Zero span rejected"), UNF_MinimapWidget::ProjectWorldToMinimap(Center, Center, 0, FVector2D(200, 200), Position));
	TestTrue(TEXT("Rejected output initialized"), Position.IsNearlyZero());
	return true;
}

// @Owner: 이정준 (구현: Codex) | 2026-10-26
// VM ownership: replacement, invalid view clearing, and legacy writer isolation.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_MinimapSnapshotTest, "NF.UI.Minimap.Snapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_MinimapSnapshotTest::RunTest(const FString& Parameters)
{
	auto* Widget = NewObject<UNF_MinimapWidget>();
	FNF_MinimapSnapshot Snapshot;
	Snapshot.bHasView = true;
	Snapshot.ViewLocation = FVector(100, 200, 300);
	Snapshot.ViewYaw = 90.f;
	FNF_MinimapMarker Marker;
	Marker.Id = TEXT("AuthorizedEnemy");
	Marker.Size = 17.f;
	Snapshot.Markers.Add(Marker);
	Widget->SetSnapshot(Snapshot);
	TestTrue(TEXT("Valid view accepted"), Widget->bHasView);
	TestTrue(TEXT("Snapshot center used"), Widget->Center.Equals(Snapshot.ViewLocation));
	TestEqual(TEXT("Snapshot heading used"), Widget->CenterYaw, 90.f);
	TestEqual(TEXT("Snapshot marker accepted"), Widget->Entries.Num(), 1);
	if (Widget->Entries.Num() == 1)
	{
		TestEqual(TEXT("VM marker size preserved"), Widget->Entries[0].Marker.Size, 17.f);
		TestEqual(TEXT("Snapshot has no widget expiry"), Widget->Entries[0].ExpiresAt, 0.0);
	}
	TestFalse(TEXT("Legacy upsert cannot add unauthorized data"), Widget->UpsertMarker(Marker));
	TestFalse(TEXT("Legacy remove cannot erase VM data"), Widget->RemoveMarker(Marker.Id));
	Widget->ClearMarkers();
	Widget->ClearMarkersByType(Marker.Type);
	Widget->SetViewLocation(FVector::ZeroVector, 0.f);
	TestEqual(TEXT("Legacy clear cannot erase VM data"), Widget->Entries.Num(), 1);
	TestTrue(TEXT("Legacy view cannot override VM center"), Widget->Center.Equals(Snapshot.ViewLocation));

	Snapshot.Markers.Reset();
	Widget->SetSnapshot(Snapshot);
	TestTrue(TEXT("Empty list removes previous markers"), Widget->Entries.IsEmpty());
	Snapshot.Markers.Add(Marker);
	FNF_MinimapMarker Invalid = Marker;
	Invalid.Id = NAME_None;
	Snapshot.Markers.Add(Invalid);
	Invalid = Marker;
	Invalid.Size = std::numeric_limits<float>::infinity();
	Snapshot.Markers.Add(Invalid);
	Invalid = Marker;
	Invalid.WorldLocation.X = std::numeric_limits<double>::quiet_NaN();
	Snapshot.Markers.Add(Invalid);
	Widget->SetSnapshot(Snapshot);
	TestEqual(TEXT("Invalid markers rejected"), Widget->Entries.Num(), 1);

	Snapshot.bHasView = false;
	Widget->SetSnapshot(Snapshot);
	TestFalse(TEXT("Missing view hides player and markers"), Widget->bHasView);
	TestTrue(TEXT("Missing view clears stored markers"), Widget->Entries.IsEmpty());
	TestTrue(TEXT("Missing view clears old center"), Widget->Center.IsZero());
	Snapshot.bHasView = true;
	Snapshot.ViewYaw = std::numeric_limits<float>::quiet_NaN();
	Widget->SetSnapshot(Snapshot);
	TestFalse(TEXT("Invalid heading invalidates view"), Widget->bHasView);
	Snapshot.ViewYaw = 0.f;
	Snapshot.ViewLocation.X = std::numeric_limits<double>::infinity();
	Widget->SetSnapshot(Snapshot);
	TestFalse(TEXT("Invalid location invalidates view"), Widget->bHasView);
	Snapshot.ViewLocation = FVector::ZeroVector;
	Widget->SetSnapshot(Snapshot);
	TestTrue(TEXT("Later valid snapshot restores display"), Widget->bHasView);
	return true;
}
#endif
