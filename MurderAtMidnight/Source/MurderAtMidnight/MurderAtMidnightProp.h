// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MurderAtMidnightProp.generated.h"

class UMurderAtMidnightInteractableComponent;
class UStaticMeshComponent;

/** A replicated world prop. The server owns it. Picking it up does not give the client ownership. */
UCLASS()
class MURDERATMIDNIGHT_API AMurderAtMidnightProp : public AActor
{
	GENERATED_BODY()

public:
	AMurderAtMidnightProp();

	UMurderAtMidnightInteractableComponent* GetInteractable() const { return Interactable; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UMurderAtMidnightInteractableComponent> Interactable;
};
