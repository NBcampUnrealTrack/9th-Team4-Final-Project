#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GamePlay/Combat/Projectiles/NF_BossHomingProjectile.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossHomingVolley.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossRelocate.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_BossHomingMovementTest, "NF.Boss.Homing.Tracking3D",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_BossHomingMovementTest::RunTest(const FString& Parameters)
{
	const FVector Origin(0, 125, 0);
	const FVector Target(300, -900, 400);
	const FVector Moved = ANF_BossHomingProjectile::CalculateTrackingLocation(Origin, Target, 100.f, 1.f);
	TestTrue(TEXT("Moves exactly Speed * DeltaSeconds"), FMath::IsNearlyEqual(FVector::Dist(Origin, Moved), 100.0, 0.001));
	TestTrue(TEXT("Moves straight toward the target in 3D"),
		(Moved - Origin).GetSafeNormal().Equals((Target - Origin).GetSafeNormal(), 0.001));
	const FVector Arrived = ANF_BossHomingProjectile::CalculateTrackingLocation(Origin, Target, 5000.f, 1.f);
	TestTrue(TEXT("Does not overshoot the target"), Arrived.Equals(Target, 0.001));
	const FVector Sideways = ANF_BossHomingProjectile::CalculateTrackingLocation(Origin, FVector(0, 1000, 0), 100.f, 1.f);
	TestTrue(TEXT("Follows a player moving along Y"), Sideways.Equals(FVector(0, 225, 0), 0.001));
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
