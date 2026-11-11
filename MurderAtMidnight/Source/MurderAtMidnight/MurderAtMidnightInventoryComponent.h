// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MurderAtMidnightInventoryComponent.generated.h"

/**
 * What one person is carrying.
 * The list replicates to every player. Carried objects are public.
 * Only the server adds or removes an entry.
 */
UCLASS(ClassGroup = (Match), meta = (BlueprintSpawnableComponent))
class MURDERATMIDNIGHT_API UMurderAtMidnightInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMurderAtMidnightInventoryComponent();

	bool HasItem(FName ItemId) const;
	bool IsFull() const;
	FString GetCarriedLabel() const;

	/** Server only. False when the hands are full or the id is empty. */
	bool TryAddItem(FName ItemId);

	/** Server only. False when that id is not being carried. */
	bool TryRemoveItem(FName ItemId);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** Item ids this person is holding. Everyone receives this list. */
	UPROPERTY(Replicated)
	TArray<FName> CarriedItemIds;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 MaxSlots = 1;
};
