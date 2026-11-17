// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MurderAtMidnightPhaseWidget.generated.h"

class UTextBlock;

/** Shows and hides text blocks supplied by a Widget Blueprint. It does not write their sentences. */
UCLASS()
class MURDERATMIDNIGHT_API UMurderAtMidnightPhaseWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WaitingText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ArrivalText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DinnerText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GardenText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ResolutionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HuntText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EndedText;

	/** The evening clock, written by code as 18:00 through 00:00. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ClockText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CarryingNothingText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CarryingKeyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CarryingWeaponText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CabinetOpenText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CabinetClosedText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DoorOpenText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DoorClosedText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeaponCleanText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeaponBloodyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KillerText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GuestText;

	/** The changing target names. Shown to the killer only. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TargetText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DeadText;

	/** The latest server reply. The words change with the action. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AgendaText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CompletedText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VoteOpenText;

	/** This player's own choice. The words are written by code. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VoteChoiceText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VoteLockedText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GuestsWinText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KillerWinText;

	FTimerHandle RefreshTimer;

	void RefreshText();
};
