// Copyright Epic Games, Inc. All Rights Reserved.

#include "MurderAtMidnightProp.h"
#include "Components/StaticMeshComponent.h"
#include "MurderAtMidnightInteractableComponent.h"
#include "UObject/ConstructorHelpers.h"

AMurderAtMidnightProp::AMurderAtMidnightProp()
{
	bReplicates = true;
	SetReplicateMovement(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}

	Interactable = CreateDefaultSubobject<UMurderAtMidnightInteractableComponent>(TEXT("Interactable"));
}
