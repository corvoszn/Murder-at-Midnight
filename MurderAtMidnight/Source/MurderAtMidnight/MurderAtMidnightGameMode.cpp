// Copyright Epic Games, Inc. All Rights Reserved.

#include "MurderAtMidnightGameMode.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "MurderAtMidnight.h"
#include "MurderAtMidnightCabinet.h"
#include "MurderAtMidnightDoor.h"
#include "MurderAtMidnightWeapon.h"
#include "MurderAtMidnightHUD.h"
#include "MurderAtMidnightKey.h"
#include "MurderAtMidnightPlayerController.h"
#include "MurderAtMidnightPlayerState.h"
#include "TimerManager.h"

AMurderAtMidnightGameMode::AMurderAtMidnightGameMode()
{
	GameStateClass = AMurderAtMidnightGameState::StaticClass();
	PlayerStateClass = AMurderAtMidnightPlayerState::StaticClass();
	HUDClass = AMurderAtMidnightHUD::StaticClass();
}

void AMurderAtMidnightGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	// The first-person Blueprint was saved before these classes existed, so its defaults still
	// point at Unreal's empty game state. Set them on the live match before that state is spawned.
	GameStateClass = AMurderAtMidnightGameState::StaticClass();
	PlayerStateClass = AMurderAtMidnightPlayerState::StaticClass();

	Super::InitGame(MapName, Options, ErrorMessage);
}

void AMurderAtMidnightGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		StartPhase(EMurderAtMidnightPhase::Arrival);
		SpawnTestKeyAndCabinet();
	}
}

void AMurderAtMidnightGameMode::SpawnTestKeyAndCabinet()
{
	UWorld* World = GetWorld();
	if (!World || !HasAuthority())
	{
		return;
	}

	bool bHasKey = false;
	bool bHasCabinet = false;
	bool bHasDoor = false;
	bool bHasWeapon = false;
	for (TActorIterator<AMurderAtMidnightKey> KeyIt(World); KeyIt; ++KeyIt)
	{
		bHasKey = true;
		break;
	}
	for (TActorIterator<AMurderAtMidnightCabinet> CabinetIt(World); CabinetIt; ++CabinetIt)
	{
		bHasCabinet = true;
		break;
	}
	for (TActorIterator<AMurderAtMidnightDoor> DoorIt(World); DoorIt; ++DoorIt)
	{
		bHasDoor = true;
		break;
	}
	for (TActorIterator<AMurderAtMidnightWeapon> WeaponIt(World); WeaponIt; ++WeaponIt)
	{
		bHasWeapon = true;
		break;
	}

	if (bHasKey && bHasCabinet && bHasDoor && bHasWeapon)
	{
		return;
	}

	FVector Origin = FVector::ZeroVector;
	FRotator Facing = FRotator::ZeroRotator;
	for (TActorIterator<APlayerStart> StartIt(World); StartIt; ++StartIt)
	{
		Origin = StartIt->GetActorLocation();
		Facing = StartIt->GetActorRotation();
		break;
	}

	const FVector Forward = Facing.Vector();
	const FVector Right = FRotationMatrix(Facing).GetUnitAxis(EAxis::Y);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (!bHasKey)
	{
		const FVector KeyLocation = Origin + Forward * 280.0f + FVector(0.0f, 0.0f, 36.0f);
		World->SpawnActor<AMurderAtMidnightKey>(KeyLocation, FRotator::ZeroRotator, Params);
	}

	if (!bHasCabinet)
	{
		const FVector CabinetLocation = Origin + Forward * 280.0f + Right * 180.0f + FVector(0.0f, 0.0f, 70.0f);
		World->SpawnActor<AMurderAtMidnightCabinet>(CabinetLocation, Facing, Params);
	}

	if (!bHasDoor)
	{
		const FVector DoorLocation = Origin + Forward * 280.0f - Right * 180.0f + FVector(0.0f, 0.0f, 100.0f);
		World->SpawnActor<AMurderAtMidnightDoor>(DoorLocation, Facing, Params);
	}

	if (!bHasWeapon)
	{
		const FVector WeaponLocation = Origin + Forward * 160.0f + Right * 80.0f + FVector(0.0f, 0.0f, 40.0f);
		World->SpawnActor<AMurderAtMidnightWeapon>(WeaponLocation, FRotator::ZeroRotator, Params);
	}

	UE_LOG(LogMurderAtMidnight, Log, TEXT("Spawned the test key, cabinet, door, and weapon in front of the player start."));
}

void AMurderAtMidnightGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PhaseTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void AMurderAtMidnightGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (!ErrorMessage.IsEmpty())
	{
		return;
	}

	if (GetNumPlayers() >= MaxPlayers)
	{
		ErrorMessage = TEXT("The manor is full.");
	}
}

void AMurderAtMidnightGameMode::Logout(AController* Exiting)
{
	UWorld* World = GetWorld();
	AMurderAtMidnightGameState* MatchState = GetGameState<AMurderAtMidnightGameState>();
	AMurderAtMidnightPlayerController* ExitingPlayer = Cast<AMurderAtMidnightPlayerController>(Exiting);
	const bool bMatchOpen = HasAuthority()
		&& bRolesAssigned
		&& World
		&& !World->bIsTearingDown
		&& MatchState
		&& MatchState->MatchResult == EMurderAtMidnightMatchResult::None
		&& ExitingPlayer;

	if (bMatchOpen)
	{
		Targets.RemoveAll([ExitingPlayer](const TObjectPtr<AMurderAtMidnightPlayerController>& Target)
		{
			return !Target || Target == ExitingPlayer;
		});
		AssignedAgendas.Remove(ExitingPlayer);

		if (ExitingPlayer == Killer)
		{
			EndMatch(EMurderAtMidnightMatchResult::GuestsWin, TEXT("The killer is gone."));
		}
		else
		{
			CheckAgendaWin();
			ConsiderLastPair(ExitingPlayer);
		}
	}

	Super::Logout(Exiting);
}

void AMurderAtMidnightGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	AssignPublicName(NewPlayer);
	AMurderAtMidnightPlayerController* MatchController = Cast<AMurderAtMidnightPlayerController>(NewPlayer);
	SendPrivateAgenda(MatchController);
	TryAssignRoles();
	if (bRolesAssigned)
	{
		MarkAlive(NewPlayer);
		AssignAgendas();
	}
	SendPrivateRole(MatchController);
}

void AMurderAtMidnightGameMode::AssignPublicName(APlayerController* Controller)
{
	APlayerState* State = Controller ? Controller->GetPlayerState<APlayerState>() : nullptr;
	if (!State || State->GetPlayerName().StartsWith(TEXT("Guest ")))
	{
		return;
	}

	++NextGuestNumber;
	State->SetPlayerName(FString::Printf(TEXT("Guest %d"), NextGuestNumber));
}

void AMurderAtMidnightGameMode::GatherPlayers(TArray<AMurderAtMidnightPlayerController*>& OutPlayers) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (AMurderAtMidnightPlayerController* Controller = Cast<AMurderAtMidnightPlayerController>(It->Get()))
		{
			OutPlayers.Add(Controller);
		}
	}
}

int32 AMurderAtMidnightGameMode::TargetCountForLobby(int32 PlayerCount) const
{
	// Killing every target leaves the killer and two guests who were never targets.
	// Two or three players cannot spare that pair, so a test lobby still has one target.
	const int32 Count = PlayerCount >= 4 ? PlayerCount - 3 : 1;
	return FMath::Clamp(Count, 0, FMath::Max(0, PlayerCount - 1));
}

void AMurderAtMidnightGameMode::TryAssignRoles()
{
	if (!HasAuthority() || bRolesAssigned)
	{
		return;
	}

	TArray<AMurderAtMidnightPlayerController*> Players;
	GatherPlayers(Players);
	if (Players.Num() < 2)
	{
		return;
	}

	for (int32 Index = Players.Num() - 1; Index > 0; --Index)
	{
		const int32 SwapIndex = FMath::RandRange(0, Index);
		Players.Swap(Index, SwapIndex);
	}

	Killer = Players[0];
	Targets.Reset();
	const int32 TargetCount = TargetCountForLobby(Players.Num());
	for (int32 Index = 1; Index <= TargetCount; ++Index)
	{
		Targets.Add(Players[Index]);
	}

	AssignAgendas();
	bRolesAssigned = true;
	UE_LOG(LogMurderAtMidnight, Log, TEXT("Assigned roles for %d players."), Players.Num());

	for (AMurderAtMidnightPlayerController* Controller : Players)
	{
		MarkAlive(Controller);
		SendPrivateRole(Controller);
		SendPrivateAgenda(Controller);
	}
}

