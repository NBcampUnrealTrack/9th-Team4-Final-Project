#include "GamePlay/Combat/Weapons/NF_WeaponCameraShakeBase.h"

#include "Shakes/PerlinNoiseCameraShakePattern.h"

UNF_WeaponCameraShakeBase::UNF_WeaponCameraShakeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 같은 클래스의 흔들림은 연사 시 중첩 대신 재시작한다.
	bSingleInstance = true;

	UPerlinNoiseCameraShakePattern* Pattern =
		CreateDefaultSubobject<UPerlinNoiseCameraShakePattern>(TEXT("FireShakePattern"));
	SetRootShakePattern(Pattern);

	// 발사 직후 빠르게 시작하고 부드럽게 끝나는 짧은 흔들림(초).
	// Duration이 0 이하이면 무한 재생이므로 발사용 BP에는 양수를 사용한다.
	Pattern->Duration = 0.12f;
	Pattern->BlendInTime = 0.01f;
	Pattern->BlendOutTime = 0.08f;

	// 위치 이동(cm)은 기본적으로 끈다. 필요하면 BP에서 축별 Amplitude를 올린다.
	Pattern->X.Amplitude = 0.f;
	Pattern->Y.Amplitude = 0.f;
	Pattern->Z.Amplitude = 0.f;
	Pattern->X.Frequency = 20.f;
	Pattern->Y.Frequency = 20.f;
	Pattern->Z.Frequency = 20.f;

	// 회전 크기는 도 단위. 상하(Pitch)를 중심으로 약한 좌우/기울기를 더한다.
	Pattern->Pitch.Amplitude = 0.6f;
	Pattern->Pitch.Frequency = 25.f;
	Pattern->Yaw.Amplitude = 0.2f;
	Pattern->Yaw.Frequency = 20.f;
	Pattern->Roll.Amplitude = 0.1f;
	Pattern->Roll.Frequency = 20.f;

	// 조준/스코프 FOV 연출과 겹치지 않도록 시야각 흔들림은 기본적으로 끈다.
	Pattern->FOV.Amplitude = 0.f;
	Pattern->FOV.Frequency = 20.f;
}
