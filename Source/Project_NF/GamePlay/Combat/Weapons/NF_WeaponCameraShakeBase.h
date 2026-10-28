#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "NF_WeaponCameraShakeBase.generated.h"

/**
 * 발사 시 사용하는 짧은 카메라 흔들림.
 * 자식 BP의 Class Defaults > Root Shake Pattern에서 무기별로 조절한다.
 * Timing: 지속/블렌드 시간(초), Amplitude: 흔들림 크기, Frequency: 흔들림 속도.
 * 화면 연출용이며 조준 반동의 누적/복귀는 별도로 처리한다.
 */
UCLASS(Blueprintable)
class PROJECT_NF_API UNF_WeaponCameraShakeBase : public UCameraShakeBase
{
	GENERATED_BODY()
public:
	UNF_WeaponCameraShakeBase(const FObjectInitializer& ObjectInitializer);
};
