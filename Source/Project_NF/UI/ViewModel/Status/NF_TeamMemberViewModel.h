#pragma once

#include "CoreMinimal.h"
#include "UI/ViewModel/Status/NF_PlayerStatusViewModel.h"
#include "NF_TeamMemberViewModel.generated.h"

class APlayerState;

// 팀원 목록이 소유한다. 로컬 플레이어용 Subsystem에 개별 등록하지 않는다.
UCLASS(BlueprintType)
class PROJECT_NF_API UNF_TeamMemberViewModel : public UNF_PlayerStatusViewModel
{
	GENERATED_BODY()

public:
	// 해당 팀원의 PlayerState와 ASC를 연결한다. 재호출 시 이름과 ASC를 갱신한다.
	void SetMember(APlayerState* NewPlayerState);

	APlayerState* GetMemberPlayerState() const;

	// 레벨 데이터 제공자가 변경 시 호출한다.
	UFUNCTION(BlueprintCallable, Category = "NF|UI|Team")
	void SetMemberLevel(int32 NewLevel);

protected:
	virtual void OnDeinitializeViewModel() override;

private:
	TWeakObjectPtr<APlayerState> MemberPlayerState;
};