void AMurderAtMidnightGameMode::MarkAlive(AController* Controller)
{
	AMurderAtMidnightPlayerState* State = Controller ? Controller->GetPlayerState<AMurderAtMidnightPlayerState>() : nullptr;
	if (State && State->LifeState == EMurderAtMidnightLifeState::InLobby)
	{
		State->SetLifeState(EMurderAtMidnightLifeState::Alive);
	}
}

void AMurderAtMidnightGameMode::SendPrivateRole(AMurderAtMidnightPlayerController* Controller)
{
	if (!HasAuthority() || !Controller || !bRolesAssigned)
	{
		return;
	}

	if (Controller == Killer)
	{
		FString TargetNames;
		for (const TObjectPtr<AMurderAtMidnightPlayerController>& Target : Targets)
		{
			if (!Target)
			{
				continue;
			}

			const AMurderAtMidnightPlayerState* State = Target->GetPlayerState<AMurderAtMidnightPlayerState>();
			if (State && State->LifeState == EMurderAtMidnightLifeState::Dead)
			{
				continue;
			}

			const FString Name = State ? State->GetPlayerName() : TEXT("a guest");
			if (!TargetNames.IsEmpty())
			{
				TargetNames += TEXT(", ");
			}
			TargetNames += Name;
		}

		if (TargetNames.IsEmpty())
		{
			TargetNames = TEXT("none");
		}

		Controller->ClientReceiveRole(true, TargetNames);
	}
	else
	{
		Controller->ClientReceiveRole(false, FString());
	}

	UE_LOG(LogMurderAtMidnight, Log, TEXT("Sent a private role to %s."), *GetNameSafe(Controller));
}

bool AMurderAtMidnightGameMode::TryMurder(AMurderAtMidnightPlayerController* Attacker, AMurderAtMidnightPlayerController* Victim, FString& OutMessage)
{
	if (!HasAuthority() || !bRolesAssigned || !Attacker || !Victim || Attacker == Victim)
	{
		OutMessage = TEXT("The server refused that.");
		return false;
	}

	if (Attacker != Killer)
	{
		OutMessage = TEXT("You cannot do that.");
		return false;
	}

	AMurderAtMidnightPlayerState* AttackerState = Attacker->GetPlayerState<AMurderAtMidnightPlayerState>();
	AMurderAtMidnightPlayerState* VictimState = Victim->GetPlayerState<AMurderAtMidnightPlayerState>();
	if (!AttackerState || AttackerState->LifeState != EMurderAtMidnightLifeState::Alive || !VictimState)
	{
		OutMessage = TEXT("The server refused that.");
		return false;
	}

	if (VictimState->LifeState != EMurderAtMidnightLifeState::Alive)
	{
		OutMessage = TEXT("They are already dead.");
		return false;
	}

	const bool bWasTarget = Targets.RemoveAll([Victim, VictimState](const TObjectPtr<AMurderAtMidnightPlayerController>& Target)
	{
		if (!Target)
		{
			return true;
		}

		const AMurderAtMidnightPlayerState* TargetState = Target->GetPlayerState<AMurderAtMidnightPlayerState>();
		return Target == Victim || TargetState == VictimState;
	}) > 0;
	VictimState->SetLifeState(EMurderAtMidnightLifeState::Dead);
	Victim->SetIgnoreMoveInput(true);
	Victim->ClientInteractMessage(TEXT("You are dead."));

	const FString VictimName = VictimState->GetPlayerName();
	OutMessage = bWasTarget
		? FString::Printf(TEXT("%s is dead. They were a target."), *VictimName)
		: FString::Printf(TEXT("%s is dead. They were not a target."), *VictimName);

	SendPrivateRole(Attacker);
	UE_LOG(LogMurderAtMidnight, Log, TEXT("%s is dead."), *VictimName);
	if (CountLiving() == 1 && IsAlive(Killer))
	{
		const AMurderAtMidnightGameState* MatchState = GetGameState<AMurderAtMidnightGameState>();
		const bool bDuringHunt = MatchState && MatchState->Phase == EMurderAtMidnightPhase::Hunt;
		EndMatch(
			EMurderAtMidnightMatchResult::KillerWins,
			bDuringHunt ? TEXT("The killer caught the last target.") : TEXT("Only the killer is left."));
	}
	else
	{
		ConsiderLastPair();
	}
	return true;
}

