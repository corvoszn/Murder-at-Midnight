// Copyright Epic Games, Inc. All Rights Reserved.

#include "MurderAtMidnightInteractableComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "MurderAtMidnightCabinet.h"
#include "MurderAtMidnightDoor.h"
#include "MurderAtMidnightCharacter.h"
#include "MurderAtMidnightGameMode.h"
#include "MurderAtMidnightInventoryComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MurderAtMidnightPlayerController.h"
#include "Net/UnrealNetwork.h"

namespace
{
	const FVector CarriedKeyOffset(40.0f, 12.0f, -12.0f);

	void SetShapeColor(UStaticMeshComponent* Mesh, const FLinearColor& Color)
	{
		if (!Mesh)
		{
			return;
		}

		UMaterialInterface* ShapeMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		UMaterialInstanceDynamic* Dynamic = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
		if (!Dynamic || (ShapeMaterial && Dynamic->Parent != ShapeMaterial))
		{
			Dynamic = ShapeMaterial ? UMaterialInstanceDynamic::Create(ShapeMaterial, Mesh) : Mesh->CreateAndSetMaterialInstanceDynamic(0);
			if (Dynamic)
			{
				Mesh->SetMaterial(0, Dynamic);
			}
		}

		if (Dynamic)
		{
			Dynamic->SetVectorParameterValue(TEXT("Color"), Color);
		}
	}
}

UMurderAtMidnightInteractableComponent::UMurderAtMidnightInteractableComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UMurderAtMidnightInteractableComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* OwnerActor = GetOwner();
	if (OwnerActor && !OwnerActor->HasAuthority())
	{
		TInlineComponentArray<UPrimitiveComponent*> Parts;
		OwnerActor->GetComponents(Parts);
		for (UPrimitiveComponent* Part : Parts)
		{
			if (Part && Part->IsSimulatingPhysics())
			{
				Part->SetSimulatePhysics(false);
			}
		}
	}

	KeepPlayersOff();
	ApplyVisual();
}

void UMurderAtMidnightInteractableComponent::KeepPlayersOff() const
{
	if (Kind != EMurderAtMidnightInteractKind::Pickup && Kind != EMurderAtMidnightInteractKind::Weapon)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	TInlineComponentArray<UPrimitiveComponent*> Parts;
	OwnerActor->GetComponents(Parts);
	for (UPrimitiveComponent* Part : Parts)
	{
		if (!Part)
		{
			continue;
		}

		Part->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Part->CanCharacterStepUpOn = ECB_No;
		Part->SetUseCCD(true);
		Part->SetMaxDepenetrationVelocity(NAME_None, 20.0f);
	}
}

void UMurderAtMidnightInteractableComponent::Configure(EMurderAtMidnightInteractKind InKind, FName InItemId, FName InRequiredItemId)
{
	Kind = InKind;
	ItemId = InItemId;
	RequiredItemId = InRequiredItemId;
}

