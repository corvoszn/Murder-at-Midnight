// Copyright Epic Games, Inc. All Rights Reserved.

#include "MurderAtMidnightInventoryComponent.h"
#include "Net/UnrealNetwork.h"

UMurderAtMidnightInventoryComponent::UMurderAtMidnightInventoryComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

bool UMurderAtMidnightInventoryComponent::HasItem(FName ItemId) const
{
	return CarriedItemIds.Contains(ItemId);
}

bool UMurderAtMidnightInventoryComponent::IsFull() const
{
	return CarriedItemIds.Num() >= MaxSlots;
}

FString UMurderAtMidnightInventoryComponent::GetCarriedLabel() const
{
	if (CarriedItemIds.Num() == 0)
	{
		return TEXT("nothing");
	}

	if (CarriedItemIds[0] == FName(TEXT("CabinetKey")))
	{
		return TEXT("a key");
	}

	return CarriedItemIds[0].ToString();
}

bool UMurderAtMidnightInventoryComponent::TryAddItem(FName ItemId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || ItemId.IsNone() || IsFull() || HasItem(ItemId))
	{
		return false;
	}

	CarriedItemIds.Add(ItemId);
	return true;
}

bool UMurderAtMidnightInventoryComponent::TryRemoveItem(FName ItemId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	return CarriedItemIds.RemoveSingle(ItemId) > 0;
}

void UMurderAtMidnightInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMurderAtMidnightInventoryComponent, CarriedItemIds);
}