TArray<FMurderAtMidnightAgendaRow> AMurderAtMidnightGameMode::CollectAgendaRows() const
{
	TArray<FMurderAtMidnightAgendaRow> Rows;
	if (AgendaTable)
	{
		TArray<FMurderAtMidnightAgendaRow*> Found;
		AgendaTable->GetAllRows(TEXT("Agendas"), Found);
		for (const FMurderAtMidnightAgendaRow* Row : Found)
		{
			if (Row && !Row->AgendaText.IsEmpty())
			{
				Rows.Add(*Row);
			}
		}
	}

	if (Rows.Num() == 0)
	{
		FMurderAtMidnightAgendaRow Cabinet;
		Cabinet.AgendaText = FText::FromString(TEXT("Find the key to unlock the cabinet, then re-lock the cabinet"));
		Cabinet.Completion = EMurderAtMidnightAgendaCheck::CabinetUnlockThenLock;
		Rows.Add(Cabinet);
	}

	const bool bHasDoorAgenda = Rows.ContainsByPredicate([](const FMurderAtMidnightAgendaRow& Row)
	{
		return Row.Completion == EMurderAtMidnightAgendaCheck::DoorUnlockWithKey;
	});
	if (!bHasDoorAgenda)
	{
		FMurderAtMidnightAgendaRow Door;
		Door.AgendaText = FText::FromString(TEXT("Find the key and unlock the door in the level"));
		Door.Completion = EMurderAtMidnightAgendaCheck::DoorUnlockWithKey;
		Rows.Add(Door);
	}

	return Rows;
}

void AMurderAtMidnightGameMode::AssignAgendas()
{
	if (!HasAuthority() || !Killer)
	{
		return;
	}

	AssignedAgendas.Reset();

	TArray<AMurderAtMidnightPlayerController*> Players;
	GatherPlayers(Players);
	TArray<AMurderAtMidnightPlayerController*> Guests;
	for (AMurderAtMidnightPlayerController* Player : Players)
	{
		if (Player && Player != Killer)
		{
			Guests.Add(Player);
		}
	}
	if (Guests.Num() == 0)
	{
		return;
	}

	TArray<FMurderAtMidnightAgendaRow> Rows = CollectAgendaRows();
	int32 DoorIndex = INDEX_NONE;
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		if (DoorIndex == INDEX_NONE && Rows[Index].Completion == EMurderAtMidnightAgendaCheck::DoorUnlockWithKey)
		{
			DoorIndex = Index;
		}
	}

	auto DealRow = [this](AMurderAtMidnightPlayerController* Guest, const FMurderAtMidnightAgendaRow& Row)
	{
		FMurderAtMidnightAssignedAgenda Assigned;
		Assigned.Text = Row.AgendaText.ToString();
		Assigned.Completion = Row.Completion;
		AssignedAgendas.Add(Guest, Assigned);
	};

	if (DoorIndex != INDEX_NONE)
	{
		const int32 GuestIndex = FMath::RandRange(0, Guests.Num() - 1);
		DealRow(Guests[GuestIndex], Rows[DoorIndex]);
		Guests.RemoveAt(GuestIndex);
		Rows.RemoveAt(DoorIndex);
	}

	for (int32 Index = Rows.Num() - 1; Index > 0; --Index)
	{
		const int32 SwapIndex = FMath::RandRange(0, Index);
		Rows.Swap(Index, SwapIndex);
	}

	for (int32 Index = 0; Index < Guests.Num() && Index < Rows.Num(); ++Index)
	{
		DealRow(Guests[Index], Rows[Index]);
	}

	for (AMurderAtMidnightPlayerController* Player : Players)
	{
		SendPrivateAgenda(Player);
	}
}

void AMurderAtMidnightGameMode::SendPrivateAgenda(AMurderAtMidnightPlayerController* Controller)
{
	if (!HasAuthority() || !Controller)
	{
		return;
	}

	if (const FMurderAtMidnightAssignedAgenda* Assigned = AssignedAgendas.Find(Controller))
	{
		Controller->ClientReceiveAgendaState(Assigned->Text, Assigned->bComplete);
		UE_LOG(LogMurderAtMidnight, Log, TEXT("Sent a private agenda to %s."), *GetNameSafe(Controller));
		return;
	}

	Controller->ClientReceiveAgendaState(FString(), false);
}

