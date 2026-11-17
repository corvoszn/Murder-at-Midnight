// Copyright Epic Games, Inc. All Rights Reserved.

#include "MurderAtMidnightWeapon.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "MurderAtMidnightInteractableComponent.h"

AMurderAtMidnightWeapon::AMurderAtMidnightWeapon()
{
	PickupVolume = CreateDefaultSubobject<USphereComponent>(TEXT("PickupVolume"));
	PickupVolume->SetSphereRadius(36.0f);
	PickupVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetRootComponent(PickupVolume);

	if (Mesh)
	{
		Mesh->SetupAttachment(PickupVolume);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetRelativeScale3D(FVector(0.12f, 0.12f, 0.9f));
	}

	if (Interactable)
	{
		Interactable->Configure(EMurderAtMidnightInteractKind::Weapon, FName(TEXT("Weapon")), NAME_None);
	}
}
