#include "GamePlay/Combat/Skill/Active/Attack/NF_AirStrike.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GamePlay/Combat/Components/NF_AreaIndicatorComponent.h"
#include "GamePlay/Combat/Projectiles/NF_AirStrikeBomb.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ANF_AirStrike::ANF_AirStrike()
{
	bReplicates = true;
	SetReplicateMovement(true);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PlaneBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaneBody"));
	SetRootComponent(PlaneBody);
	PlaneBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaneBody->SetRelativeScale3D(FVector(6.0f, 1.0f, 0.4f));
	PlaneWings = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaneWings"));
	PlaneWings->SetupAttachment(PlaneBody);
	PlaneWings->SetUsingAbsoluteScale(true);
	PlaneWings->SetRelativeScale3D(FVector(1.5f, 7.0f, 0.15f));
	PlaneWings->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		PlaneBody->SetStaticMesh(Cube.Object);
		PlaneWings->SetStaticMesh(Cube.Object);
	}
	WarningArea = CreateDefaultSubobject<UNF_AreaIndicatorComponent>(TEXT("WarningArea"));
	BombClass = ANF_AirStrikeBomb::StaticClass();
}

bool ANF_AirStrike::IsStrikeConfigured() const
{
	return BombClass != nullptr;
}

void ANF_AirStrike::InitializeStrike(FVector Center, FVector Direction, const FGameplayEffectSpecHandle& InDamageSpec)
{
	if (!HasAuthority())
	{
		return;
	}
	StrikeCenter = Center;
	FlightDirection = FVector(Direction.X, Direction.Y, 0.0f).GetSafeNormal();
	if (FlightDirection.IsNearlyZero())
	{
		FlightDirection = FVector::ForwardVector;
	}
	ReplicatedRadius = GetStrikeRadius();
	DamageSpec = InDamageSpec;
}

void ANF_AirStrike::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANF_AirStrike, StrikeCenter);
	DOREPLIFETIME(ANF_AirStrike, ReplicatedRadius);
	DOREPLIFETIME(ANF_AirStrike, bFlightVisible);
}

void ANF_AirStrike::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		// FinishSpawning의 생성 Transform 적용 이후 비행 시작 위치를 설정한다.
		SetActorLocation(StrikeCenter - FlightDirection * (ReplicatedRadius + FMath::Max(ApproachDistance, 0.0f))
			+ FVector::UpVector * FMath::Max(FlightAltitude, 100.0f));
		SetActorRotation(FlightDirection.Rotation());
	}
	RefreshVisuals();
	if (HasAuthority())
	{
		if (WarningDelay > 0.0f)
		{
			GetWorldTimerManager().SetTimer(StartTimer, this, &ANF_AirStrike::StartFlight, WarningDelay, false);
		}
		else
		{
			StartFlight();
		}
	}
}

void ANF_AirStrike::RefreshVisuals()
{
	PlaneBody->SetVisibility(bFlightVisible);
	PlaneWings->SetVisibility(bFlightVisible);
	if (GetNetMode() != NM_DedicatedServer)
	{
		WarningArea->ShowArea(StrikeCenter, ReplicatedRadius, FVector::UpVector);
	}
}

void ANF_AirStrike::StartFlight()
{
	if (!HasAuthority())
	{
		return;
	}
	const FVector Side = FVector::CrossProduct(FVector::UpVector, FlightDirection);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(NF_AirStrikeTargets), false, this);
	Query.AddIgnoredActor(GetOwner());
	for (int32 Index = 0; Index < FMath::Clamp(BombCount, 1, 128); ++Index)
	{
		const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
		const float Distance = ReplicatedRadius * FMath::Sqrt(FMath::FRand());
		const FVector Point = StrikeCenter + FlightDirection * (FMath::Cos(Angle) * Distance)
			+ Side * (FMath::Sin(Angle) * Distance);
		FHitResult Ground;
		const FVector Top = Point + FVector::UpVector * FMath::Max(FlightAltitude, 100.0f);
		if (GetWorld()->LineTraceSingleByChannel(Ground, Top,
			Point - FVector::UpVector * FMath::Max(GroundSearchDepth, 100.0f), ECC_Visibility, Query)
			&& Ground.ImpactNormal.Z >= 0.3f)
		{
			BombTargets.Add(Ground.ImpactPoint);
		}
	}
	BombTargets.Sort([this](const FVector& A, const FVector& B)
	{
		return FVector::DotProduct(A - StrikeCenter, FlightDirection)
			< FVector::DotProduct(B - StrikeCenter, FlightDirection);
	});
	bFlightActive = true;
	bFlightVisible = true;
	RefreshVisuals();
	ForceNetUpdate();
	SetActorTickEnabled(true);
}

