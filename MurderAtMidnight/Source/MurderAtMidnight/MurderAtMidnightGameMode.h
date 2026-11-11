// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MurderAtMidnightGameState.h"
#include "MurderAtMidnightGameMode.generated.h"

class AMurderAtMidnightPlayerController;

/**
 *  Server-only match authority for a first person game.
 *  This step advances the public phase and clock, and keeps each player's agenda.
 *  It does not assign roles or decide a winner.
 *  Public facts are written onto the GameState. Private facts go to one PlayerController.
 */
UCLASS(abstract)
class AMurderAtMidnightGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMurderAtMidnightGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** Server only. Assigns an agenda if this player does not have one, then tells that controller. */
	void SendPrivateAgenda(AMurderAtMidnightPlayerController* Controller);

protected:
	/** Real-time length of Arrival, in seconds. Exposed so Blueprint can tune it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match", meta = (ClampMin = "1.0"))
	float ArrivalDurationSeconds = 120.0f;

	/** Real-time length of Dinner, in seconds. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match", meta = (ClampMin = "1.0"))
	float DinnerDurationSeconds = 120.0f;

	/** Real-time length of the Garden party, in seconds. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match", meta = (ClampMin = "1.0"))
	float GardenDurationSeconds = 120.0f;

	/** Real-time length of Resolution, in seconds. Ending it does not pick a winner yet. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match", meta = (ClampMin = "1.0"))
	float ResolutionDurationSeconds = 120.0f;

	/** Players allowed in one match, including the killer. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match", meta = (ClampMin = "1", ClampMax = "8"))
	int32 MaxPlayers = 8;

	/** Agenda lines. This list lives on the server. It is not replicated. */
	UPROPERTY(EditDefaultsOnly, Category = "Match")
	TArray<FString> AgendaTexts;

private:
	FTimerHandle PhaseTimer;

	/** Agenda text by controller. Server memory only. Not a replicated property. */
	TMap<TObjectPtr<APlayerController>, FString> PrivateAgendas;

	void EnsureAgendaTexts();

	void StartPhase(EMurderAtMidnightPhase NewPhase);
	void AdvancePhase();
	float GetPhaseDuration(EMurderAtMidnightPhase Phase) const;
	void SpawnTestKeyAndCabinet();
};
