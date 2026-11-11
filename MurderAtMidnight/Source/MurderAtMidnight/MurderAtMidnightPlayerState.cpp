// Copyright Epic Games, Inc. All Rights Reserved.

#include "MurderAtMidnightPlayerState.h"
#include "Net/UnrealNetwork.h"

AMurderAtMidnightPlayerState::AMurderAtMidnightPlayerState()
{
}

void AMurderAtMidnightPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Public fact. Every client receives this.
	DOREPLIFETIME(AMurderAtMidnightPlayerState, LifeState);
}
