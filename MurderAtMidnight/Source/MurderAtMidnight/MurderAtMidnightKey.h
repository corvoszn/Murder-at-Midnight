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
	/** Round pickup volume. The visible mesh stays smaller than this. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USphereComponent> PickupVolume;
};
