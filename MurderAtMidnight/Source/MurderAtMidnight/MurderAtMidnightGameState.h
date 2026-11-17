// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "MurderAtMidnightGameState.generated.h"

/** Public evening phase. Every client receives this. It is not a secret. */
UENUM(BlueprintType)
enum class EMurderAtMidnightPhase : uint8
{
	WaitingForPlayers UMETA(DisplayName = "Waiting For Players"),
	Arrival UMETA(DisplayName = "Arrival"),
	Dinner UMETA(DisplayName = "Dinner"),
	Garden UMETA(DisplayName = "Garden"),
	/** Midnight. The clock is stopped. This is the final check, not another period of play. */
	Resolution UMETA(DisplayName = "Resolution"),
	/** Unused. The hunt is not part of this build. */
	Hunt UMETA(DisplayName = "Hunt"),
	Ended UMETA(DisplayName = "Ended")
};

/** Public match result. None until the server ends the match. */
UENUM(BlueprintType)
enum class EMurderAtMidnightMatchResult : uint8
{
	None UMETA(DisplayName = "None"),
	GuestsWin UMETA(DisplayName = "Guests Win"),
	KillerWins UMETA(DisplayName = "Killer Wins")
};

/**
 * Public match facts. Replicated to every player.
 * The killer, the target list, and agendas do not belong here.
 * The GameMode is the only writer. This class does not decide a winner.
 */
UCLASS()
class MURDERATMIDNIGHT_API AMurderAtMidnightGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AMurderAtMidnightGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Current phase. Replicated to every client. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	EMurderAtMidnightPhase Phase = EMurderAtMidnightPhase::WaitingForPlayers;

	/** Server world time when the current phase ends. Replicated to every client. Zero when the clock is stopped. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	double PhaseEndsAtServerTime = 0.0;

	/** Server world time when the current phase started. Replicated so every client can draw the same clock. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	double PhaseStartedAtServerTime = 0.0;

	/** Real seconds the current phase lasts. Zero at midnight and before the evening starts. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	float PhaseDurationSeconds = 0.0f;

	/** Minutes after 18:00 when the match ended. -1 while the clock is still running. 360 is midnight. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	int32 FrozenClockMinute = -1;

	/** Whether a hosted vote is open. Replicated to every client. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	bool bVoteOpen = false;

	/** A failed gathering vote. Cleared when the next gathering starts. Replicated to every client. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	bool bVoteLocked = false;

	/** Public result after the match ends. Replicated to every client. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	EMurderAtMidnightMatchResult MatchResult = EMurderAtMidnightMatchResult::None;

	/** Seconds left in the current phase, from the replicated server clock. */
	UFUNCTION(BlueprintPure, Category = "Match")
	double GetPhaseSecondsRemaining() const;

	/** Minutes after 18:00. 0 is 18:00. 360 is midnight. */
	UFUNCTION(BlueprintPure, Category = "Match")
	int32 GetClockMinute() const;

	/** The evening clock, from 18:00 to 00:00. */
	UFUNCTION(BlueprintPure, Category = "Match")
	FString GetClockLabel() const;
};