void ANF_AirStrike::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || !bFlightActive)
	{
		return;
	}
	const float HalfLength = ReplicatedRadius + FMath::Max(ApproachDistance, 0.0f);
	Travel += FMath::Max(FlightSpeed, 100.0f) * DeltaSeconds;
	const float Along = FMath::Min(Travel, 2.0f * HalfLength) - HalfLength;
	SetActorLocation(StrikeCenter + FlightDirection * Along + FVector::UpVector * FMath::Max(FlightAltitude, 100.0f));
	while (NextBomb < BombTargets.Num()
		&& FVector::DotProduct(BombTargets[NextBomb] - StrikeCenter, FlightDirection) <= Along)
	{
		DropBomb(BombTargets[NextBomb++]);
	}
	if (Travel >= 2.0f * HalfLength)
	{
		bFlightActive = false;
		bFlightVisible = false;
		RefreshVisuals();
		ForceNetUpdate();
		SetActorTickEnabled(false);
		// 비행이 끝나도 마지막 폭탄의 낙하 동안 경고를 유지한다.
		SetLifeSpan(FMath::Max(MaxFallTime + 0.5f, 0.5f));
	}
}

void ANF_AirStrike::DropBomb(FVector Target)
{
	if (!BombClass)
	{
		return;
	}
	const FVector Start = GetActorLocation();
	const ANF_AirStrikeBomb* Defaults = BombClass->GetDefaultObject<ANF_AirStrikeBomb>();
	const float Height = Start.Z - Target.Z;
	if (Height <= 10.0f)
	{
		return;
	}
	const float DownSpeed = FMath::Max(InitialFallSpeed, 1.0f);
	const float Gravity = FMath::Max(-GetWorld()->GetGravityZ() * Defaults->GetGravityScale(), 0.0f);
	const float FallTime = Gravity > SMALL_NUMBER
		? (FMath::Sqrt(DownSpeed * DownSpeed + 2.0f * Gravity * Height) - DownSpeed) / Gravity
		: Height / DownSpeed;
	FVector Velocity = (Target - Start) / FMath::Max(FallTime, 0.01f);
	Velocity.Z = -DownSpeed;
	const FTransform Transform(Velocity.Rotation(), Start);
	ANF_AirStrikeBomb* Bomb = GetWorld()->SpawnActorDeferred<ANF_AirStrikeBomb>(BombClass,
		Transform, GetOwner(), GetInstigator(), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Bomb)
	{
		return;
	}
	// 한 프레임에 여러 폭탄을 투하해도 폭탄끼리 충돌해 즉시 폭발하지 않게 한다.
	for (const TWeakObjectPtr<ANF_AirStrikeBomb>& Other : DroppedBombs)
	{
		if (Other.IsValid())
		{
			Bomb->IgnoreOtherBomb(Other.Get());
			Other->IgnoreOtherBomb(Bomb);
		}
	}
	Bomb->InitializeBomb(Velocity, DamageSpec);
	Bomb->FinishSpawning(Transform);
	DroppedBombs.Add(Bomb);
	MaxFallTime = FMath::Max(MaxFallTime, FallTime);
}

void ANF_AirStrike::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(StartTimer);
	WarningArea->HideArea();
	Super::EndPlay(EndPlayReason);
}
