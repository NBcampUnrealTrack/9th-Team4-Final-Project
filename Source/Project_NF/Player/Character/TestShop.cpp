#include "TestShop.h"

#include "Kismet/KismetSystemLibrary.h"


ATestShop::ATestShop()
{
	bReplicates = true;
}

void ATestShop::NF_Interaction(const APlayerController* InteractedController)
{
	OpenShop(InteractedController);
}

void ATestShop::OpenShop(const APlayerController* InteractedController)
{
	
	UKismetSystemLibrary::PrintString(InteractedController,
										  (TEXT("상점 UI 오픈")),
										  true, true, FColor::Magenta, 10.0f);
}



