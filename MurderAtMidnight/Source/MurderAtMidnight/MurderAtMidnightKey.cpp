// Copyright Epic Games, Inc. All Rights Reserved.

#include "MurderAtMidnightKey.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "MurderAtMidnightInteractableComponent.h"
#include "UObject/ConstructorHelpers.h"

AMurderAtMidnightKey::AMurderAtMidnightKey()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded() && Mesh)
	{
		Mesh->SetStaticMesh(CylinderMesh.Object);
	}

	if (Mesh)
	{
		Mesh->SetRelativeScale3D(FVector(0.15f, 0.15f, 0.35f));
	}

	PickupVolume = CreateDefaultSubobject<USphereComponent>(TEXT("PickupVolume"));
	PickupVolume->SetupAttachment(Mesh);
	PickupVolume->SetSphereRadius(36.0f);
	PickupVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	if (Interactable)
	{
		Interactable->Configure(EMurderAtMidnightInteractKind::Pickup, FName(TEXT("CabinetKey")), NAME_None);
	}
}
