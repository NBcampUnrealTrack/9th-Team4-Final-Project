#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GamePlay/Combat/Skill/Portal/NF_PortalView.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_PortalViewTransformTest, "NF.Skills.Portal.ViewTransform",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNF_PortalViewTransformTest::RunTest(const FString& Parameters)
{
	const FTransform Entrance(FRotator(0, 180, 0), FVector(300, 0, 110), FVector(2, 3, 4));
	const FTransform Exit(FRotator::ZeroRotator, FVector(1500, 0, 110), FVector(5, 6, 7));
	const FTransform Camera(FRotator(-10, 0, 0), FVector(100, 40, 170));
	const FTransform Through = ANF_PortalView::TransformCamera(Camera, Entrance, Exit);
	TestTrue(TEXT("Default entry/exit mouths preserve the existing forward travel heading"),
		Through.GetRotation().Equals(Camera.GetRotation(), 0.001));
	TestTrue(TEXT("Camera offset maps across portal without applying actor scale"),
		Through.GetLocation().Equals(FVector(1300, 40, 170), 0.01));
	const FTransform Back = ANF_PortalView::TransformCamera(Through, Exit, Entrance);
	TestTrue(TEXT("Reverse view mapping restores camera position"), Back.GetLocation().Equals(Camera.GetLocation(), 0.01));
	TestTrue(TEXT("Reverse view mapping restores camera orientation"), Back.GetRotation().Equals(Camera.GetRotation(), 0.001));
	const FTransform RotatedExit(FRotator(0, 90, 0), Exit.GetLocation());
	const FTransform Turned = ANF_PortalView::TransformCamera(Camera, Entrance, RotatedExit);
	TestTrue(TEXT("Rotated exit also rotates the viewer's lateral/forward offset"),
		Turned.GetLocation().Equals(FVector(1460, -200, 170), 0.01));
	return true;
}
#endif
