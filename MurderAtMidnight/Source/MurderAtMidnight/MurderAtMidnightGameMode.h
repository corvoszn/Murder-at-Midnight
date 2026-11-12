// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MurderAtMidnightGameState.h"
#include "MurderAtMidnightGameMode.generated.h"

class AMurderAtMidnightPlayerController;

struct FMurderAtMidnightAgendaProgress
{
	bool bUnlockedCabinet = false;
	bool bComplete = false;
};

/**
 *  Server-only match authority for a first person game.
 *  This step advances the public phase and clock, keeps each player's agenda, and assigns roles.
 *  It does not decide a winner.
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

	/** Server only. Repeats this player's role. The client does not choose it. */
	void SendPrivateRole(AMurderAtMidnightPlayerController* Controller);

	/** Server only. The killer is close to a living player and has asked to kill them. */
	void TryMurder(AMurderAtMidnightPlayerController* Attacker, AMurderAtMidnightPlayerController* Victim, FString& OutMessage);

	/** Server only. A player with the key opened or locked the cabinet. */
	void NotifyCabinetLockChanged(AMurderAtMidnightPlayerController* Who, bool bIsOpen);

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

	/** Whether each guest has unlocked the cabinet, and whether their agenda is done. Server memory only. */
	TMap<TObjectPtr<APlayerController>, FMurderAtMidnightAgendaProgress> AgendaProgress;

	/** The killer. Server memory only. Not a replicated property. */
	TObjectPtr<AMurderAtMidnightPlayerController> Killer;

	/** The killer's targets. Server memory only. Not a replicated property. */
	TArray<TObjectPtr<AMurderAtMidnightPlayerController>> Targets;

	bool bRolesAssigned = false;
	int32 NextGuestNumber = 0;

	void EnsureAgendaTexts();
	FString BuildAgendaLine(const AMurderAtMidnightPlayerController* Controller) const;
	void AssignPublicName(APlayerController* Controller);
	void TryAssignRoles();
	int32 TargetCountForLobby(int32 PlayerCount) const;
	void GatherPlayers(TArray<AMurderAtMidnightPlayerController*>& OutPlayers) const;
	void MarkAlive(AController* Controller);

	void StartPhase(EMurderAtMidnightPhase NewPhase);
	void AdvancePhase();
	float GetPhaseDuration(EMurderAtMidnightPhase Phase) const;
	void SpawnTestKeyAndCabinet();
};
