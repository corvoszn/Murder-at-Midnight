// Copyright Epic Games, Inc. All Rights Reserved.

#include "MurderAtMidnightCabinet.h"
#include "Components/StaticMeshComponent.h"
#include "MurderAtMidnightInteractableComponent.h"

AMurderAtMidnightCabinet::AMurderAtMidnightCabinet()
{
	if (Mesh)
	{
		Mesh->SetRelativeScale3D(FVector(0.9f, 0.55f, 1.35f));
	}

	if (Interactable)
	{
		Interactable->Configure(
			EMurderAtMidnightInteractKind::LockedContainer,
			NAME_None,
			FName(TEXT("CabinetKey")));
	}
}
