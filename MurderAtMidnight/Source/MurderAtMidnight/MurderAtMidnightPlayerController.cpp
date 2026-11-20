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
#include "MurderAtMidnightProp.h"
#include "MurderAtMidnightGameMode.h"
#include "MurderAtMidnightPlayerState.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "MurderAtMidnightOnlineSubsystem.h"
#include "EOSShared.h"
#include "IOnlineSubsystemEOS.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSubsystemEOSTypesPublic.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "VoiceChat.h"
#include "TimerManager.h"

namespace
{
	UMurderAtMidnightInteractableComponent* FindHeld(UWorld* World, AMurderAtMidnightCharacter* Who)
	{
		if (!World || !Who)
		{
			return nullptr;
		}

		for (TActorIterator<AMurderAtMidnightProp> It(World); It; ++It)
		{
			if (UMurderAtMidnightInteractableComponent* Interactable = It->GetInteractable())
			{
				if (Interactable->GetCarrier() == Who)
				{
					return Interactable;
				}
			}
		}

		return nullptr;
	}
}

AMurderAtMidnightPlayerController::AMurderAtMidnightPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = AMurderAtMidnightCameraManager::StaticClass();
}

void AMurderAtMidnightPlayerController::HostMatch(const FString& Mode)
{
	if (!IsLocalController())
	{
		return;
	}

	if (Mode.Equals(TEXT("local"), ESearchCase::IgnoreCase))
	{
		ClientTravel(TEXT("/Game/FirstPerson/Lvl_FirstPerson?listen?bUseIPSockets"), TRAVEL_Absolute);
		return;
	}

	if (UGameInstance* Instance = GetGameInstance())
	{
		if (UMurderAtMidnightOnlineSubsystem* Online = Instance->GetSubsystem<UMurderAtMidnightOnlineSubsystem>())
		{
			Online->Host();
		}
	}
}

void AMurderAtMidnightPlayerController::JoinMatch(const FString& Address)
{
	if (!IsLocalController())
	{
		return;
	}

	if (!Address.IsEmpty())
	{
		ClientTravel(Address, TRAVEL_Absolute);
		return;
	}

	if (UGameInstance* Instance = GetGameInstance())
	{
		if (UMurderAtMidnightOnlineSubsystem* Online = Instance->GetSubsystem<UMurderAtMidnightOnlineSubsystem>())
		{
			Online->Join();
		}
	}
}

void AMurderAtMidnightPlayerController::ShowLocalStatus(const FString& Line)
{
	LastInteractMessage = Line;
}

void AMurderAtMidnightPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	ShowPhaseWidget();

	if (IsLocalController())
	{
		ServerRequestPrivateAgenda();
		GetWorldTimerManager().SetTimer(VoiceTimer, this, &AMurderAtMidnightPlayerController::UpdateProximityVoice, 0.1f, true);
	}
}

void AMurderAtMidnightPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(VoiceTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void AMurderAtMidnightPlayerController::UpdateProximityVoice()
{
	if (!IsLocalController())
	{
		return;
	}

	IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld(), TEXT("EOS"));
	IOnlineIdentityPtr Identity = Subsystem ? Subsystem->GetIdentityInterface() : nullptr;
	IOnlineSubsystemEOS* Eos = static_cast<IOnlineSubsystemEOS*>(Subsystem);
	if (!Eos || !Identity.IsValid() || Identity->GetLoginStatus(0) != ELoginStatus::LoggedIn)
	{
		return;
	}

	const FUniqueNetIdPtr LocalId = Identity->GetUniquePlayerId(0);
	if (!LocalId.IsValid() || LocalId->GetType() != FName(TEXT("EOS")))
	{
		return;
	}

	IVoiceChatUser* Voice = Eos->GetVoiceChatUserInterface(*LocalId);
	if (!Voice)
	{
		return;
	}

	const IUniqueNetIdEOS* LocalEosId = static_cast<const IUniqueNetIdEOS*>(LocalId.Get());
	const FString MyVoiceName = LexToString(LocalEosId->GetProductUserId());

	const AMurderAtMidnightPlayerState* MyState = GetPlayerState<AMurderAtMidnightPlayerState>();
	const bool bSilent = MyState && (MyState->LifeState == EMurderAtMidnightLifeState::Dead
		|| MyState->LifeState == EMurderAtMidnightLifeState::Rat);
	Voice->SetAudioInputDeviceMuted(bSilent);

	const APawn* MyPawn = GetPawn();
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	TMap<FString, FVector> Locations;
	if (GameState)
	{
		for (const APlayerState* OtherState : GameState->PlayerArray)
		{
			if (!OtherState || !OtherState->GetUniqueId().IsValid())
			{
				continue;
			}

			const FUniqueNetIdRepl& OtherId = OtherState->GetUniqueId();
			if (OtherId.GetUniqueNetId()->GetType() != FName(TEXT("EOS")))
			{
				continue;
			}

			const APawn* OtherPawn = OtherState->GetPawn();
			if (!OtherPawn)
			{
				continue;
			}

			const IUniqueNetIdEOS* OtherEosId = static_cast<const IUniqueNetIdEOS*>(OtherId.GetUniqueNetId().Get());
			Locations.Add(LexToString(OtherEosId->GetProductUserId()), OtherPawn->GetActorLocation());
		}
	}

	const float NearDistance = FMath::Min(VoiceFullVolumeDistance, VoiceHearDistance);
	const float FarDistance = FMath::Max(VoiceFullVolumeDistance, VoiceHearDistance);
	for (const FString& Channel : Voice->GetChannels())
	{
		for (const FString& Speaker : Voice->GetPlayersInChannel(Channel))
		{
			if (Speaker == MyVoiceName)
			{
				continue;
			}

			float Volume = 0.f;
			if (MyPawn)
			{
				if (const FVector* SpeakerLocation = Locations.Find(Speaker))
				{
					const float Distance = FVector::Dist(MyPawn->GetActorLocation(), *SpeakerLocation);
					Volume = FMath::GetMappedRangeValueClamped(
						FVector2D(NearDistance, FarDistance), FVector2D(1.f, 0.f), Distance);
				}
			}

			const float* Previous = LastVoiceVolumes.Find(Speaker);
			if (!Previous || !FMath::IsNearlyEqual(*Previous, Volume, 0.05f))
			{
				Voice->SetPlayerVolume(Speaker, Volume);
				LastVoiceVolumes.Add(Speaker, Volume);
			}
		}
	}
}

