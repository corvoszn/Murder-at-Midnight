// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MurderAtMidnightCharacter.h"
#include "MurderAtMidnightInteractableComponent.generated.h"

UENUM()
enum class EMurderAtMidnightInteractKind : uint8
{
	Pickup,
	LockedContainer
};

/**
 * One world object a player can use.
 * Pickup and locked container share this component.
 * Open, carried, and who is holding it replicate to everyone. The server is the only writer.
 */
UCLASS(ClassGroup = (Match), meta = (BlueprintSpawnableComponent))
class MURDERATMIDNIGHT_API UMurderAtMidnightInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMurderAtMidnightInteractableComponent();

	void Configure(EMurderAtMidnightInteractKind InKind, FName InItemId, FName InRequiredItemId);

	bool IsCarried() const { return bCarried; }
	bool IsOpen() const { return bOpen; }
	FName GetItemId() const { return ItemId; }
	AMurderAtMidnightCharacter* GetCarrier() const { return Carrier; }

	/** Server only. Pickup or unlock, after the controller has already checked range. */
	bool ServerInteract(AMurderAtMidnightCharacter* Who, FString& OutMessage);

	/** Server only. Put a carried pickup back in the world. */
	bool ServerDrop(AMurderAtMidnightCharacter* Who, const FVector& WorldLocation, FString& OutMessage);

	/** Server only. Move a carried pickup into another person's hands. */
	bool ServerGive(AMurderAtMidnightCharacter* Giver, AMurderAtMidnightCharacter* Receiver, FString& OutMessage);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

protected:
	UFUNCTION()
	void OnRep_State();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Interact")
	EMurderAtMidnightInteractKind Kind = EMurderAtMidnightInteractKind::Pickup;

	UPROPERTY(EditDefaultsOnly, Category = "Interact")
	FName ItemId;

	UPROPERTY(EditDefaultsOnly, Category = "Interact")
	FName RequiredItemId;

	/** True when a person is holding this pickup. Everyone receives this. */
	UPROPERTY(ReplicatedUsing = OnRep_State)
	bool bCarried = false;

	/** True when this container has been unlocked. Everyone receives this. */
	UPROPERTY(ReplicatedUsing = OnRep_State)
	bool bOpen = false;

	/** Who is holding this pickup. Everyone receives this. Null when it is on the floor. */
	UPROPERTY(ReplicatedUsing = OnRep_State)
	TObjectPtr<AMurderAtMidnightCharacter> Carrier;

	void AttachToCarrier();
	void ReleaseAt(const FVector& WorldLocation);
	void ApplyVisual() const;
};
