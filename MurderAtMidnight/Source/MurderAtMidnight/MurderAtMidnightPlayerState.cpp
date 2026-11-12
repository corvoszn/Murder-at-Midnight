// Copyright Epic Games, Inc. All Rights Reserved.

#include "MurderAtMidnightPlayerState.h"
#include "MurderAtMidnightCharacter.h"
#include "Net/UnrealNetwork.h"

AMurderAtMidnightPlayerState::AMurderAtMidnightPlayerState()
{
}

void AMurderAtMidnightPlayerState::SetLifeState(EMurderAtMidnightLifeState NewState)
{
	if (!HasAuthority())
	{
		return;
	}

	LifeState = NewState;
	OnRep_LifeState();
}

void AMurderAtMidnightPlayerState::OnRep_LifeState()
{
	if (LifeState != EMurderAtMidnightLifeState::Dead)
	{
		return;
	}

	if (AMurderAtMidnightCharacter* Body = Cast<AMurderAtMidnightCharacter>(GetPawn()))
	{
		Body->ApplyDeadBody();
	}
}

void AMurderAtMidnightPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Public fact. Every client receives this.
	DOREPLIFETIME(AMurderAtMidnightPlayerState, LifeState);
}
