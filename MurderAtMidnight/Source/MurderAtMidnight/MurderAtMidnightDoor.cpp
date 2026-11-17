// Copyright Epic Games, Inc. All Rights Reserved.

#include "MurderAtMidnightDoor.h"
#include "Components/StaticMeshComponent.h"
#include "MurderAtMidnightInteractableComponent.h"

AMurderAtMidnightDoor::AMurderAtMidnightDoor()
{
	if (Mesh)
	{
		Mesh->SetRelativeScale3D(FVector(0.2f, 1.2f, 2.0f));
	}

	if (Interactable)
	{
		Interactable->Configure(
			EMurderAtMidnightInteractKind::LockedContainer,
			NAME_None,
			FName(TEXT("CabinetKey")));
	}
}
