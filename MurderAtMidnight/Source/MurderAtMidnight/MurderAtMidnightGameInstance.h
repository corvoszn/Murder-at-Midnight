// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MurderAtMidnightGameInstance.generated.h"

/**
 * Lives on each machine for the whole session. It is not replicated.
 * Hosting and local settings belong here later.
 * The killer, the target list, and agendas must not be stored here.
 */
UCLASS()
class MURDERATMIDNIGHT_API UMurderAtMidnightGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UMurderAtMidnightGameInstance();
};
