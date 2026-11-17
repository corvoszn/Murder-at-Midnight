// Copyright Epic Games, Inc. All Rights Reserved.

#include "MurderAtMidnightCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MurderAtMidnight.h"
#include "MurderAtMidnightGameState.h"
#include "MurderAtMidnightInventoryComponent.h"
#include "MurderAtMidnightPlayerController.h"

AMurderAtMidnightCharacter::AMurderAtMidnightCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	Inventory = CreateDefaultSubobject<UMurderAtMidnightInventoryComponent>(TEXT("Inventory"));

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;
}

void AMurderAtMidnightCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AMurderAtMidnightCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AMurderAtMidnightCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMurderAtMidnightCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMurderAtMidnightCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AMurderAtMidnightCharacter::LookInput);
	}
	else
	{
		UE_LOG(LogMurderAtMidnight, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}

	PlayerInputComponent->BindKey(EKeys::F, IE_Pressed, this, &AMurderAtMidnightCharacter::InteractPressed);
	PlayerInputComponent->BindKey(EKeys::G, IE_Pressed, this, &AMurderAtMidnightCharacter::DropPressed);
	PlayerInputComponent->BindKey(EKeys::K, IE_Pressed, this, &AMurderAtMidnightCharacter::KillPressed);
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AMurderAtMidnightCharacter::KillPressed);
	PlayerInputComponent->BindKey(EKeys::V, IE_Pressed, this, &AMurderAtMidnightCharacter::VotePressed);
	PlayerInputComponent->BindKey(EKeys::P, IE_Pressed, this, &AMurderAtMidnightCharacter::SkipPhasePressed);
}

void AMurderAtMidnightCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (USkeletalMeshComponent* BodyMesh = GetMesh())
	{
		BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		BodyMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		BodyMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->InitialPushForceFactor = 0.0f;
		Movement->PushForceFactor = 0.0f;
		Movement->bScalePushForceToVelocity = false;
	}
}


void AMurderAtMidnightCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void AMurderAtMidnightCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void AMurderAtMidnightCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AMurderAtMidnightCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void AMurderAtMidnightCharacter::DoJumpStart()
{
	// pass Jump to the character
	Jump();
}

void AMurderAtMidnightCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}

void AMurderAtMidnightCharacter::InteractPressed()
{
	if (AMurderAtMidnightPlayerController* MatchController = Cast<AMurderAtMidnightPlayerController>(GetController()))
	{
		MatchController->RequestInteract();
	}
}

void AMurderAtMidnightCharacter::DropPressed()
{
	if (AMurderAtMidnightPlayerController* MatchController = Cast<AMurderAtMidnightPlayerController>(GetController()))
	{
		MatchController->RequestDrop();
	}
}

void AMurderAtMidnightCharacter::KillPressed()
{
	if (AMurderAtMidnightPlayerController* MatchController = Cast<AMurderAtMidnightPlayerController>(GetController()))
	{
		const AMurderAtMidnightGameState* MatchState = GetWorld() ? GetWorld()->GetGameState<AMurderAtMidnightGameState>() : nullptr;
		if (MatchState && MatchState->bVoteOpen)
		{
			MatchController->RequestCastVote();
			return;
		}

		MatchController->RequestKill();
	}
}

void AMurderAtMidnightCharacter::VotePressed()
{
	if (AMurderAtMidnightPlayerController* MatchController = Cast<AMurderAtMidnightPlayerController>(GetController()))
	{
		MatchController->RequestVote();
	}
}

void AMurderAtMidnightCharacter::SkipPhasePressed()
{
	if (AMurderAtMidnightPlayerController* MatchController = Cast<AMurderAtMidnightPlayerController>(GetController()))
	{
		MatchController->RequestSkipPhase();
	}
}

float AMurderAtMidnightCharacter::GetPickupDelay(float MassKg) const
{
	const float Alpha = FMath::Clamp(MassKg / FMath::Max(HeavyMassKg, 1.0f), 0.0f, 1.0f);
	return FMath::Lerp(LightPickupSeconds, HeavyPickupSeconds, Alpha);
}

void AMurderAtMidnightCharacter::ApplyCarryMass(float MassKg)
{
	ClearCarryMass();

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	SavedMaxWalkSpeed = Movement->MaxWalkSpeed;
	bCarrySlowApplied = true;
	const float Alpha = FMath::Clamp(MassKg / FMath::Max(HeavyMassKg, 1.0f), 0.0f, 1.0f);
	Movement->MaxWalkSpeed = SavedMaxWalkSpeed * FMath::Lerp(1.0f, SlowestWalkScale, Alpha);
}

void AMurderAtMidnightCharacter::ClearCarryMass()
{
	if (!bCarrySlowApplied)
	{
		return;
	}

	bCarrySlowApplied = false;
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = SavedMaxWalkSpeed;
	}
}

void AMurderAtMidnightCharacter::ApplyDeadBody()
{
	if (bDeadBodyApplied)
	{
		return;
	}

	bDeadBodyApplied = true;
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	if (USkeletalMeshComponent* BodyMesh = GetMesh())
	{
		BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BodyMesh->AddRelativeRotation(FRotator(0.0f, 0.0f, 80.0f));
	}
}