bool UMurderAtMidnightInteractableComponent::ServerInteract(AMurderAtMidnightCharacter* Who, FString& OutMessage)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !Who)
	{
		OutMessage = TEXT("The server refused that.");
		return false;
	}

	UMurderAtMidnightInventoryComponent* Inventory = Who->GetInventory();
	if (!Inventory)
	{
		OutMessage = TEXT("The server refused that.");
		return false;
	}

	if (Kind == EMurderAtMidnightInteractKind::Pickup || Kind == EMurderAtMidnightInteractKind::Weapon)
	{
		if (bCarried)
		{
			OutMessage = TEXT("Someone is already holding that.");
			return false;
		}

		if (!Inventory->TryAddItem(ItemId))
		{
			OutMessage = TEXT("Your hands are full.");
			return false;
		}

		Carrier = Who;
		bCarried = true;
		CarryMass = 0.0f;
		if (UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent()))
		{
			CarryMass = ReadCarryMass(Root);
			TInlineComponentArray<UPrimitiveComponent*> Parts;
			OwnerActor->GetComponents(Parts);
			for (UPrimitiveComponent* Part : Parts)
			{
				CarryMass = FMath::Max(CarryMass, ReadCarryMass(Part));
			}
		}

		const float Delay = Who->GetPickupDelay(CarryMass);
		if (Delay <= 0.05f || !GetWorld())
		{
			StartHold(Who);
		}
		else
		{
			bHoldPending = true;
			HoldPendingUntil = GetWorld()->GetTimeSeconds() + Delay;
			SetComponentTickEnabled(true);
		}

		ApplyVisual();
		OwnerActor->ForceNetUpdate();
		OutMessage = Kind == EMurderAtMidnightInteractKind::Weapon ? TEXT("You picked up the weapon.") : TEXT("You picked up the key.");
		return true;
	}

	if (bOpen)
	{
		if (!Inventory->HasItem(RequiredItemId))
		{
			OutMessage = TEXT("isopen");
			return false;
		}

		bOpen = false;
	}
	else
	{
		if (!Inventory->HasItem(RequiredItemId))
		{
			OutMessage = TEXT("isclosed");
			return false;
		}

		bOpen = true;
	}

	ApplyVisual();
	OwnerActor->ForceNetUpdate();
	OutMessage = bOpen ? TEXT("isopen") : TEXT("isclosed");

	if (UWorld* World = GetWorld())
	{
		if (AMurderAtMidnightGameMode* MatchMode = World->GetAuthGameMode<AMurderAtMidnightGameMode>())
		{
			if (AMurderAtMidnightPlayerController* Controller = Cast<AMurderAtMidnightPlayerController>(Who->GetController()))
			{
				if (OwnerActor->IsA(AMurderAtMidnightCabinet::StaticClass()))
				{
					MatchMode->NotifyCabinetLockChanged(Controller, bOpen);
				}
				else if (bOpen && OwnerActor->IsA(AMurderAtMidnightDoor::StaticClass()))
				{
					MatchMode->NotifyDoorOpened(Controller);
				}
			}
		}
	}

	return true;
}

bool UMurderAtMidnightInteractableComponent::ServerDrop(AMurderAtMidnightCharacter* Who, const FVector& WorldLocation, FString& OutMessage)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !Who || !bCarried || Carrier != Who)
	{
		OutMessage = TEXT("You are not holding anything.");
		return false;
	}

	UMurderAtMidnightInventoryComponent* Inventory = Who->GetInventory();
	if (!Inventory || !Inventory->TryRemoveItem(ItemId))
	{
		OutMessage = TEXT("You are not holding anything.");
		return false;
	}

	if (bHoldPending)
	{
		bHoldPending = false;
		SetComponentTickEnabled(false);
		Carrier = nullptr;
		bCarried = false;
		ApplyVisual();
		OwnerActor->ForceNetUpdate();
		OutMessage = Kind == EMurderAtMidnightInteractKind::Weapon ? TEXT("You dropped the weapon.") : TEXT("You dropped the key.");
		return true;
	}

	const bool bPlaceAtDrop = !bRestorePhysicsOnDrop;
	StopHold(Who);
	Carrier = nullptr;
	bCarried = false;
	if (bPlaceAtDrop)
	{
		ReleaseAt(WorldLocation);
	}
	ApplyVisual();
	OwnerActor->ForceNetUpdate();
	OutMessage = Kind == EMurderAtMidnightInteractKind::Weapon ? TEXT("You dropped the weapon.") : TEXT("You dropped the key.");
	return true;
}

bool UMurderAtMidnightInteractableComponent::ServerThrow(AMurderAtMidnightCharacter* Who, const FVector& AimDirection, float HoldSeconds, FString& OutMessage)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !Who || !bCarried || Carrier != Who)
	{
		OutMessage = TEXT("You are not holding anything.");
		return false;
	}

	if (bHoldPending)
	{
		OutMessage = TEXT("It is not in your hand yet.");
		return false;
	}

	UMurderAtMidnightInventoryComponent* Inventory = Who->GetInventory();
	if (!Inventory || !Inventory->TryRemoveItem(ItemId))
	{
		OutMessage = TEXT("You are not holding anything.");
		return false;
	}

	FVector Direction = AimDirection.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		Direction = Who->GetControlRotation().Vector();
	}

	const float Speed = Who->GetThrowSpeed(CarryMass, HoldSeconds);
	ReleaseThrown(Who, Direction * Speed);
	Carrier = nullptr;
	bCarried = false;
	ApplyVisual();
	OwnerActor->ForceNetUpdate();
	OutMessage = TEXT("You threw it.");
	return true;
}

