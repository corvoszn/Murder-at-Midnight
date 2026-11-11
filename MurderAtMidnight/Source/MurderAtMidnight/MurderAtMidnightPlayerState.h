// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MurderAtMidnightPlayerState.generated.h"

/** Public life state. Every client receives this. The role and the agenda do not belong here. */
UENUM(BlueprintType)
enum class EMurderAtMidnightLifeState : uint8
{
	InLobby UMETA(DisplayName = "In Lobby"),
	Alive UMETA(DisplayName = "Alive"),
	Dead UMETA(DisplayName = "Dead"),
	Rat UMETA(DisplayName = "Rat")
};

/**
 * Public facts about one player. Replicated to every client, and kept when the body changes.
 * The display name is the built-in player name. The current body is the possessed pawn.
 * This class does not store the killer id, the target list, or an agenda.
 */
UCLASS()
class MURDERATMIDNIGHT_API AMurderAtMidnightPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AMurderAtMidnightPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Whether this player is in the lobby, alive, dead, or a rat. Replicated to every client. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	EMurderAtMidnightLifeState LifeState = EMurderAtMidnightLifeState::InLobby;
};
