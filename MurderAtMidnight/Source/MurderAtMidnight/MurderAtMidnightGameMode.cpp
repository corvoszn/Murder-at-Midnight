// Copyright Epic Games, Inc. All Rights Reserved.

#include "MurderAtMidnightGameMode.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
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
	SendPrivateAgenda(Cast<AMurderAtMidnightPlayerController>(NewPlayer));
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

	EnsureAgendaTexts();
	if (AgendaTexts.Num() == 0)
	{
		return;
	}

	if (!PrivateAgendas.Contains(Controller))
	{
		const int32 Index = PrivateAgendas.Num() % AgendaTexts.Num();
		PrivateAgendas.Add(Controller, AgendaTexts[Index]);
	}

	Controller->ClientReceiveAgenda(PrivateAgendas[Controller]);
	UE_LOG(LogMurderAtMidnight, Log, TEXT("Sent a private agenda to %s."), *GetNameSafe(Controller));
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
