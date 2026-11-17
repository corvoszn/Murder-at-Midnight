// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MurderAtMidnightProp.h"
#include "MurderAtMidnightWeapon.generated.h"

class USphereComponent;

/** A blunt object. It can be carried and used. It records that it was used, never who used it. */
UCLASS()
class MURDERATMIDNIGHT_API AMurderAtMidnightWeapon : public AMurderAtMidnightProp
{
	GENERATED_BODY()

public:
	AMurderAtMidnightWeapon();

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USphereComponent> PickupVolume;
};