void UMurderAtMidnightInteractableComponent::ServerMarkBloody()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || Kind != EMurderAtMidnightInteractKind::Weapon)
	{
		return;
	}

	bBloody = true;
	ApplyVisual();
	OwnerActor->ForceNetUpdate();
}

bool UMurderAtMidnightInteractableComponent::ServerGive(AMurderAtMidnightCharacter* Giver, AMurderAtMidnightCharacter* Receiver, FString& OutMessage)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !Giver || !Receiver || Giver == Receiver || !bCarried || Carrier != Giver)
	{
		OutMessage = TEXT("You have nothing to give.");
		return false;
	}

	UMurderAtMidnightInventoryComponent* FromInventory = Giver->GetInventory();
	UMurderAtMidnightInventoryComponent* ToInventory = Receiver->GetInventory();
	if (!FromInventory || !ToInventory || !FromInventory->HasItem(ItemId))
	{
		OutMessage = TEXT("You have nothing to give.");
		return false;
	}

	if (ToInventory->IsFull())
	{
		OutMessage = TEXT("Their hands are full.");
		return false;
	}

	if (!FromInventory->TryRemoveItem(ItemId) || !ToInventory->TryAddItem(ItemId))
	{
		FromInventory->TryAddItem(ItemId);
		OutMessage = TEXT("The server refused that.");
		return false;
	}

	StopHold(Giver);
	Carrier = Receiver;
	bCarried = true;
	StartHold(Receiver);
	ApplyVisual();
	OwnerActor->ForceNetUpdate();
	OutMessage = Kind == EMurderAtMidnightInteractKind::Weapon ? TEXT("You handed over the weapon.") : TEXT("You handed over the key.");
	return true;
}

void UMurderAtMidnightInteractableComponent::AttachToCarrier()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !Carrier)
	{
		return;
	}

	UCameraComponent* Camera = Carrier->GetFirstPersonCameraComponent();
	if (!Camera)
	{
		return;
	}

	const FVector Offset = Kind == EMurderAtMidnightInteractKind::Weapon ? FVector(55.0f, 22.0f, -18.0f) : CarriedKeyOffset;
	OwnerActor->AttachToComponent(Camera, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	OwnerActor->SetActorRelativeLocation(Offset);
	OwnerActor->SetActorRelativeRotation(FRotator::ZeroRotator);
}

void UMurderAtMidnightInteractableComponent::ReleaseAt(const FVector& WorldLocation)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	OwnerActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	OwnerActor->SetActorRotation(FRotator::ZeroRotator);
	MoveClearOfWorld(WorldLocation);
}

void UMurderAtMidnightInteractableComponent::ApplyVisual() const
{
	AActor* OwnerActor = GetOwner();
	UStaticMeshComponent* Mesh = OwnerActor ? OwnerActor->FindComponentByClass<UStaticMeshComponent>() : nullptr;
	if (!Mesh)
	{
		return;
	}

	if (Kind == EMurderAtMidnightInteractKind::Pickup || Kind == EMurderAtMidnightInteractKind::Weapon)
	{
		if (Kind == EMurderAtMidnightInteractKind::Weapon)
		{
			SetShapeColor(Mesh, bBloody ? FLinearColor(0.85f, 0.02f, 0.02f) : FLinearColor(0.22f, 0.22f, 0.24f));
		}
		else
		{
			Mesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.85f, 0.62f, 0.12f));
		}
		return;
	}

	const FVector Color = FVector(0.35f, 0.16f, 0.1f);
	Mesh->SetVectorParameterValueOnMaterials(TEXT("Color"), Color);
}

UPrimitiveComponent* UMurderAtMidnightInteractableComponent::FindHoldBody() const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return nullptr;
	}

	if (UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent()))
	{
		return Root;
	}

	return OwnerActor->FindComponentByClass<UPrimitiveComponent>();
}