void AMurderAtMidnightGameMode::NotifyCabinetLockChanged(AMurderAtMidnightPlayerController* Who, bool bIsOpen)
{
	if (!HasAuthority() || !Who)
	{
		return;
	}

	FMurderAtMidnightAssignedAgenda* Progress = AssignedAgendas.Find(Who);
	if (!Progress || Progress->Completion != EMurderAtMidnightAgendaCheck::CabinetUnlockThenLock)
	{
		return;
	}
	if (Progress->bComplete)
	{
		return;
	}

	if (bIsOpen)
	{
		Progress->bUnlockedCabinet = true;
		return;
	}

	if (Progress->bUnlockedCabinet)
	{
		Progress->bComplete = true;
		SendPrivateAgenda(Who);
		UE_LOG(LogMurderAtMidnight, Log, TEXT("A private agenda was completed."));
		CheckAgendaWin();
	}
}

void AMurderAtMidnightGameMode::NotifyDoorOpened(AMurderAtMidnightPlayerController* Who)
{
	if (!HasAuthority() || !Who)
	{
		return;
	}

	FMurderAtMidnightAssignedAgenda* Progress = AssignedAgendas.Find(Who);
	if (!Progress || Progress->bComplete || Progress->Completion != EMurderAtMidnightAgendaCheck::DoorUnlockWithKey)
	{
		return;
	}

	Progress->bComplete = true;
	SendPrivateAgenda(Who);
	UE_LOG(LogMurderAtMidnight, Log, TEXT("A private agenda was completed."));
	CheckAgendaWin();
}

bool AMurderAtMidnightGameMode::IsAlive(const AMurderAtMidnightPlayerController* Controller) const
{
	const AMurderAtMidnightPlayerState* State = Controller ? Controller->GetPlayerState<AMurderAtMidnightPlayerState>() : nullptr;
	return State && State->LifeState == EMurderAtMidnightLifeState::Alive;
}

int32 AMurderAtMidnightGameMode::CountLiving() const
{
	TArray<AMurderAtMidnightPlayerController*> Players;
	GatherPlayers(Players);
	int32 Living = 0;
	for (const AMurderAtMidnightPlayerController* Player : Players)
	{
		if (IsAlive(Player))
		{
			Living++;
		}
	}
	return Living;
}

bool AMurderAtMidnightGameMode::AllAgendasComplete() const
{
	if (AssignedAgendas.Num() == 0)
	{
		return false;
	}

	for (const TPair<TObjectPtr<AMurderAtMidnightPlayerController>, FMurderAtMidnightAssignedAgenda>& Pair : AssignedAgendas)
	{
		if (!Pair.Value.bComplete)
		{
			return false;
		}
	}
	return true;
}

bool AMurderAtMidnightGameMode::HasLivingTarget() const
{
	for (const TObjectPtr<AMurderAtMidnightPlayerController>& Target : Targets)
	{
		if (IsAlive(Target))
		{
			return true;
		}
	}
	return false;
}

bool AMurderAtMidnightGameMode::IsGathering(EMurderAtMidnightPhase Phase) const
{
	return Phase == EMurderAtMidnightPhase::Arrival
		|| Phase == EMurderAtMidnightPhase::Dinner
		|| Phase == EMurderAtMidnightPhase::Garden;
}

void AMurderAtMidnightGameMode::TellPlayers(const FString& Message)
{
	TArray<AMurderAtMidnightPlayerController*> Players;
	GatherPlayers(Players);
	for (AMurderAtMidnightPlayerController* Player : Players)
	{
		if (Player)
		{
			Player->ClientInteractMessage(Message);
		}
	}
}

