// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "MurderAtMidnightInventoryComponent.h"
#include "MurderAtMidnightCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A basic first person character
 */
UCLASS(abstract)
class AMurderAtMidnightCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** Carried item ids. Replicated to every player. The server is the only writer. */
	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<UMurderAtMidnightInventoryComponent> Inventory;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* MouseLookAction;
	
public:
	AMurderAtMidnightCharacter();

protected:

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	void InteractPressed();
	void DropPressed();
	void KillPressed();
	void VotePressed();
	void SkipPhasePressed();

public:

	void ApplyDeadBody();

private:
	bool bDeadBodyApplied = false;

protected:

	/** Set up input action bindings */
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	

public:

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	UMurderAtMidnightInventoryComponent* GetInventory() const { return Inventory; }

	/** Seconds before this mass is in the player's hand. */
	float GetPickupDelay(float MassKg) const;

	/** Slow walking while this mass is being carried. The server calls this. */
	void ApplyCarryMass(float MassKg);

	/** Put the walk speed back. The server calls this. */
	void ClearCarryMass();

private:
	/** Kilograms that count as a fully heavy carry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	float HeavyMassKg = 20.0f;

	/** Walk speed at Heavy Mass Kg, as a fraction of normal. 0.5 is half speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry", meta = (AllowPrivateAccess = "true", ClampMin = "0.1", ClampMax = "1"))
	float SlowestWalkScale = 0.5f;

	/** Pickup time for a weightless object, in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float LightPickupSeconds = 0.05f;

	/** Pickup time at Heavy Mass Kg, in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float HeavyPickupSeconds = 1.2f;

	float SavedMaxWalkSpeed = 0.0f;
	bool bCarrySlowApplied = false;

};