FVector UMurderAtMidnightInteractableComponent::GetHoldPoint() const
{
	if (!Carrier)
	{
		return FVector::ZeroVector;
	}

	const UCameraComponent* Camera = Carrier->GetFirstPersonCameraComponent();
	if (!Camera)
	{
		return Carrier->GetActorLocation();
	}

	return Camera->GetComponentLocation()
		+ Camera->GetForwardVector() * HoldDistance
		+ Camera->GetUpVector() * HoldHeight;
}

FRotator UMurderAtMidnightInteractableComponent::GetHoldRotation() const
{
	AActor* OwnerActor = GetOwner();
	const UCameraComponent* Camera = Carrier ? Carrier->GetFirstPersonCameraComponent() : nullptr;
	if (!Camera || !OwnerActor)
	{
		return OwnerActor ? OwnerActor->GetActorRotation() : FRotator::ZeroRotator;
	}

	return (Camera->GetComponentQuat() * HoldRelativeRotation.Quaternion()).Rotator();
}

float UMurderAtMidnightInteractableComponent::ReadCarryMass(UPrimitiveComponent* Body) const
{
	if (!Body)
	{
		return 0.0f;
	}

	if (!Body->IsSimulatingPhysics() && !Body->BodyInstance.bOverrideMass)
	{
		return 0.0f;
	}

	return FMath::Max(0.0f, Body->GetMass());
}

void UMurderAtMidnightInteractableComponent::CancelPendingPickup(AMurderAtMidnightCharacter* Who, const FString& Message)
{
	bHoldPending = false;
	SetComponentTickEnabled(false);

	if (Who)
	{
		if (UMurderAtMidnightInventoryComponent* Inventory = Who->GetInventory())
		{
			Inventory->TryRemoveItem(ItemId);
		}
		if (AMurderAtMidnightPlayerController* Controller = Cast<AMurderAtMidnightPlayerController>(Who->GetController()))
		{
			Controller->ClientInteractMessage(Message);
		}
	}

	Carrier = nullptr;
	bCarried = false;
	if (AActor* OwnerActor = GetOwner())
	{
		ApplyVisual();
		OwnerActor->ForceNetUpdate();
	}
}

void UMurderAtMidnightInteractableComponent::MoveClearOfWorld(const FVector& Desired)
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	UPrimitiveComponent* Root = OwnerActor ? Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent()) : nullptr;
	if (!OwnerActor || !World || !Root)
	{
		return;
	}

	const FVector Safe = Carrier ? Carrier->GetPawnViewLocation() : Desired;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HoldClear), false, OwnerActor);
	if (Carrier)
	{
		Params.AddIgnoredActor(Carrier);
	}

	FHitResult Hit;
	const FCollisionShape Shape = Root->GetCollisionShape();
	const FQuat Rotation = OwnerActor->GetActorQuat();
	bool bHit = World->SweepSingleByChannel(Hit, Safe, Desired, Rotation, ECC_WorldStatic, Shape, Params);
	if (bHit && Hit.bStartPenetrating)
	{
		bHit = World->SweepSingleByChannel(Hit, Safe + FVector(0.0f, 0.0f, 80.0f), Desired, Rotation, ECC_WorldStatic, Shape, Params);
	}

	FVector Rest = Desired;
	if (bHit && Hit.bBlockingHit && !Hit.bStartPenetrating)
	{
		Rest = Hit.Location + Hit.Normal * 4.0f;
	}
	else if (bHit && Hit.bStartPenetrating)
	{
		Rest = Safe + FVector(0.0f, 0.0f, 80.0f);
	}

	OwnerActor->SetActorLocation(Rest, false, nullptr, ETeleportType::TeleportPhysics);
}

void UMurderAtMidnightInteractableComponent::LeaveTheHolder(AMurderAtMidnightCharacter* Who)
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	UPrimitiveComponent* Root = OwnerActor ? Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent()) : nullptr;
	if (!Who || !OwnerActor || !World || !Root)
	{
		return;
	}

	const FVector Forward = Who->GetActorForwardVector().GetSafeNormal2D();
	const FVector Dir = Forward.IsNearlyZero() ? FVector::ForwardVector : Forward;
	const UCapsuleComponent* Capsule = Who->GetCapsuleComponent();
	const float Clearance = (Capsule ? Capsule->GetScaledCapsuleRadius() : 40.0f) + 28.0f;

	FVector Location = OwnerActor->GetActorLocation();
	const FVector Flat = FVector(Location.X, Location.Y, Who->GetActorLocation().Z);
	const FVector Offset = Flat - Who->GetActorLocation();
	if (Offset.Size2D() < Clearance)
	{
		Location = Who->GetActorLocation() + Dir * Clearance;
		Location.Z = OwnerActor->GetActorLocation().Z;
	}

	OwnerActor->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
}

