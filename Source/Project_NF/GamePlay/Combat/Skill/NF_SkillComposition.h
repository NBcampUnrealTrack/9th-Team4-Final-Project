#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"

// @Owner: 스킬 담당 (구현: Codex) | 2026-10-14
// 목적: 실행 인스턴스뿐 아니라 BP의 SCS 컴포넌트도 포함해 조합을 검증한다.
template<class T>
void NF_GetCompositionComponents(const AActor* Actor, TArray<T*>& Out)
{
	Actor->GetComponents(Out);
	if (Actor->HasAnyFlags(RF_ClassDefaultObject))
	{
		UBlueprintGeneratedClass* ActualClass = Cast<UBlueprintGeneratedClass>(Actor->GetClass());
		for (UClass* Class = Actor->GetClass(); Class; Class = Class->GetSuperClass())
		{
			UBlueprintGeneratedClass* BlueprintClass = Cast<UBlueprintGeneratedClass>(Class);
			if (!BlueprintClass || !BlueprintClass->SimpleConstructionScript) { continue; }
			for (const USCS_Node* Node : BlueprintClass->SimpleConstructionScript->GetAllNodes())
			{
				if (T* Component = Cast<T>(Node->GetActualComponentTemplate(ActualClass))) { Out.AddUnique(Component); }
			}
		}
	}
}
