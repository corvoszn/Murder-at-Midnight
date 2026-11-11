// Copyright Epic Games, Inc. All Rights Reserved.

#include "MurderAtMidnightGameState.h"
#include "Net/UnrealNetwork.h"

AMurderAtMidnightGameState::AMurderAtMidnightGameState()
{
}

void AMurderAtMidnightGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Public facts. Every client receives these.
	DOREPLIFETIME(AMurderAtMidnightGameState, Phase);
	DOREPLIFETIME(AMurderAtMidnightGameState, PhaseEndsAtServerTime);
	DOREPLIFETIME(AMurderAtMidnightGameState, bVoteOpen);
	DOREPLIFETIME(AMurderAtMidnightGameState, MatchResult);
}

double AMurderAtMidnightGameState::GetPhaseSecondsRemaining() const
{
	if (PhaseEndsAtServerTime <= 0.0)
	{
		return 0.0;
	}

	return FMath::Max(0.0, PhaseEndsAtServerTime - GetServerWorldTimeSeconds());
}
