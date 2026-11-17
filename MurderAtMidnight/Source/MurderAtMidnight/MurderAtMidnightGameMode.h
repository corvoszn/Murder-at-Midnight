// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/GameModeBase.h"
#include "MurderAtMidnightGameState.h"
#include "MurderAtMidnightGameMode.generated.h"

class AMurderAtMidnightPlayerController;

UENUM(BlueprintType)
enum class EMurderAtMidnightAgendaCheck : uint8
{
	None UMETA(DisplayName = "No check yet"),
	CabinetUnlockThenLock UMETA(DisplayName = "Unlock the cabinet, then lock it"),
	DoorUnlockWithKey UMETA(DisplayName = "Unlock the door while holding the key")
};

/** One sentence in the agenda table. The row name is the id. */
USTRUCT(BlueprintType)
struct FMurderAtMidnightAgendaRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agenda")
	FText AgendaText;

	/** The server check that marks this row done. Each row picks its own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agenda", meta = (DisplayName = "Finished By"))
	EMurderAtMidnightAgendaCheck Completion = EMurderAtMidnightAgendaCheck::None;
};

struct FMurderAtMidnightAssignedAgenda
{
	FString Text;
	EMurderAtMidnightAgendaCheck Completion = EMurderAtMidnightAgendaCheck::None;
	bool bUnlockedCabinet = false;
	bool bComplete = false;
};

/**
 *  Server-only match authority for a first person game.
 *  This step advances the public phase and clock, keeps each player's agenda, and assigns roles.
 *  It also checks the evening result. There is no hunt in this build.
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
	bool TryMurder(AMurderAtMidnightPlayerController* Attacker, AMurderAtMidnightPlayerController* Victim, FString& OutMessage);

	/** Server only. A player with the key opened or locked the cabinet. */
	void NotifyCabinetLockChanged(AMurderAtMidnightPlayerController* Who, bool bIsOpen);

	/** Server only. A player with the key in hand opened the door. */
	void NotifyDoorOpened(AMurderAtMidnightPlayerController* Who);

	/** Server only. During a gathering, a living player asks to open a vote. */
	bool TryCallVote(AMurderAtMidnightPlayerController* Who, FString& OutMessage);

	/** Server only. A living player names a living player while a vote is open. */
	bool TryCastVote(AMurderAtMidnightPlayerController* Who, AMurderAtMidnightPlayerController* Accused, FString& OutMessage);

	/** Server only. Testing shortcut. Moves one step: Arrival, Dinner, Garden, then midnight. */
	void SkipToNextPhase();

protected:
	/** Real-time length of Arrival, in seconds. Exposed so Blueprint can tune it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match", meta = (ClampMin = "1.0"))
	float ArrivalDurationSeconds = 120.0f;

	/** Real-time length of Dinner, in seconds. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match", meta = (ClampMin = "1.0"))
	float DinnerDurationSeconds = 120.0f;

	/** Real-time length of the Garden party, in seconds. Midnight is checked when this ends. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match", meta = (ClampMin = "1.0"))
	float GardenDurationSeconds = 120.0f;

	/** Players allowed in one match, including the killer. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match", meta = (ClampMin = "1", ClampMax = "8"))
	int32 MaxPlayers = 8;

	/** Sentences dealt to guests. Empty uses one cabinet sentence so a match can still be tested. */
	UPROPERTY(EditDefaultsOnly, Category = "Match")
	TObjectPtr<UDataTable> AgendaTable;

private:
	FTimerHandle PhaseTimer;

	/** The agenda dealt to each guest. Server memory only. Not a replicated property. */
	TMap<TObjectPtr<AMurderAtMidnightPlayerController>, FMurderAtMidnightAssignedAgenda> AssignedAgendas;

	/** The killer. Server memory only. Not a replicated property. */
	TObjectPtr<AMurderAtMidnightPlayerController> Killer;

	/** The killer's targets. Server memory only. Not a replicated property. */
	TArray<TObjectPtr<AMurderAtMidnightPlayerController>> Targets;

	bool bRolesAssigned = false;
	int32 NextGuestNumber = 0;

	/** Ballots for the open vote. Server memory only. Not a replicated property. */
	TMap<TObjectPtr<AMurderAtMidnightPlayerController>, TObjectPtr<AMurderAtMidnightPlayerController>> Ballots;

	/** The open vote is the midnight vote. A failed midnight vote is a killer win. */
	bool bMidnightVote = false;

	void AssignPublicName(APlayerController* Controller);
	void AssignAgendas();
	TArray<FMurderAtMidnightAgendaRow> CollectAgendaRows() const;
	void TryAssignRoles();
	int32 TargetCountForLobby(int32 PlayerCount) const;
	void GatherPlayers(TArray<AMurderAtMidnightPlayerController*>& OutPlayers) const;
	void MarkAlive(AController* Controller);

	void StartPhase(EMurderAtMidnightPhase NewPhase);
	void AdvancePhase();
	void CheckEveningResult();
	void CheckAgendaWin();
	void OpenVote(bool bAtMidnight);
	void ResolveVoteIfReady();
	void EndMatch(EMurderAtMidnightMatchResult Result, const FString& Announcement);
	void TellPlayers(const FString& Message);
	bool IsAlive(const AMurderAtMidnightPlayerController* Controller) const;
	int32 CountLiving() const;
	bool AllAgendasComplete() const;
	bool HasLivingTarget() const;
	bool IsGathering(EMurderAtMidnightPhase Phase) const;
	float GetPhaseDuration(EMurderAtMidnightPhase Phase) const;
	void SpawnTestKeyAndCabinet();
};
