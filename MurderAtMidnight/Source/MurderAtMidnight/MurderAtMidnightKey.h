// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MurderAtMidnightProp.h"
#include "MurderAtMidnightKey.generated.h"

class USphereComponent;

/** The one test key. Its item id matches the cabinet lock. */
UCLASS()
class MURDERATMIDNIGHT_API AMurderAtMidnightKey : public AMurderAtMidnightProp
{
	GENERATED_BODY()

public:
	AMurderAtMidnightKey();

private:
	/** Look target only. The mesh owns the physical shape. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USphereComponent> PickupVolume;
};
