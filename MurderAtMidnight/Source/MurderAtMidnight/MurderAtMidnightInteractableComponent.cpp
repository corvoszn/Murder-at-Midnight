// Copyright Epic Games, Inc. All Rights Reserved.

#include "MurderAtMidnightInteractableComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "MurderAtMidnightCharacter.h"
#include "MurderAtMidnightGameMode.h"
#include "MurderAtMidnightInventoryComponent.h"
#include "MurderAtMidnightPlayerController.h"
#include "Net/UnrealNetwork.h"

namespace
{
	const FVector CarriedKeyOffset(40.0f, 12.0f, -12.0f);
}

UMurderAtMidnightInteractableComponent::UMurderAtMidnightInteractableComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UMurderAtMidnightInteractableComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyVisual();
}

void UMurderAtMidnightInteractableComponent::Configure(EMurderAtMidnightInteractKind InKind, FName InItemId, FName InRequiredItemId)
{
	Kind = InKind;
	ItemId = InItemId;
	RequiredItemId = InRequiredItemId;
}

bool UMurderAtMidnightInteractableComponent::ServerInteract(AMurderAtMidnightCharacter* Who, FString& OutMessage)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !Who)
	{
		OutMessage = TEXT("The server refused that.");
		return false;
	}

	UMurderAtMidnightInventoryComponent* Inventory = Who->GetInventory();
	if (!Inventory)
	{
		OutMessage = TEXT("The server refused that.");
		return false;
	}

	if (Kind == EMurderAtMidnightInteractKind::Pickup)
	{
		if (bCarried)
		{
			OutMessage = TEXT("Someone is already holding that.");
			return false;
		}

		if (!Inventory->TryAddItem(ItemId))
		{
			OutMessage = TEXT("Your hands are full.");
			return false;
		}

		Carrier = Who;
		bCarried = true;
		AttachToCarrier();
		ApplyVisual();
		OwnerActor->ForceNetUpdate();
		OutMessage = TEXT("You picked up the key.");
		return true;
	}

	if (bOpen)
	{
		if (!Inventory->HasItem(RequiredItemId))
		{
			OutMessage = TEXT("isopen");
			return false;
		}

		bOpen = false;
	}
	else
	{
		if (!Inventory->HasItem(RequiredItemId))
		{
			OutMessage = TEXT("isclosed");
			return false;
		}

		bOpen = true;
	}

	ApplyVisual();
	OwnerActor->ForceNetUpdate();
	OutMessage = bOpen ? TEXT("isopen") : TEXT("isclosed");

	if (UWorld* World = GetWorld())
	{
		if (AMurderAtMidnightGameMode* MatchMode = World->GetAuthGameMode<AMurderAtMidnightGameMode>())
		{
			if (AMurderAtMidnightPlayerController* Controller = Cast<AMurderAtMidnightPlayerController>(Who->GetController()))
			{
				MatchMode->NotifyCabinetLockChanged(Controller, bOpen);
			}
		}
	}

	return true;
}

bool UMurderAtMidnightInteractableComponent::ServerDrop(AMurderAtMidnightCharacter* Who, const FVector& WorldLocation, FString& OutMessage)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !Who || !bCarried || Carrier != Who)
	{
		OutMessage = TEXT("You are not holding anything.");
		return false;
	}

	UMurderAtMidnightInventoryComponent* Inventory = Who->GetInventory();
	if (!Inventory || !Inventory->TryRemoveItem(ItemId))
	{
		OutMessage = TEXT("You are not holding anything.");
		return false;
	}

	Carrier = nullptr;
	bCarried = false;
	ReleaseAt(WorldLocation);
	ApplyVisual();
	OwnerActor->ForceNetUpdate();
	OutMessage = TEXT("You dropped the key.");
	return true;
}

bool UMurderAtMidnightInteractableComponent::ServerGive(AMurderAtMidnightCharacter* Giver, AMurderAtMidnightCharacter* Receiver, FString& OutMessage)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !Giver || !Receiver || Giver == Receiver || !bCarried || Carrier != Giver)
	{
		OutMessage = TEXT("You have nothing to give.");
		return false;
	}

	UMurderAtMidnightInventoryComponent* FromInventory = Giver->GetInventory();
	UMurderAtMidnightInventoryComponent* ToInventory = Receiver->GetInventory();
	if (!FromInventory || !ToInventory || !FromInventory->HasItem(ItemId))
	{
		OutMessage = TEXT("You have nothing to give.");
		return false;
	}

	if (ToInventory->IsFull())
	{
		OutMessage = TEXT("Their hands are full.");
		return false;
	}

	if (!FromInventory->TryRemoveItem(ItemId) || !ToInventory->TryAddItem(ItemId))
	{
		FromInventory->TryAddItem(ItemId);
		OutMessage = TEXT("The server refused that.");
		return false;
	}

	Carrier = Receiver;
	bCarried = true;
	AttachToCarrier();
	ApplyVisual();
	OwnerActor->ForceNetUpdate();
	OutMessage = TEXT("You handed over the key.");
	return true;
}

void UMurderAtMidnightInteractableComponent::AttachToCarrier()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !Carrier)
	{
		return;
	}

	UCameraComponent* Camera = Carrier->GetFirstPersonCameraComponent();
	if (!Camera)
	{
		return;
	}

	OwnerActor->AttachToComponent(Camera, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	OwnerActor->SetActorRelativeLocation(CarriedKeyOffset);
	OwnerActor->SetActorRelativeRotation(FRotator::ZeroRotator);
}

void UMurderAtMidnightInteractableComponent::ReleaseAt(const FVector& WorldLocation)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	OwnerActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	OwnerActor->SetActorLocation(WorldLocation);
	OwnerActor->SetActorRotation(FRotator::ZeroRotator);
}

void UMurderAtMidnightInteractableComponent::ApplyVisual() const
{
	AActor* OwnerActor = GetOwner();
	UStaticMeshComponent* Mesh = OwnerActor ? OwnerActor->FindComponentByClass<UStaticMeshComponent>() : nullptr;
	if (!Mesh)
	{
		return;
	}

	if (Kind == EMurderAtMidnightInteractKind::Pickup)
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (UPrimitiveComponent* Volume = Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent()))
		{
			if (Volume != Mesh)
			{
				Volume->SetCollisionEnabled(bCarried ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
			}
		}
		Mesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.85f, 0.62f, 0.12f));
		return;
	}

	const FVector Color = FVector(0.35f, 0.16f, 0.1f);
	Mesh->SetVectorParameterValueOnMaterials(TEXT("Color"), Color);
}

void UMurderAtMidnightInteractableComponent::OnRep_State()
{
	if (bCarried)
	{
		AttachToCarrier();
	}
	else if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

	ApplyVisual();
}

void UMurderAtMidnightInteractableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMurderAtMidnightInteractableComponent, bCarried);
	DOREPLIFETIME(UMurderAtMidnightInteractableComponent, bOpen);
	DOREPLIFETIME(UMurderAtMidnightInteractableComponent, Carrier);
}