bool UMurderAtMidnightInteractableComponent::IsClearOfHolder(const AActor* OwnerActor) const
{
	const AMurderAtMidnightCharacter* Who = IgnoredHolder.Get();
	const UCapsuleComponent* Capsule = Who ? Who->GetCapsuleComponent() : nullptr;
	if (!OwnerActor || !Who || !Capsule)
	{
		return true;
	}

	const FVector Delta = OwnerActor->GetActorLocation() - Who->GetActorLocation();
	if (FMath::Abs(Delta.Z) > Capsule->GetScaledCapsuleHalfHeight() + 20.0f)
	{
		return true;
	}

	return Delta.Size2D() > Capsule->GetScaledCapsuleRadius() + 24.0f;
}

void UMurderAtMidnightInteractableComponent::PlaceAtHold()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !Carrier)
	{
		return;
	}

	const FVector Target = GetHoldPoint();
	const FRotator Rotation = GetHoldRotation();
	OwnerActor->SetActorRotation(Rotation);

	FHitResult Hit;
	OwnerActor->SetActorLocation(Target, true, &Hit, ETeleportType::TeleportPhysics);
	if (Hit.bStartPenetrating)
	{
		MoveClearOfWorld(Target);
		return;
	}

	if (Hit.bBlockingHit && OwnerActor->HasAuthority())
	{
		if (UPrimitiveComponent* Other = Hit.GetComponent())
		{
			if (Other->IsSimulatingPhysics() && BumpForce > 0.0f)
			{
				const FVector Push = (Target - OwnerActor->GetActorLocation()).GetSafeNormal();
				Other->AddForce(Push * BumpForce);
			}
		}
	}
}

void UMurderAtMidnightInteractableComponent::IgnoreHolder(AMurderAtMidnightCharacter* Who)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !Who)
	{
		return;
	}

	RestoreHolder();

	TInlineComponentArray<UPrimitiveComponent*> Parts;
	OwnerActor->GetComponents(Parts);
	for (UPrimitiveComponent* Part : Parts)
	{
		if (!Part)
		{
			continue;
		}

		IgnoredParts.Add(Part);
		SavedPawnResponses.Add(Part->GetCollisionResponseToChannel(ECC_Pawn));
		Part->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Part->IgnoreActorWhenMoving(Who, true);
	}

	if (UCapsuleComponent* Capsule = Who->GetCapsuleComponent())
	{
		Capsule->IgnoreActorWhenMoving(OwnerActor, true);
	}

	IgnoredHolder = Who;
}

void UMurderAtMidnightInteractableComponent::RestoreHolder()
{
	AActor* OwnerActor = GetOwner();
	AMurderAtMidnightCharacter* Who = IgnoredHolder.Get();
	const int32 Count = FMath::Min(IgnoredParts.Num(), SavedPawnResponses.Num());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (UPrimitiveComponent* Part = IgnoredParts[Index])
		{
			if (Who)
			{
				Part->IgnoreActorWhenMoving(Who, false);
			}
		}
	}
	IgnoredParts.Reset();
	SavedPawnResponses.Reset();

	if (Who && OwnerActor)
	{
		if (UCapsuleComponent* Capsule = Who->GetCapsuleComponent())
		{
			Capsule->IgnoreActorWhenMoving(OwnerActor, false);
		}
	}

	IgnoredHolder = nullptr;
	KeepPlayersOff();
}

