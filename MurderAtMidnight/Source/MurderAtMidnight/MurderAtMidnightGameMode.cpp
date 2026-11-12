// Copyright Epic Games, Inc. All Rights Reserved.

#include "MurderAtMidnightGameMode.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "MurderAtMidnight.h"
#include "MurderAtMidnightCabinet.h"
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
	EnsureAgendaTexts();
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

	if (bHasKey && bHasCabinet)
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

	UE_LOG(LogMurderAtMidnight, Log, TEXT("Spawned the test key and cabinet in front of the player start."));
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
	int32 Count = 1;
	if (PlayerCount >= 7)
	{
		Count = 3;
	}
	else if (PlayerCount >= 5)
	{
		Count = 2;
	}

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

	bRolesAssigned = true;
	UE_LOG(LogMurderAtMidnight, Log, TEXT("Assigned roles for %d players."), Players.Num());

	for (AMurderAtMidnightPlayerController* Controller : Players)
	{
		MarkAlive(Controller);
		SendPrivateRole(Controller);
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
		PrivateAgendas.Remove(Controller);
		Controller->ClientReceiveAgenda(FString());

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

		Controller->ClientReceiveRole(FString::Printf(TEXT("You are the killer.\nYour targets: %s"), *TargetNames));
	}
	else
	{
		Controller->ClientReceiveRole(TEXT("You are a guest."));
	}

	UE_LOG(LogMurderAtMidnight, Log, TEXT("Sent a private role to %s."), *GetNameSafe(Controller));
}

void AMurderAtMidnightGameMode::TryMurder(AMurderAtMidnightPlayerController* Attacker, AMurderAtMidnightPlayerController* Victim, FString& OutMessage)
{
	if (!HasAuthority() || !bRolesAssigned || !Attacker || !Victim || Attacker == Victim)
	{
		OutMessage = TEXT("The server refused that.");
		return;
	}

	if (Attacker != Killer)
	{
		OutMessage = TEXT("You cannot do that.");
		return;
	}

	AMurderAtMidnightPlayerState* AttackerState = Attacker->GetPlayerState<AMurderAtMidnightPlayerState>();
	AMurderAtMidnightPlayerState* VictimState = Victim->GetPlayerState<AMurderAtMidnightPlayerState>();
	if (!AttackerState || AttackerState->LifeState != EMurderAtMidnightLifeState::Alive || !VictimState)
	{
		OutMessage = TEXT("The server refused that.");
		return;
	}

	if (VictimState->LifeState != EMurderAtMidnightLifeState::Alive)
	{
		OutMessage = TEXT("They are already dead.");
		return;
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
}

void AMurderAtMidnightGameMode::EnsureAgendaTexts()
{
	if (AgendaTexts.Num() > 0)
	{
		return;
	}

	AgendaTexts = {
		TEXT("Open the cabinet in the hall."),
		TEXT("Leave the key where another guest will find it."),
		TEXT("Find out who was in the garden before dinner."),
		TEXT("Put the portrait in the hall back straight."),
		TEXT("Return the red book to the library shelf."),
		TEXT("Learn where the host keeps the wine."),
		TEXT("Hide the spare glasses in the pantry."),
		TEXT("Check whether the cellar door is locked.")
	};
}

void AMurderAtMidnightGameMode::SendPrivateAgenda(AMurderAtMidnightPlayerController* Controller)
{
	if (!HasAuthority() || !Controller)
	{
		return;
	}

	if (bRolesAssigned && Controller == Killer)
	{
		PrivateAgendas.Remove(Controller);
		AgendaProgress.Remove(Controller);
		Controller->ClientReceiveAgenda(FString());
		return;
	}

	if (!AgendaProgress.Contains(Controller))
	{
		AgendaProgress.Add(Controller, FMurderAtMidnightAgendaProgress());
	}

	const FString Line = BuildAgendaLine(Controller);
	PrivateAgendas.Add(Controller, Line);
	Controller->ClientReceiveAgenda(Line);
	UE_LOG(LogMurderAtMidnight, Log, TEXT("Sent a private agenda to %s."), *GetNameSafe(Controller));
}

FString AMurderAtMidnightGameMode::BuildAgendaLine(const AMurderAtMidnightPlayerController* Controller) const
{
	FString Line = TEXT("Find the key to unlock the cabinet, then re-lock the cabinet");
	if (const FMurderAtMidnightAgendaProgress* Progress = AgendaProgress.Find(Controller))
	{
		if (Progress->bComplete)
		{
			Line += TEXT(" - Completed");
		}
	}

	return Line;
}

void AMurderAtMidnightGameMode::NotifyCabinetLockChanged(AMurderAtMidnightPlayerController* Who, bool bIsOpen)
{
	if (!HasAuthority() || !Who || (bRolesAssigned && Who == Killer))
	{
		return;
	}

	FMurderAtMidnightAgendaProgress& Progress = AgendaProgress.FindOrAdd(Who);
	if (Progress.bComplete)
	{
		return;
	}

	if (bIsOpen)
	{
		Progress.bUnlockedCabinet = true;
		return;
	}

	if (Progress.bUnlockedCabinet)
	{
		Progress.bComplete = true;
		SendPrivateAgenda(Who);
		UE_LOG(LogMurderAtMidnight, Log, TEXT("A private agenda was completed."));
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

	MatchState->Phase = NewPhase;
	const float Duration = GetPhaseDuration(NewPhase);
	MatchState->PhaseEndsAtServerTime = Duration > 0.0f
		? MatchState->GetServerWorldTimeSeconds() + Duration
		: 0.0;
	MatchState->ForceNetUpdate();
	UE_LOG(LogMurderAtMidnight, Log, TEXT("Phase is now %d for %.0f seconds."), static_cast<int32>(NewPhase), Duration);

	if (Duration <= 0.0f)
	{
		return;
	}

	World->GetTimerManager().SetTimer(PhaseTimer, this, &AMurderAtMidnightGameMode::AdvancePhase, Duration, false);
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
	case EMurderAtMidnightPhase::Resolution:
		StartPhase(EMurderAtMidnightPhase::Ended);
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
	case EMurderAtMidnightPhase::Resolution: return ResolutionDurationSeconds;
	default: return 0.0f;
	}
}
