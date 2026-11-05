#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GamePlay/Combat/Projectiles/NF_BossHomingProjectile.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossHomingVolley.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossRelocate.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_BossHomingMovementTest, "NF.Boss.Homing.WorldXZTracking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_BossHomingMovementTest::RunTest(const FString& Parameters)
{
	const FVector Origin(0, 125, 0);
	const FVector Target(300, -900, 400);
	const FVector Moved = ANF_BossHomingProjectile::CalculateTrackingLocation(Origin, Target, 100.f, 1.f);
	TestTrue(TEXT("Tracks X/Z while preserving launch Y"), Moved.Equals(FVector(60, 125, 80), 0.001));
	const FVector Arrived = ANF_BossHomingProjectile::CalculateTrackingLocation(Origin, Target, 1000.f, 1.f);
	TestTrue(TEXT("Does not overshoot the target plane projection"), Arrived.Equals(FVector(300, 125, 400), 0.001));
	const FVector Sideways = ANF_BossHomingProjectile::CalculateTrackingLocation(Origin, FVector(0, 1000, 0), 1000.f, 1.f);
	TestTrue(TEXT("A player moving only along Y cannot drag the missile sideways"), Sideways.Equals(Origin));
	FVector Substeps = Origin;
	for (int32 Index = 0; Index < 10; ++Index)
	{
		Substeps = ANF_BossHomingProjectile::CalculateTrackingLocation(Substeps, Target, 100.f, 0.1f);
	}
	TestTrue(TEXT("Tracking speed is independent of frame subdivision"), Substeps.Equals(Moved, 0.001));
	TestTrue(TEXT("Saved Relocate StateTree references now execute homing"),
		UNFGA_BossRelocate::StaticClass()->IsChildOf(UNFGA_BossHomingVolley::StaticClass()));
	return true;
}
#endif
