// Fill out your copyright notice in the Description page of Project Settings.


#include "NF_BuildShopAccessPolicy.h"


bool FNF_BuildShopAccessPolicy::IsShopAllowed(
	const ENF_MatchPhase MatchPhase,
	const ENF_BuildShopType ShopKind)
{
	switch (MatchPhase)
	{
	case ENF_MatchPhase::Build:
		// 최초 빌드 페이즈에는 팀 거점 상점만 허용.
		return ShopKind == ENF_BuildShopType::TeamBase;

	case ENF_MatchPhase::Farming:
	case ENF_MatchPhase::Boss:
	case ENF_MatchPhase::Escape:
		// 생존 여부는 GAS의 State.Build.Locked 태그가 처리.
		return ShopKind == ENF_BuildShopType::TeamBase || ShopKind == ENF_BuildShopType::Field;

	case ENF_MatchPhase::Waiting:
	case ENF_MatchPhase::Finished:
		
	default:
		return false;
	}
}