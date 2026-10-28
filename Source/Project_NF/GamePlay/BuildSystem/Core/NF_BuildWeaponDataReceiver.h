#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NF_BuildWeaponDataReceiver.generated.h"

class UNF_WeaponDataAsset;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UNF_BuildWeaponDataReceiver : public UInterface
{
	GENERATED_BODY()
};

// @Owner: 황준현 | 2026-10-28. 무기 담당자가 구현할 서버 장착 전 초기화 계약.
// 성공 시 GetWeaponData()가 NewData를 반환해야 한다. 탄약/외형 초기화와
// WeaponData 복제 및 클라이언트 갱신은 무기 구현이 담당한다.
class PROJECT_NF_API INF_BuildWeaponDataReceiver
{
	GENERATED_BODY()

public:
	virtual bool ApplyBuildWeaponData(UNF_WeaponDataAsset* NewData) = 0;
};