void AMurderAtMidnightGameMode::EndMatch(EMurderAtMidnightMatchResult Result, const FString& Announcement)
{
	AMurderAtMidnightGameState* MatchState = GetGameState<AMurderAtMidnightGameState>();
	UWorld* World = GetWorld();
	if (!HasAuthority() || !MatchState || MatchState->MatchResult != EMurderAtMidnightMatchResult::None)
	{
		return;
	}

	if (World)
	{
		World->GetTimerManager().ClearTimer(PhaseTimer);
	}

	Ballots.Empty();
	bMidnightVote = false;
	MatchState->FrozenClockMinute = MatchState->GetClockMinute();
	MatchState->MatchResult = Result;
	MatchState->bVoteOpen = false;
	MatchState->bVoteLocked = false;
	MatchState->Phase = EMurderAtMidnightPhase::Ended;
	MatchState->PhaseEndsAtServerTime = 0.0;
	MatchState->ForceNetUpdate();
	TellPlayers(Announcement);
	UE_LOG(LogMurderAtMidnight, Log, TEXT("Match ended: %s"), *Announcement);
}

void AMurderAtMidnightGameMode::CheckAgendaWin()
{
	if (AllAgendasComplete())
	{
		EndMatch(EMurderAtMidnightMatchResult::GuestsWin, TEXT("Every agenda is finished."));
	}
}

void AMurderAtMidnightGameMode::OpenVote(bool bAtMidnight)
{
	AMurderAtMidnightGameState* MatchState = GetGameState<AMurderAtMidnightGameState>();
	if (!MatchState || MatchState->MatchResult != EMurderAtMidnightMatchResult::None)
	{
		return;
	}

	Ballots.Empty();
	bMidnightVote = bAtMidnight;
	MatchState->bVoteOpen = true;
	MatchState->bVoteLocked = false;
	MatchState->ForceNetUpdate();
	TArray<AMurderAtMidnightPlayerController*> Players;
	GatherPlayers(Players);
	for (AMurderAtMidnightPlayerController* Player : Players)
	{
		if (Player)
		{
			Player->ClientReceiveVoteChoice(FString());
		}
	}

	TellPlayers(TEXT("A vote is open. Look at a living player and click."));
}

void AMurderAtMidnightGameMode::ResolveVoteIfReady()
{
	AMurderAtMidnightGameState* MatchState = GetGameState<AMurderAtMidnightGameState>();
	if (!MatchState || !MatchState->bVoteOpen)
	{
		return;
	}

	TArray<AMurderAtMidnightPlayerController*> Players;
	GatherPlayers(Players);
	TArray<AMurderAtMidnightPlayerController*> Living;
	for (AMurderAtMidnightPlayerController* Player : Players)
	{
		if (IsAlive(Player))
		{
			Living.Add(Player);
		}
	}

	if (Living.Num() == 0)
	{
		return;
	}

	for (AMurderAtMidnightPlayerController* Player : Living)
	{
		if (!Ballots.Contains(Player))
		{
			return;
		}
	}

	int32 VotesForKiller = 0;
	for (AMurderAtMidnightPlayerController* Player : Living)
	{
		const TObjectPtr<AMurderAtMidnightPlayerController>* Choice = Ballots.Find(Player);
		if (Choice && *Choice == Killer)
		{
			VotesForKiller++;
		}
	}

	const bool bMajority = VotesForKiller * 2 > Living.Num();
	Ballots.Empty();
	MatchState->bVoteOpen = false;

	if (bMajority)
	{
		EndMatch(EMurderAtMidnightMatchResult::GuestsWin, TEXT("The vote named the killer."));
		return;
	}

	if (bMidnightVote)
	{
		EndMatch(EMurderAtMidnightMatchResult::KillerWins, TEXT("The killer wins."));
		return;
	}

	bMidnightVote = false;
	MatchState->bVoteLocked = true;
	MatchState->ForceNetUpdate();
	TellPlayers(TEXT("The vote changed nothing."));
}

bool AMurderAtMidnightGameMode::TryCallVote(AMurderAtMidnightPlayerController* Who, FString& OutMessage)
{
	const AMurderAtMidnightGameState* MatchState = GetGameState<AMurderAtMidnightGameState>();
	if (!HasAuthority() || !Who || !MatchState || MatchState->MatchResult != EMurderAtMidnightMatchResult::None)
	{
		OutMessage = TEXT("The server refused that.");
		return false;
	}

	if (!IsAlive(Who))
	{
		OutMessage = TEXT("You cannot vote.");
		return false;
	}

	if (MatchState->bVoteOpen)
	{
		OutMessage = TEXT("A vote is already open.");
		return false;
	}

	if (MatchState->Phase == EMurderAtMidnightPhase::Hunt)
	{
		OutMessage = TEXT("Voting is closed.");
		return false;
	}

	if (MatchState->Phase == EMurderAtMidnightPhase::Resolution)
	{
		OutMessage = TEXT("The midnight vote cannot be called.");
		return false;
	}

	if (!IsGathering(MatchState->Phase))
	{
		OutMessage = TEXT("A vote waits for a gathering.");
		return false;
	}

	if (MatchState->bVoteLocked)
	{
		OutMessage = TEXT("Voting is locked until the next gathering.");
		return false;
	}

	if (CountLiving() < 2)
	{
		OutMessage = TEXT("Not enough living players to vote.");
		return false;
	}

	OpenVote(false);
	OutMessage = TEXT("You called a vote.");
	return true;
}