void UMurderAtMidnightInteractableComponent::StartHold(AMurderAtMidnightCharacter* Who)
{
	AActor* OwnerActor = GetOwner();
	UPrimitiveComponent* Root = OwnerActor ? Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent()) : nullptr;
	if (!OwnerActor || !Who || !Root)
	{
		return;
	}

	bHoldPending = false;
	bSettlingDrop = false;
	OwnerActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	TInlineComponentArray<UPrimitiveComponent*> Parts;
	OwnerActor->GetComponents(Parts);
	UPrimitiveComponent* SimulatingChild = nullptr;
	const bool bRootWasSimulating = Root->IsSimulatingPhysics();
	for (UPrimitiveComponent* Part : Parts)
	{
		if (!Part || Part == Root || !Part->IsSimulatingPhysics())
		{
			continue;
		}

		if (!SimulatingChild || Part->IsA<UStaticMeshComponent>())
		{
			SimulatingChild = Part;
		}
	}

	// A loose mesh under the root is a second body. Stop it simulating, and keep the collision the user set.
	if (SimulatingChild)
	{
		const FTransform Relative = SimulatingChild->GetRelativeTransform();
		const FTransform World = SimulatingChild->GetComponentTransform();
		for (UPrimitiveComponent* Part : Parts)
		{
			if (Part && Part != Root && Part->IsSimulatingPhysics())
			{
				Part->SetPhysicsLinearVelocity(FVector::ZeroVector);
				Part->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
				Part->SetSimulatePhysics(false);
			}
		}

		OwnerActor->SetActorTransform(World * Relative.Inverse(), false, nullptr, ETeleportType::TeleportPhysics);
	}

	bRestorePhysicsOnDrop = bRootWasSimulating || SimulatingChild != nullptr;
	CarryMass = FMath::Max(CarryMass, ReadCarryMass(SimulatingChild));
	CarryMass = FMath::Max(CarryMass, ReadCarryMass(Root));

	for (UPrimitiveComponent* Part : Parts)
	{
		if (Part && Part->IsSimulatingPhysics())
		{
			Part->SetPhysicsLinearVelocity(FVector::ZeroVector);
			Part->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			Part->SetSimulatePhysics(false);
		}
	}

	IgnoreHolder(Who);

	if (const UCameraComponent* Camera = Who->GetFirstPersonCameraComponent())
	{
		HoldRelativeRotation = (Camera->GetComponentQuat().Inverse() * OwnerActor->GetActorQuat()).Rotator();
	}

	Who->ApplyCarryMass(CarryMass);
	bHeldInView = true;
	PlaceAtHold();
	SetComponentTickEnabled(true);
	OwnerActor->ForceNetUpdate();
}

void UMurderAtMidnightInteractableComponent::StopHold(AMurderAtMidnightCharacter* Who)
{
	bHoldPending = false;
	bHeldInView = false;

	if (!IgnoredHolder.IsValid())
	{
		IgnoredHolder = Who;
	}

	if (Who)
	{
		Who->ClearCarryMass();
	}

	if (AActor* OwnerActor = GetOwner())
	{
		MoveClearOfWorld(OwnerActor->GetActorLocation());
		LeaveTheHolder(Who);
	}

	UPrimitiveComponent* Body = FindHoldBody();

	if (bRestorePhysicsOnDrop && Body)
	{
		Body->SetMaxDepenetrationVelocity(NAME_None, 40.0f);
		if (FBodyInstance* Instance = Body->GetBodyInstance())
		{
			Instance->SetBodyTransform(Body->GetComponentTransform(), ETeleportType::TeleportPhysics);
			Instance->SetLinearVelocity(FVector::ZeroVector, false);
			Instance->SetAngularVelocityInRadians(FVector::ZeroVector, false);
		}
		Body->SetSimulatePhysics(true);
		StopCarriedMotion(Body);
		bRestorePhysicsOnDrop = false;
		bSettlingDrop = true;
		SettleFrames = 0;
		if (AActor* OwnerActor = GetOwner())
		{
			SettledDropLocation = OwnerActor->GetActorLocation();
		}
		SetComponentTickEnabled(true);
		return;
	}

	RestoreHolder();
	SetComponentTickEnabled(false);
}

