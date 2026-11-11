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
	Resolution UMETA(DisplayName = "Resolution"),
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

	/** Server world time when the current phase ends. Replicated to every client. Zero until the clock is started. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	double PhaseEndsAtServerTime = 0.0;

	/** Whether a hosted vote is open. Replicated to every client. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	bool bVoteOpen = false;

	/** Public result after the match ends. Replicated to every client. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	EMurderAtMidnightMatchResult MatchResult = EMurderAtMidnightMatchResult::None;

	/** Seconds left in the current phase, from the replicated server clock. */
	UFUNCTION(BlueprintPure, Category = "Match")
	double GetPhaseSecondsRemaining() const;
};
