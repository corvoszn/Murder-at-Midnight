// Copyright Epic Games, Inc. All Rights Reserved.

#include "MurderAtMidnightPhaseWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "EngineUtils.h"
#include "MurderAtMidnightCabinet.h"
#include "MurderAtMidnightCharacter.h"
#include "MurderAtMidnightGameState.h"
#include "MurderAtMidnightInteractableComponent.h"
#include "MurderAtMidnightInventoryComponent.h"
#include "MurderAtMidnightPlayerController.h"
#include "MurderAtMidnightPlayerState.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"

namespace
{
	FString PhaseLabel(EMurderAtMidnightPhase Phase)
	{
		switch (Phase)
		{
		case EMurderAtMidnightPhase::WaitingForPlayers: return TEXT("Waiting for players");
		case EMurderAtMidnightPhase::Arrival: return TEXT("Arrival");
		case EMurderAtMidnightPhase::Dinner: return TEXT("Dinner");
		case EMurderAtMidnightPhase::Garden: return TEXT("Garden");
		case EMurderAtMidnightPhase::Resolution: return TEXT("Resolution");
		case EMurderAtMidnightPhase::Hunt: return TEXT("Hunt");
		case EMurderAtMidnightPhase::Ended: return TEXT("Ended");
		default: return TEXT("Unknown");
		}
	}
}

TSharedRef<SWidget> UMurderAtMidnightPhaseWidget::RebuildWidget()
{
	BuildLayout();
	return Super::RebuildWidget();
}

void UMurderAtMidnightPhaseWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildLayout();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RefreshTimer, this, &UMurderAtMidnightPhaseWidget::RefreshText, 0.1f, true);
	}

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

void UMurderAtMidnightPhaseWidget::BuildLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	PhaseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PhaseText"));
	PhaseText->SetText(FText::FromString(TEXT("Arrival")));
	PhaseText->SetColorAndOpacity(FSlateColor(FLinearColor::Yellow));
	PhaseText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 22));
	PhaseText->SetAutoWrapText(false);

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetPadding(FMargin(16.0f, 10.0f));
	Backdrop->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
	Backdrop->SetContent(PhaseText);

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Backdrop))
	{
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		PanelSlot->SetPosition(FVector2D(32.0f, 32.0f));
		PanelSlot->SetAlignment(FVector2D(0.0f, 0.0f));
	}

	UBorder* CrosshairX = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CrosshairX"));
	CrosshairX->SetBrushColor(FLinearColor::White);
	UBorder* CrosshairY = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CrosshairY"));
	CrosshairY->SetBrushColor(FLinearColor::White);

	if (UCanvasPanelSlot* HorizontalSlot = Canvas->AddChildToCanvas(CrosshairX))
	{
		HorizontalSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		HorizontalSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		HorizontalSlot->SetAutoSize(false);
		HorizontalSlot->SetSize(FVector2D(14.0f, 2.0f));
		HorizontalSlot->SetPosition(FVector2D::ZeroVector);
	}

	if (UCanvasPanelSlot* VerticalSlot = Canvas->AddChildToCanvas(CrosshairY))
	{
		VerticalSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		VerticalSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		VerticalSlot->SetAutoSize(false);
		VerticalSlot->SetSize(FVector2D(2.0f, 14.0f));
		VerticalSlot->SetPosition(FVector2D::ZeroVector);
	}

	WidgetTree->RootWidget = Canvas;
}

void UMurderAtMidnightPhaseWidget::RefreshText()
{
	if (!PhaseText)
	{
		return;
	}

	UWorld* World = GetWorld();
	const AMurderAtMidnightGameState* MatchState = World ? World->GetGameState<AMurderAtMidnightGameState>() : nullptr;
	if (!MatchState)
	{
		PhaseText->SetText(FText::FromString(TEXT("No match clock")));
		return;
	}

	const int32 SecondsRemaining = FMath::CeilToInt(MatchState->GetPhaseSecondsRemaining());
	const APlayerController* Controller = GetOwningPlayer();
	const AMurderAtMidnightCharacter* Character = Controller ? Cast<AMurderAtMidnightCharacter>(Controller->GetPawn()) : nullptr;
	const UMurderAtMidnightInventoryComponent* Inventory = Character ? Character->GetInventory() : nullptr;
	const FString Carrying = Inventory ? Inventory->GetCarriedLabel() : TEXT("nothing");
	FString CabinetState = TEXT("isclosed");
	if (World)
	{
		for (TActorIterator<AMurderAtMidnightCabinet> It(World); It; ++It)
		{
			if (const UMurderAtMidnightInteractableComponent* Interactable = It->GetInteractable())
			{
				CabinetState = Interactable->IsOpen() ? TEXT("isopen") : TEXT("isclosed");
				break;
			}
		}
	}
	const AMurderAtMidnightPlayerController* MatchController = Cast<AMurderAtMidnightPlayerController>(Controller);
	const FString Result = MatchController ? MatchController->GetLastInteractMessage() : FString();

	FString Text = FString::Printf(TEXT("%s    %d\nF use    G drop    Click attack\nCarrying: %s\n%s"), *PhaseLabel(MatchState->Phase), SecondsRemaining, *Carrying, *CabinetState);
	if (!Result.IsEmpty())
	{
		Text += TEXT("\n") + Result;
	}

	if (MatchController && !MatchController->GetPrivateRole().IsEmpty())
	{
		Text += TEXT("\n\n") + MatchController->GetPrivateRole();
	}

	if (MatchController && !MatchController->GetPrivateAgenda().IsEmpty())
	{
		Text += TEXT("\n\nYour agenda\n") + MatchController->GetPrivateAgenda();
	}

	if (const AMurderAtMidnightPlayerState* State = MatchController ? MatchController->GetPlayerState<AMurderAtMidnightPlayerState>() : nullptr)
	{
		if (State->LifeState == EMurderAtMidnightLifeState::Dead)
		{
			Text += TEXT("\n\nYou are dead.");
		}
	}

	PhaseText->SetText(FText::FromString(Text));
}
