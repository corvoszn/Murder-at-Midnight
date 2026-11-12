// Copyright Epic Games, Inc. All Rights Reserved.


#include "MurderAtMidnightPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "MurderAtMidnightCameraManager.h"
#include "MurderAtMidnightPhaseWidget.h"
#include "Blueprint/UserWidget.h"
#include "MurderAtMidnight.h"
#include "MurderAtMidnightCharacter.h"
#include "MurderAtMidnightInteractableComponent.h"
#include "MurderAtMidnightKey.h"
#include "MurderAtMidnightGameMode.h"
#include "MurderAtMidnightPlayerState.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Widgets/Input/SVirtualJoystick.h"

AMurderAtMidnightPlayerController::AMurderAtMidnightPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = AMurderAtMidnightCameraManager::StaticClass();
}

void AMurderAtMidnightPlayerController::HostMatch()
{
	if (IsLocalController())
	{
		ClientTravel(TEXT("/Game/FirstPerson/Lvl_FirstPerson?listen"), TRAVEL_Absolute);
	}
}

void AMurderAtMidnightPlayerController::JoinMatch(const FString& Address)
{
	if (IsLocalController() && !Address.IsEmpty())
	{
		ClientTravel(Address, TRAVEL_Absolute);
	}
}

void AMurderAtMidnightPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	ShowPhaseWidget();

	if (IsLocalController())
	{
		ServerRequestPrivateAgenda();
	}
}

void AMurderAtMidnightPlayerController::ShowPhaseWidget()
{
	if (!IsLocalPlayerController() || PhaseWidget)
	{
		return;
	}

	PhaseWidget = CreateWidget<UMurderAtMidnightPhaseWidget>(this, UMurderAtMidnightPhaseWidget::StaticClass());
	if (PhaseWidget)
	{
		PhaseWidget->AddToViewport(1000);
	}
}

void AMurderAtMidnightPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ShowPhaseWidget();

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogMurderAtMidnight, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AMurderAtMidnightPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}

}

void AMurderAtMidnightPlayerController::RequestInteract()
{
	if (!IsLocalController())
	{
		return;
	}

	FVector ViewStart = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	GetPlayerViewPoint(ViewStart, ViewRotation);
	ServerRequestInteract(ViewStart, ViewRotation.Vector());
}

void AMurderAtMidnightPlayerController::RequestDrop()
{
	if (!IsLocalController())
	{
		return;
	}

	FVector ViewStart = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	GetPlayerViewPoint(ViewStart, ViewRotation);
	ServerRequestDrop(ViewRotation.Vector());
}

void AMurderAtMidnightPlayerController::RequestKill()
{
	if (!IsLocalController())
	{
		return;
	}

	FVector ViewStart = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	GetPlayerViewPoint(ViewStart, ViewRotation);
	ServerRequestKill(ViewStart, ViewRotation.Vector());
}

bool AMurderAtMidnightPlayerController::IsDead() const
{
	const AMurderAtMidnightPlayerState* State = GetPlayerState<AMurderAtMidnightPlayerState>();
	return State && State->LifeState == EMurderAtMidnightLifeState::Dead;
}

bool AMurderAtMidnightPlayerController::TraceAlongView(const FVector& ViewStart, const FVector& ViewAim, FHitResult& OutHit, float MaxDistance)
{
	AMurderAtMidnightCharacter* Self = Cast<AMurderAtMidnightCharacter>(GetPawn());
	UWorld* World = GetWorld();
	if (!Self || !World)
	{
		return false;
	}

	const FVector Eyes = Self->GetPawnViewLocation();
	const FVector Aim = ViewAim.GetSafeNormal();
	FVector Start = Eyes;
	if (!Aim.IsNearlyZero() && FVector::DistSquared(ViewStart, Eyes) <= FMath::Square(200.0f))
	{
		Start = ViewStart;
	}

	const FVector TraceAim = Aim.IsNearlyZero() ? GetControlRotation().Vector() : Aim;
	const FVector End = Start + TraceAim * MaxDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(Interact), false, Self);
	TArray<AActor*> Attached;
	Self->GetAttachedActors(Attached, true, true);
	Params.AddIgnoredActors(Attached);

	const FCollisionShape SweepShape = FCollisionShape::MakeSphere(16.0f);
	return World->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, ECC_Visibility, SweepShape, Params);
}

void AMurderAtMidnightPlayerController::ServerRequestInteract_Implementation(FVector ViewStart, FVector ViewAim)
{
	AMurderAtMidnightCharacter* Self = Cast<AMurderAtMidnightCharacter>(GetPawn());
	if (!Self || IsDead())
	{
		ClientInteractMessage(TEXT("The server refused that."));
		return;
	}

	FHitResult Hit;
	if (!TraceAlongView(ViewStart, ViewAim, Hit) || !Hit.GetActor())
	{
		ClientInteractMessage(TEXT("Nothing there."));
		return;
	}

	FString Message;
	if (AMurderAtMidnightCharacter* Other = Cast<AMurderAtMidnightCharacter>(Hit.GetActor()))
	{
		UMurderAtMidnightInteractableComponent* Held = nullptr;
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<AMurderAtMidnightKey> It(World); It; ++It)
			{
				if (UMurderAtMidnightInteractableComponent* Interactable = It->GetInteractable())
				{
					if (Interactable->GetCarrier() == Self)
					{
						Held = Interactable;
						break;
					}
				}
			}
		}

		if (!Held)
		{
			Message = TEXT("You have nothing to give.");
		}
		else
		{
			Held->ServerGive(Self, Other, Message);
		}
	}
	else if (UMurderAtMidnightInteractableComponent* Interactable = Hit.GetActor()->FindComponentByClass<UMurderAtMidnightInteractableComponent>())
	{
		Interactable->ServerInteract(Self, Message);
	}
	else
	{
		Message = TEXT("Nothing there.");
	}

	UE_LOG(LogMurderAtMidnight, Log, TEXT("%s tried to use something: %s"), *GetName(), *Message);
	ClientInteractMessage(Message);
}

