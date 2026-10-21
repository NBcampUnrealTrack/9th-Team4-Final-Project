#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GamePlay/BuildSystem/Core/NF_BuildTypes.h"
#include "NF_BuildShopScreen.generated.h"

class UNF_BuildComponent;
class UNF_BuildShopViewModel;

// @Owner: 황준현 | 2026-10-20
// 플레이어의 빌드 상태를 기존 상점 ViewModel에 전달한다.
UCLASS()
class PROJECT_NF_API UNF_BuildShopScreen : public UUserWidget
{
	GENERATED_BODY()

public:
	// Blueprint 버튼은 반환된 컴포넌트의 선택·배치·적용 API를 사용한다.
	UFUNCTION(BlueprintPure, Category = "NF|Build|UI")
	UNF_BuildComponent* GetBuildComponent() const
	{
		return Build;
	}

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 생성된 인스턴스를 Blueprint의 MVVM ViewModel에 연결한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Build|UI")
	void OnViewModelReady(UNF_BuildShopViewModel* InViewModel);

	// 서버 적용 결과를 안내 문구 등에 표시한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Build|UI")
	void OnBuildResult(const FNF_BuildValidation& Validation);

	UPROPERTY(Transient, BlueprintReadOnly, Category = "NF|Build|UI")
	TObjectPtr<UNF_BuildShopViewModel> ShopViewModel;

private:
	UFUNCTION()
	void RefreshViewModel();

	UFUNCTION()
	void HandleBuildResult(const FNF_BuildValidation& Validation);

	UPROPERTY(Transient)
	TObjectPtr<UNF_BuildComponent> Build;
};