bool AMurderAtMidnightGameMode::TryCastVote(AMurderAtMidnightPlayerController* Who, AMurderAtMidnightPlayerController* Accused, FString& OutMessage)
{
	const AMurderAtMidnightGameState* MatchState = GetGameState<AMurderAtMidnightGameState>();
	if (!HasAuthority() || !Who || !Accused || !MatchState || !MatchState->bVoteOpen)
	{
		OutMessage = TEXT("No vote is open.");
		return false;
	}

	if (!IsAlive(Who) || !IsAlive(Accused))
	{
		OutMessage = TEXT("Look at a living player.");
		return false;
	}

	Ballots.Add(Who, Accused);
	const AMurderAtMidnightPlayerState* AccusedState = Accused->GetPlayerState<AMurderAtMidnightPlayerState>();
	const FString AccusedName = AccusedState ? AccusedState->GetPlayerName() : TEXT("someone");
	const FString ChoiceLine = FString::Printf(TEXT("You voted for: %s"), *AccusedName);
	Who->ClientReceiveVoteChoice(ChoiceLine);
	OutMessage = ChoiceLine;
	ResolveVoteIfReady();
	return true;
}

void AMurderAtMidnightGameMode::CheckEveningResult()
{
	const AMurderAtMidnightGameState* MatchState = GetGameState<AMurderAtMidnightGameState>();
	if (!HasAuthority() || !bRolesAssigned || !MatchState || MatchState->MatchResult != EMurderAtMidnightMatchResult::None)
	{
		return;
	}

	if (AllAgendasComplete())
	{
		EndMatch(EMurderAtMidnightMatchResult::GuestsWin, TEXT("Every agenda is finished."));
		return;
	}

	if (HasLivingTarget())
	{
		EndMatch(EMurderAtMidnightMatchResult::GuestsWin, TEXT("A target is still alive at midnight."));
		return;
	}

	if (CountLiving() >= 3)
	{
		OpenVote(true);
	}
}

void AMurderAtMidnightGameMode::StartPhase(EMurderAtMidnightPhase NewPhase)
{
	AMurderAtMidnightGameState* MatchState = GetGameState<AMurderAtMidnightGameState>();
	UWorld* World = GetWorld();
	if (!MatchState || !World)
	{
		return;
	}

	Ballots.Empty();
	bMidnightVote = false;
	MatchState->bVoteOpen = false;
	MatchState->bVoteLocked = false;

	MatchState->Phase = NewPhase;
	MatchState->FrozenClockMinute = -1;
	const float Duration = GetPhaseDuration(NewPhase);
	const double Now = MatchState->GetServerWorldTimeSeconds();
	MatchState->PhaseStartedAtServerTime = Now;
	MatchState->PhaseDurationSeconds = Duration;
	MatchState->PhaseEndsAtServerTime = Duration > 0.0f ? Now + Duration : 0.0;
	MatchState->ForceNetUpdate();
	UE_LOG(LogMurderAtMidnight, Log, TEXT("Phase is now %d for %.0f seconds."), static_cast<int32>(NewPhase), Duration);

	World->GetTimerManager().ClearTimer(PhaseTimer);
	if (NewPhase == EMurderAtMidnightPhase::Hunt && Duration > 0.0f)
	{
		World->GetTimerManager().SetTimer(PhaseTimer, this, &AMurderAtMidnightGameMode::OnHuntTimer, Duration, false);
	}
	else if (Duration > 0.0f)
	{
		World->GetTimerManager().SetTimer(PhaseTimer, this, &AMurderAtMidnightGameMode::AdvancePhase, Duration, false);
	}

	if (NewPhase == EMurderAtMidnightPhase::Resolution)
	{
		CheckEveningResult();
	}
}

