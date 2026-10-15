#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UI/Minimap/NF_MinimapWidget.h"

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
#endif