void AMurderAtMidnightPlayerController::ServerRequestDrop_Implementation(FVector ViewAim)
{
	AMurderAtMidnightCharacter* Self = Cast<AMurderAtMidnightCharacter>(GetPawn());
	UWorld* World = GetWorld();
	if (!Self || !World || IsDead())
	{
		ClientInteractMessage(TEXT("The server refused that."));
		return;
	}

	UMurderAtMidnightInteractableComponent* Held = nullptr;
	for (TActorIterator<AMurderAtMidnightKey> It(World); It; ++It)
	{
		if (UMurderAtMidnightInteractableComponent* Interactable = It->GetInteractable())
		{
			if (Interactable->GetCarrier() == Self)
			{
				Held = Interactable;
				break;
			}
		}
	}

	FString Message;
	if (!Held)
	{
		Message = TEXT("You are not holding anything.");
	}
	else
	{
		const FVector Aim = ViewAim.GetSafeNormal2D();
		FVector DropLocation = Self->GetActorLocation() + Aim * 120.0f;
		if (const UCapsuleComponent* Capsule = Self->GetCapsuleComponent())
		{
			DropLocation.Z = Self->GetActorLocation().Z - Capsule->GetScaledCapsuleHalfHeight() + 36.0f;
		}

		Held->ServerDrop(Self, DropLocation, Message);
	}

	UE_LOG(LogMurderAtMidnight, Log, TEXT("%s tried to drop something: %s"), *GetName(), *Message);
	ClientInteractMessage(Message);
}

void AMurderAtMidnightPlayerController::ServerRequestKill_Implementation(FVector ViewStart, FVector ViewAim)
{
	if (IsDead())
	{
		ClientInteractMessage(TEXT("The server refused that."));
		return;
	}

	UWorld* World = GetWorld();
	AMurderAtMidnightGameMode* MatchMode = World ? World->GetAuthGameMode<AMurderAtMidnightGameMode>() : nullptr;
	AMurderAtMidnightCharacter* Self = Cast<AMurderAtMidnightCharacter>(GetPawn());
	if (!MatchMode || !Self)
	{
		ClientInteractMessage(TEXT("The server refused that."));
		return;
	}

	FHitResult Hit;
	if (!TraceAlongView(ViewStart, ViewAim, Hit, 400.0f) || !Hit.GetActor())
	{
		ClientInteractMessage(TEXT("Nobody there."));
		return;
	}

	AMurderAtMidnightCharacter* VictimBody = Cast<AMurderAtMidnightCharacter>(Hit.GetActor());
	if (!VictimBody)
	{
		ClientInteractMessage(TEXT("Nobody there."));
		return;
	}

	AMurderAtMidnightPlayerController* Victim = nullptr;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		AMurderAtMidnightPlayerController* Candidate = Cast<AMurderAtMidnightPlayerController>(It->Get());
		if (Candidate && Candidate->GetPawn() == VictimBody)
		{
			Victim = Candidate;
			break;
		}
	}

	if (!Victim || Victim == this)
	{
		ClientInteractMessage(TEXT("Nobody there."));
		return;
	}

	if (FVector::DistSquared(Self->GetActorLocation(), VictimBody->GetActorLocation()) > FMath::Square(450.0f))
	{
		ClientInteractMessage(TEXT("They are too far away."));
		return;
	}

	FString Message;
	MatchMode->TryMurder(this, Victim, Message);
	UE_LOG(LogMurderAtMidnight, Log, TEXT("A kill was requested."));
	ClientInteractMessage(Message);
}

void AMurderAtMidnightPlayerController::ClientInteractMessage_Implementation(const FString& Message)
{
	LastInteractMessage = Message;
}

void AMurderAtMidnightPlayerController::ServerRequestPrivateAgenda_Implementation()
{
	UWorld* World = GetWorld();
	AMurderAtMidnightGameMode* MatchMode = World ? World->GetAuthGameMode<AMurderAtMidnightGameMode>() : nullptr;
	if (MatchMode)
	{
		MatchMode->SendPrivateAgenda(this);
		MatchMode->SendPrivateRole(this);
	}
}

void AMurderAtMidnightPlayerController::ClientReceiveAgenda_Implementation(const FString& AgendaText)
{
	PrivateAgenda = AgendaText;
}

void AMurderAtMidnightPlayerController::ClientReceiveRole_Implementation(const FString& RoleText)
{
	PrivateRole = RoleText;
}

bool AMurderAtMidnightPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