void UMurderAtMidnightInteractableComponent::ReleaseThrown(AMurderAtMidnightCharacter* Who, const FVector& Velocity)
{
	bHoldPending = false;
	bHeldInView = false;
	bSettlingDrop = false;

	if (!IgnoredHolder.IsValid())
	{
		IgnoredHolder = Who;
	}

	if (Who)
	{
		Who->ClearCarryMass();
	}

	if (AActor* OwnerActor = GetOwner())
	{
		MoveClearOfWorld(OwnerActor->GetActorLocation());
		LeaveTheHolder(Who);
	}

	UPrimitiveComponent* Body = FindHoldBody();
	if (bRestorePhysicsOnDrop && Body)
	{
		Body->SetSimulatePhysics(true);
		Body->SetPhysicsLinearVelocity(Velocity);
		Body->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		if (FBodyInstance* Instance = Body->GetBodyInstance())
		{
			Instance->SetLinearVelocity(Velocity, false);
			Instance->SetAngularVelocityInRadians(FVector::ZeroVector, false);
		}
		Body->WakeAllRigidBodies();
		bRestorePhysicsOnDrop = false;
	}

	RestoreHolder();
	SetComponentTickEnabled(false);
}

void UMurderAtMidnightInteractableComponent::StopCarriedMotion(UPrimitiveComponent* Body) const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	TInlineComponentArray<UPrimitiveComponent*> Parts;
	OwnerActor->GetComponents(Parts);
	for (UPrimitiveComponent* Part : Parts)
	{
		if (!Part)
		{
			continue;
		}

		Part->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Part->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}

	if (Body)
	{
		Body->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Body->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}
}

void UMurderAtMidnightInteractableComponent::FinishSettledDrop(AActor* OwnerActor)
{
	if (OwnerActor && !IsClearOfHolder(OwnerActor))
	{
		LeaveTheHolder(IgnoredHolder.Get());
	}

	StopCarriedMotion(FindHoldBody());
	RestoreHolder();
	bSettlingDrop = false;
	SettleFrames = 0;
	SetComponentTickEnabled(false);
}

void UMurderAtMidnightInteractableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	if (!OwnerActor->HasAuthority())
	{
		if (bHeldInView && Carrier && Carrier->IsLocallyControlled())
		{
			PlaceAtHold();
		}
		return;
	}

	if (bSettlingDrop)
	{
		if (SettleFrames < 2)
		{
			OwnerActor->SetActorLocation(SettledDropLocation, false, nullptr, ETeleportType::TeleportPhysics);
			StopCarriedMotion(FindHoldBody());
		}
		else if (UPrimitiveComponent* Body = FindHoldBody())
		{
			if (Body->GetPhysicsLinearVelocity().SizeSquared() > FMath::Square(250.0f))
			{
				StopCarriedMotion(Body);
			}
		}

		SettleFrames++;
		if ((SettleFrames >= 3 && IsClearOfHolder(OwnerActor)) || SettleFrames > 45)
		{
			FinishSettledDrop(OwnerActor);
		}
		return;
	}

	if (!bCarried || !Carrier)
	{
		return;
	}

	if (bHoldPending)
	{
		if (FVector::Dist(Carrier->GetActorLocation(), OwnerActor->GetActorLocation()) > 350.0f)
		{
			CancelPendingPickup(Carrier, TEXT("You left it behind."));
			return;
		}

		if (GetWorld() && GetWorld()->GetTimeSeconds() >= HoldPendingUntil)
		{
			bHoldPending = false;
			StartHold(Carrier);
		}
		return;
	}

	PlaceAtHold();
}

void UMurderAtMidnightInteractableComponent::OnRep_State()
{
	if (!bCarried)
	{
		if (AActor* OwnerActor = GetOwner())
		{
			OwnerActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}
	}

	const bool bFollowLocally = bHeldInView && Carrier && Carrier->IsLocallyControlled();
	SetComponentTickEnabled(bFollowLocally);
	ApplyVisual();
}

void UMurderAtMidnightInteractableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMurderAtMidnightInteractableComponent, bCarried);
	DOREPLIFETIME(UMurderAtMidnightInteractableComponent, bOpen);
	DOREPLIFETIME(UMurderAtMidnightInteractableComponent, bBloody);
	DOREPLIFETIME(UMurderAtMidnightInteractableComponent, Carrier);
	DOREPLIFETIME(UMurderAtMidnightInteractableComponent, bHeldInView);
	DOREPLIFETIME(UMurderAtMidnightInteractableComponent, HoldRelativeRotation);
}