void AMurderAtMidnightPlayerController::ShowPhaseWidget()
{
	if (!IsLocalPlayerController() || PhaseWidget)
	{
		return;
	}

	const TSubclassOf<UMurderAtMidnightPhaseWidget> WidgetClass = PhaseWidgetClass
		? PhaseWidgetClass
		: TSubclassOf<UMurderAtMidnightPhaseWidget>(UMurderAtMidnightPhaseWidget::StaticClass());
	PhaseWidget = CreateWidget<UMurderAtMidnightPhaseWidget>(this, WidgetClass);
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

void AMurderAtMidnightPlayerController::RequestPrimaryPress()
{
	if (!IsLocalController())
	{
		return;
	}

	const AMurderAtMidnightGameState* MatchState = GetWorld() ? GetWorld()->GetGameState<AMurderAtMidnightGameState>() : nullptr;
	if (MatchState && MatchState->bVoteOpen)
	{
		RequestCastVote();
		return;
	}

	const AMurderAtMidnightCharacter* Self = Cast<AMurderAtMidnightCharacter>(GetPawn());
	const UMurderAtMidnightInventoryComponent* Inventory = Self ? Self->GetInventory() : nullptr;
	if (Inventory && Inventory->IsFull())
	{
		bLocalThrowCharging = true;
		ServerRequestBeginThrow();
		return;
	}

	FVector ViewStart = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	GetPlayerViewPoint(ViewStart, ViewRotation);
	ServerRequestGrab(ViewStart, ViewRotation.Vector());
}

void AMurderAtMidnightPlayerController::RequestPrimaryRelease()
{
	if (!IsLocalController() || !bLocalThrowCharging)
	{
		return;
	}

	bLocalThrowCharging = false;
	FRotator ViewRotation = FRotator::ZeroRotator;
	FVector ViewStart = FVector::ZeroVector;
	GetPlayerViewPoint(ViewStart, ViewRotation);
	ServerRequestThrow(ViewRotation.Vector());
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

void AMurderAtMidnightPlayerController::RequestVote()
{
	if (!IsLocalController())
	{
		return;
	}

	ServerRequestVote();
}

void AMurderAtMidnightPlayerController::RequestSkipPhase()
{
	if (!IsLocalController())
	{
		return;
	}

	ServerRequestSkipPhase();
}

void AMurderAtMidnightPlayerController::RequestCastVote()
{
	if (!IsLocalController())
	{
		return;
	}

	FVector ViewStart = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	GetPlayerViewPoint(ViewStart, ViewRotation);
	ServerRequestCastVote(ViewStart, ViewRotation.Vector());
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
	if (UMurderAtMidnightInteractableComponent* Held = FindHeld(World, Self))
	{
		if (AActor* HeldActor = Held->GetOwner())
		{
			Params.AddIgnoredActor(HeldActor);
		}
	}

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
		UMurderAtMidnightInteractableComponent* Held = FindHeld(GetWorld(), Self);

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

	UMurderAtMidnightInteractableComponent* Held = FindHeld(World, Self);

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
		ThrowChargeStartedAt = -1.0f;
	}

	UE_LOG(LogMurderAtMidnight, Log, TEXT("%s tried to drop something: %s"), *GetName(), *Message);
	ClientInteractMessage(Message);
}

void AMurderAtMidnightPlayerController::ServerRequestGrab_Implementation(FVector ViewStart, FVector ViewAim)
{
	AMurderAtMidnightCharacter* Self = Cast<AMurderAtMidnightCharacter>(GetPawn());
	UWorld* World = GetWorld();
	if (!Self || !World || IsDead())
	{
		ClientInteractMessage(TEXT("The server refused that."));
		return;
	}

	if (UMurderAtMidnightInteractableComponent* Held = FindHeld(World, Self))
	{
		if (Held->IsHoldPending())
		{
			ClientInteractMessage(TEXT("It is not in your hand yet."));
		}
		return;
	}

	FHitResult Hit;
	if (TraceAlongView(ViewStart, ViewAim, Hit) && Hit.GetActor())
	{
		if (UMurderAtMidnightInteractableComponent* Interactable = Hit.GetActor()->FindComponentByClass<UMurderAtMidnightInteractableComponent>())
		{
			if (Interactable->CanBeCarried())
			{
				FString Message;
				Interactable->ServerInteract(Self, Message);
				ClientInteractMessage(Message);
				return;
			}
		}
	}

	ServerRequestKill_Implementation(ViewStart, ViewAim);
}

void AMurderAtMidnightPlayerController::ServerRequestBeginThrow_Implementation()
{
	UWorld* World = GetWorld();
	AMurderAtMidnightCharacter* Self = Cast<AMurderAtMidnightCharacter>(GetPawn());
	UMurderAtMidnightInteractableComponent* Held = World ? FindHeld(World, Self) : nullptr;
	if (!World || !Held || Held->GetCarrier() != Self || Held->IsHoldPending())
	{
		ThrowChargeStartedAt = -1.0f;
		return;
	}

	ThrowChargeStartedAt = World->GetTimeSeconds();
}

void AMurderAtMidnightPlayerController::ServerRequestThrow_Implementation(FVector ViewAim)
{
	UWorld* World = GetWorld();
	AMurderAtMidnightCharacter* Self = Cast<AMurderAtMidnightCharacter>(GetPawn());
	if (!World || !Self || IsDead() || ThrowChargeStartedAt < 0.0f)
	{
		ThrowChargeStartedAt = -1.0f;
		return;
	}

	const float HoldSeconds = World->GetTimeSeconds() - ThrowChargeStartedAt;
	ThrowChargeStartedAt = -1.0f;

	UMurderAtMidnightInteractableComponent* Held = FindHeld(World, Self);
	FString Message;
	if (!Held)
	{
		Message = TEXT("You are not holding anything.");
	}
	else
	{
		Held->ServerThrow(Self, ViewAim, HoldSeconds, Message);
	}

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

	UMurderAtMidnightInteractableComponent* Weapon = FindHeld(World, Self);
	if (!Weapon || !Weapon->IsWeapon())
	{
		ClientInteractMessage(TEXT("You are not holding a weapon."));
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
	if (MatchMode->TryMurder(this, Victim, Message))
	{
		Weapon->ServerMarkBloody();
	}
	UE_LOG(LogMurderAtMidnight, Log, TEXT("A kill was requested."));
	ClientInteractMessage(Message);
}

void AMurderAtMidnightPlayerController::ServerRequestSkipPhase_Implementation()
{
	AMurderAtMidnightGameMode* MatchMode = GetWorld() ? GetWorld()->GetAuthGameMode<AMurderAtMidnightGameMode>() : nullptr;
	if (!MatchMode)
	{
		return;
	}

	MatchMode->SkipToNextPhase();
}

void AMurderAtMidnightPlayerController::ServerRequestVote_Implementation()
{
	AMurderAtMidnightGameMode* MatchMode = GetWorld() ? GetWorld()->GetAuthGameMode<AMurderAtMidnightGameMode>() : nullptr;
	if (!MatchMode || IsDead())
	{
		ClientInteractMessage(TEXT("The server refused that."));
		return;
	}

	FString Message;
	MatchMode->TryCallVote(this, Message);
	ClientInteractMessage(Message);
}

void AMurderAtMidnightPlayerController::ServerRequestCastVote_Implementation(FVector ViewStart, FVector ViewAim)
{
	AMurderAtMidnightGameMode* MatchMode = GetWorld() ? GetWorld()->GetAuthGameMode<AMurderAtMidnightGameMode>() : nullptr;
	if (!MatchMode || IsDead())
	{
		ClientInteractMessage(TEXT("The server refused that."));
		return;
	}

	FHitResult Hit;
	AMurderAtMidnightPlayerController* Accused = nullptr;
	if (TraceAlongView(ViewStart, ViewAim, Hit) && Hit.GetActor())
	{
		if (AMurderAtMidnightCharacter* Other = Cast<AMurderAtMidnightCharacter>(Hit.GetActor()))
		{
			Accused = Cast<AMurderAtMidnightPlayerController>(Other->GetController());
		}
	}

	FString Message;
	if (!Accused)
	{
		Message = TEXT("Look at a living player.");
	}
	else
	{
		MatchMode->TryCastVote(this, Accused, Message);
	}

	ClientInteractMessage(Message);
}

void AMurderAtMidnightPlayerController::ClientReceiveVoteChoice_Implementation(const FString& Line)
{
	VoteChoice = Line;
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

void AMurderAtMidnightPlayerController::ClientReceiveAgendaState_Implementation(const FString& AgendaText, bool bInComplete)
{
	PrivateAgenda = AgendaText;
	bAgendaComplete = bInComplete;
}

void AMurderAtMidnightPlayerController::ClientReceiveRole_Implementation(bool bInIsKiller, const FString& InTargetNames)
{
	bRoleReceived = true;
	bIsKiller = bInIsKiller;
	TargetNames = InTargetNames;
}

bool AMurderAtMidnightPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
