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
	DOREPLIFETIME(AMurderAtMidnightGameState, PhaseStartedAtServerTime);
	DOREPLIFETIME(AMurderAtMidnightGameState, PhaseDurationSeconds);
	DOREPLIFETIME(AMurderAtMidnightGameState, FrozenClockMinute);
	DOREPLIFETIME(AMurderAtMidnightGameState, bVoteOpen);
	DOREPLIFETIME(AMurderAtMidnightGameState, bVoteLocked);
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

int32 AMurderAtMidnightGameState::GetClockMinute() const
{
	if (FrozenClockMinute >= 0)
	{
		return FrozenClockMinute;
	}

	int32 StartMinute = 0;
	switch (Phase)
	{
	case EMurderAtMidnightPhase::Arrival:
		StartMinute = 0;
		break;
	case EMurderAtMidnightPhase::Dinner:
		StartMinute = 120;
		break;
	case EMurderAtMidnightPhase::Garden:
		StartMinute = 240;
		break;
	case EMurderAtMidnightPhase::Resolution:
		return 360;
	default:
		return 0;
	}

	if (PhaseDurationSeconds <= 0.0f)
	{
		return StartMinute;
	}

	const double Elapsed = FMath::Clamp(
		GetServerWorldTimeSeconds() - PhaseStartedAtServerTime,
		0.0,
		static_cast<double>(PhaseDurationSeconds));
	const int32 IntoPhase = FMath::Clamp(
		FMath::FloorToInt(static_cast<float>(Elapsed / PhaseDurationSeconds) * 120.0f),
		0,
		119);
	return StartMinute + IntoPhase;
}

FString AMurderAtMidnightGameState::GetClockLabel() const
{
	const int32 MinuteOfEvening = FMath::Clamp(GetClockMinute(), 0, 360);
	if (MinuteOfEvening >= 360)
	{
		return TEXT("00:00");
	}

	const int32 Hour = 18 + MinuteOfEvening / 60;
	const int32 Minute = MinuteOfEvening % 60;
	return FString::Printf(TEXT("%02d:%02d"), Hour, Minute);
}