void AMurderAtMidnightGameMode::OnHuntTimer()
{
	EndMatch(EMurderAtMidnightMatchResult::GuestsWin, TEXT("The last target survived."));
}

void AMurderAtMidnightGameMode::StartHunt()
{
	AMurderAtMidnightGameState* MatchState = GetGameState<AMurderAtMidnightGameState>();
	if (!HasAuthority() || !MatchState || MatchState->MatchResult != EMurderAtMidnightMatchResult::None)
	{
		return;
	}

	if (MatchState->Phase == EMurderAtMidnightPhase::Hunt)
	{
		return;
	}

	const int32 EveningMinute = MatchState->GetClockMinute();
	StartPhase(EMurderAtMidnightPhase::Hunt);
	MatchState->FrozenClockMinute = EveningMinute;
	MatchState->ForceNetUpdate();

	TArray<AMurderAtMidnightPlayerController*> Players;
	GatherPlayers(Players);
	for (AMurderAtMidnightPlayerController* Player : Players)
	{
		if (Player)
		{
			Player->ClientReceiveVoteChoice(FString());
		}
	}

	TellPlayers(TEXT("Only two living players remain."));
}

void AMurderAtMidnightGameMode::ConsiderLastPair(const AMurderAtMidnightPlayerController* AlreadyGone)
{
	const AMurderAtMidnightGameState* MatchState = GetGameState<AMurderAtMidnightGameState>();
	if (!HasAuthority() || !bRolesAssigned || !MatchState || MatchState->MatchResult != EMurderAtMidnightMatchResult::None)
	{
		return;
	}

	if (MatchState->Phase == EMurderAtMidnightPhase::Hunt)
	{
		return;
	}

	TArray<AMurderAtMidnightPlayerController*> Players;
	GatherPlayers(Players);
	TArray<AMurderAtMidnightPlayerController*> Living;
	for (AMurderAtMidnightPlayerController* Player : Players)
	{
		if (Player != AlreadyGone && IsAlive(Player))
		{
			Living.Add(Player);
		}
	}

	if (Living.Num() != 2 || !Living.Contains(Killer))
	{
		return;
	}

	AMurderAtMidnightPlayerController* Guest = Living[0] == Killer ? Living[1] : Living[0];
	if (IsLivingTarget(Guest))
	{
		StartHunt();
		return;
	}

	EndMatch(EMurderAtMidnightMatchResult::KillerWins, TEXT("The last guest was never a target."));
}

bool AMurderAtMidnightGameMode::IsLivingTarget(const AMurderAtMidnightPlayerController* Controller) const
{
	return IsAlive(Controller) && Targets.Contains(Controller);
}

void AMurderAtMidnightGameMode::SkipToNextPhase()
{
	if (!HasAuthority())
	{
		return;
	}

	const AMurderAtMidnightGameState* MatchState = GetGameState<AMurderAtMidnightGameState>();
	if (!MatchState || MatchState->MatchResult != EMurderAtMidnightMatchResult::None)
	{
		return;
	}

	AdvancePhase();
}

void AMurderAtMidnightGameMode::AdvancePhase()
{
	const AMurderAtMidnightGameState* MatchState = GetGameState<AMurderAtMidnightGameState>();
	if (!MatchState)
	{
		return;
	}

	switch (MatchState->Phase)
	{
	case EMurderAtMidnightPhase::Arrival:
		StartPhase(EMurderAtMidnightPhase::Dinner);
		break;
	case EMurderAtMidnightPhase::Dinner:
		StartPhase(EMurderAtMidnightPhase::Garden);
		break;
	case EMurderAtMidnightPhase::Garden:
		StartPhase(EMurderAtMidnightPhase::Resolution);
		break;
	default:
		break;
	}
}

float AMurderAtMidnightGameMode::GetPhaseDuration(EMurderAtMidnightPhase Phase) const
{
	switch (Phase)
	{
	case EMurderAtMidnightPhase::Arrival: return ArrivalDurationSeconds;
	case EMurderAtMidnightPhase::Dinner: return DinnerDurationSeconds;
	case EMurderAtMidnightPhase::Garden: return GardenDurationSeconds;
	case EMurderAtMidnightPhase::Hunt: return HuntDurationSeconds;
	default: return 0.0f;
	}
}
