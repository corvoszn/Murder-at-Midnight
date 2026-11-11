// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MurderAtMidnightProp.h"
#include "MurderAtMidnightCabinet.generated.h"

/** The one test cabinet. It opens only when the carrier has the matching key. */
UCLASS()
class MURDERATMIDNIGHT_API AMurderAtMidnightCabinet : public AMurderAtMidnightProp
{
	GENERATED_BODY()

public:
	AMurderAtMidnightCabinet();
};
