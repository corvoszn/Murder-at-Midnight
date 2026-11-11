// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MurderAtMidnightPhaseWidget.generated.h"

class UTextBlock;

/** On-screen phase and countdown. It reads the public game state. It does not decide the match. */
UCLASS()
class MURDERATMIDNIGHT_API UMurderAtMidnightPhaseWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY()
	TObjectPtr<UTextBlock> PhaseText;

	FTimerHandle RefreshTimer;

	void BuildLayout();
	void RefreshText();
};
