// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MurderAtMidnightProp.h"
#include "MurderAtMidnightDoor.generated.h"

/** The one test door. It uses the same key check as the cabinet. */
UCLASS()
class MURDERATMIDNIGHT_API AMurderAtMidnightDoor : public AMurderAtMidnightProp
{
	GENERATED_BODY()

public:
	AMurderAtMidnightDoor();
};
