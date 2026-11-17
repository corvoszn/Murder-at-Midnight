// Copyright Epic Games, Inc. All Rights Reserved.

#include "MurderAtMidnightPhaseWidget.h"
#include "Components/TextBlock.h"
#include "EngineUtils.h"
#include "MurderAtMidnightCabinet.h"
#include "MurderAtMidnightDoor.h"
#include "MurderAtMidnightWeapon.h"
#include "MurderAtMidnightCharacter.h"
#include "MurderAtMidnightGameState.h"
#include "MurderAtMidnightInteractableComponent.h"
#include "MurderAtMidnightInventoryComponent.h"
#include "MurderAtMidnightPlayerController.h"
#include "MurderAtMidnightPlayerState.h"
#include "TimerManager.h"

namespace
{
	void SetShown(UTextBlock* Text, bool bShown)
	{
		if (Text)
		{
			Text->SetVisibility(bShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

void UMurderAtMidnightPhaseWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	World->GetTimerManager().SetTimer(RefreshTimer, this, &UMurderAtMidnightPhaseWidget::RefreshText, 0.1f, true);
	RefreshText();
}

void UMurderAtMidnightPhaseWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimer);
	}

	Super::NativeDestruct();
}

void UMurderAtMidnightPhaseWidget::RefreshText()
{
	UWorld* World = GetWorld();
	const AMurderAtMidnightGameState* MatchState = World ? World->GetGameState<AMurderAtMidnightGameState>() : nullptr;
	const APlayerController* Controller = GetOwningPlayer();
	const AMurderAtMidnightPlayerController* MatchController = Cast<AMurderAtMidnightPlayerController>(Controller);
	const AMurderAtMidnightCharacter* Character = Controller ? Cast<AMurderAtMidnightCharacter>(Controller->GetPawn()) : nullptr;
	const UMurderAtMidnightInventoryComponent* Inventory = Character ? Character->GetInventory() : nullptr;
	const FString Carrying = Inventory ? Inventory->GetCarriedLabel() : TEXT("nothing");

	const EMurderAtMidnightPhase Phase = MatchState ? MatchState->Phase : EMurderAtMidnightPhase::WaitingForPlayers;
	SetShown(WaitingText, Phase == EMurderAtMidnightPhase::WaitingForPlayers);
	SetShown(ArrivalText, Phase == EMurderAtMidnightPhase::Arrival);
	SetShown(DinnerText, Phase == EMurderAtMidnightPhase::Dinner);
	SetShown(GardenText, Phase == EMurderAtMidnightPhase::Garden);
	SetShown(ResolutionText, Phase == EMurderAtMidnightPhase::Resolution);
	SetShown(HuntText, false);
	SetShown(EndedText, Phase == EMurderAtMidnightPhase::Ended);

	if (ClockText && MatchState)
	{
		ClockText->SetText(FText::FromString(MatchState->GetClockLabel()));
		SetShown(ClockText, Phase != EMurderAtMidnightPhase::WaitingForPlayers);
	}

	SetShown(CarryingNothingText, Carrying == TEXT("nothing"));
	SetShown(CarryingKeyText, Carrying == TEXT("a key"));
	SetShown(CarryingWeaponText, Carrying == TEXT("a weapon"));

	bool bCabinetOpen = false;
	bool bDoorOpen = false;
	bool bHoldingWeapon = false;
	bool bWeaponBloody = false;
	if (World)
	{
		for (TActorIterator<AMurderAtMidnightCabinet> It(World); It; ++It)
		{
			if (const UMurderAtMidnightInteractableComponent* Interactable = It->GetInteractable())
			{
				bCabinetOpen = Interactable->IsOpen();
				break;
			}
		}
		for (TActorIterator<AMurderAtMidnightDoor> It(World); It; ++It)
		{
			if (const UMurderAtMidnightInteractableComponent* Interactable = It->GetInteractable())
			{
				bDoorOpen = Interactable->IsOpen();
				break;
			}
		}
		for (TActorIterator<AMurderAtMidnightWeapon> It(World); It; ++It)
		{
			if (const UMurderAtMidnightInteractableComponent* Interactable = It->GetInteractable())
			{
				bHoldingWeapon = Character && Interactable->GetCarrier() == Character;
				bWeaponBloody = Interactable->IsBloody();
				break;
			}
		}
	}

	SetShown(CabinetOpenText, bCabinetOpen);
	SetShown(CabinetClosedText, !bCabinetOpen);
	SetShown(DoorOpenText, bDoorOpen);
	SetShown(DoorClosedText, !bDoorOpen);
	SetShown(WeaponCleanText, bHoldingWeapon && !bWeaponBloody);
	SetShown(WeaponBloodyText, bHoldingWeapon && bWeaponBloody);

	const bool bRoleReceived = MatchController && MatchController->HasRole();
	const bool bKiller = bRoleReceived && MatchController->IsKiller();
	SetShown(KillerText, bKiller);
	SetShown(GuestText, bRoleReceived && !bKiller);
	SetShown(TargetText, bKiller);
	if (TargetText && bKiller)
	{
		TargetText->SetText(FText::FromString(MatchController->GetTargetNames()));
	}

	const AMurderAtMidnightPlayerState* State = MatchController ? MatchController->GetPlayerState<AMurderAtMidnightPlayerState>() : nullptr;
	SetShown(DeadText, State && State->LifeState == EMurderAtMidnightLifeState::Dead);

	const FString Message = MatchController ? MatchController->GetLastInteractMessage() : FString();
	SetShown(MessageText, !Message.IsEmpty());
	if (MessageText && !Message.IsEmpty())
	{
		MessageText->SetText(FText::FromString(Message));
	}

	const FString Agenda = MatchController ? MatchController->GetPrivateAgenda() : FString();
	SetShown(AgendaText, !Agenda.IsEmpty());
	if (AgendaText && !Agenda.IsEmpty())
	{
		AgendaText->SetText(FText::FromString(Agenda));
	}
	SetShown(CompletedText, MatchController && MatchController->IsAgendaComplete());

	const bool bVoteOpen = MatchState && MatchState->bVoteOpen;
	SetShown(VoteOpenText, bVoteOpen);
	SetShown(VoteLockedText, MatchState && MatchState->bVoteLocked && !bVoteOpen);
	SetShown(GuestsWinText, MatchState && MatchState->MatchResult == EMurderAtMidnightMatchResult::GuestsWin);
	SetShown(KillerWinText, MatchState && MatchState->MatchResult == EMurderAtMidnightMatchResult::KillerWins);

	const FString VoteChoice = MatchController ? MatchController->GetVoteChoice() : FString();
	SetShown(VoteChoiceText, !VoteChoice.IsEmpty());
	if (VoteChoiceText && !VoteChoice.IsEmpty())
	{
		VoteChoiceText->SetText(FText::FromString(VoteChoice));
	}
}
