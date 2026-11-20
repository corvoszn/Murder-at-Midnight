// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "MurderAtMidnightCharacter.h"
#include "MurderAtMidnightInteractableComponent.generated.h"

UENUM()
enum class EMurderAtMidnightInteractKind : uint8
{
	Pickup,
	LockedContainer,
	Weapon
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
	bool IsHoldPending() const { return bHoldPending; }
	bool IsHeldInView() const { return bHeldInView; }
	bool CanBeCarried() const { return Kind == EMurderAtMidnightInteractKind::Pickup || Kind == EMurderAtMidnightInteractKind::Weapon; }
	bool IsWeapon() const { return Kind == EMurderAtMidnightInteractKind::Weapon; }
	bool IsBloody() const { return bBloody; }
	FName GetItemId() const { return ItemId; }
	AMurderAtMidnightCharacter* GetCarrier() const { return Carrier; }

	/** Server only. Pickup or unlock, after the controller has already checked range. */
	bool ServerInteract(AMurderAtMidnightCharacter* Who, FString& OutMessage);

	/** Server only. Put a carried pickup back in the world. */
	bool ServerDrop(AMurderAtMidnightCharacter* Who, const FVector& WorldLocation, FString& OutMessage);

	/** Server only. Launch a carried pickup along the aim. Speed comes from its weight and how long the click was held. */
	bool ServerThrow(AMurderAtMidnightCharacter* Who, const FVector& AimDirection, float HoldSeconds, FString& OutMessage);

	/** Server only. Move a carried pickup into another person's hands. */
	bool ServerGive(AMurderAtMidnightCharacter* Giver, AMurderAtMidnightCharacter* Receiver, FString& OutMessage);

	/** Server only. The weapon was used. This stores that fact and nothing about who did it. */
	void ServerMarkBloody();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UFUNCTION()
	void OnRep_State();

private:
	/** Centimetres in front of the view. The held object is placed here. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hold", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float HoldDistance = 80.0f;

	/** Centimetres above or below the view. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hold", meta = (AllowPrivateAccess = "true"))
	float HoldHeight = -15.0f;

	/** Push applied to a loose object this one is pressed against. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hold", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float BumpForce = 20000.0f;
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

	/** True after this weapon has been used. Everyone receives this. It does not store who used it. */
	UPROPERTY(ReplicatedUsing = OnRep_State)
	bool bBloody = false;

	/** Who is holding this pickup. Everyone receives this. Null when it is on the floor. */
	UPROPERTY(ReplicatedUsing = OnRep_State)
	TObjectPtr<AMurderAtMidnightCharacter> Carrier;

	/** True once the object is in the hold pose. False during the pickup delay. Everyone receives this. */
	UPROPERTY(ReplicatedUsing = OnRep_State)
	bool bHeldInView = false;

	/** Rotation relative to the holder's view, captured when the hold starts. Everyone receives this. */
	UPROPERTY(Replicated)
	FRotator HoldRelativeRotation = FRotator::ZeroRotator;

	void AttachToCarrier();
	void ReleaseAt(const FVector& WorldLocation);
	void ApplyVisual() const;
	void StartHold(AMurderAtMidnightCharacter* Who);
	void StopHold(AMurderAtMidnightCharacter* Who);
	void ReleaseThrown(AMurderAtMidnightCharacter* Who, const FVector& Velocity);
	void PlaceAtHold();
	void MoveClearOfWorld(const FVector& Desired);
	void LeaveTheHolder(AMurderAtMidnightCharacter* Who);
	void StopCarriedMotion(UPrimitiveComponent* Body) const;
	void FinishSettledDrop(AActor* OwnerActor);
	bool IsClearOfHolder(const AActor* OwnerActor) const;
	void CancelPendingPickup(AMurderAtMidnightCharacter* Who, const FString& Message);
	UPrimitiveComponent* FindHoldBody() const;
	float ReadCarryMass(UPrimitiveComponent* Body) const;
	FVector GetHoldPoint() const;
	FRotator GetHoldRotation() const;
	void IgnoreHolder(AMurderAtMidnightCharacter* Who);
	void RestoreHolder();
	void KeepPlayersOff() const;

	bool bHoldPending = false;
	bool bSettlingDrop = false;
	int32 SettleFrames = 0;
	FVector SettledDropLocation = FVector::ZeroVector;
	float HoldPendingUntil = 0.0f;
	float CarryMass = 0.0f;
	bool bRestorePhysicsOnDrop = false;

	/** Parts whose pawn collision was turned off for the hold, so it can be put back. */
	TArray<TObjectPtr<UPrimitiveComponent>> IgnoredParts;

	TArray<TEnumAsByte<ECollisionResponse>> SavedPawnResponses;

	TWeakObjectPtr<AMurderAtMidnightCharacter> IgnoredHolder;
};
