// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "GameFramework/PlayerController.h"
#include "MurderAtMidnightPlayerController.generated.h"

class UInputMappingContext;
class UMurderAtMidnightPhaseWidget;
class UUserWidget;

/**
 *  First person Player Controller.
 *  Manages the input mapping context and the camera manager.
 *  Private match messages will be sent to this controller only.
 */
UCLASS(abstract, config="Game")
class MURDERATMIDNIGHT_API AMurderAtMidnightPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:

	/** Constructor */
	AMurderAtMidnightPlayerController();

	/** Reload this map as a listen server. Run from the local console: HostMatch */
	UFUNCTION(Exec)
	void HostMatch();

	/** Connect to a listen server. Run from the local console: JoinMatch 127.0.0.1 */
	UFUNCTION(Exec)
	void JoinMatch(const FString& Address);

	virtual void ReceivedPlayer() override;

	/** Local input. Asks the server to use whatever this player is looking at. */
	void RequestInteract();

	/** Local input. Asks the server to drop a carried key. */
	void RequestDrop();

	/** Local input. Asks the server to attack the player being looked at. */
	void RequestKill();

	/** Local input. Asks the server to call a vote. */
	void RequestVote();

	/** Local input. Asks the server to skip to the next evening stage. Testing only. */
	void RequestSkipPhase();

	/** Local input. Asks the server to name the living player being looked at. */
	void RequestCastVote();

	const FString& GetVoteChoice() const { return VoteChoice; }

	const FString& GetLastInteractMessage() const { return LastInteractMessage; }

	bool HasAgenda() const { return !PrivateAgenda.IsEmpty(); }
	bool IsAgendaComplete() const { return bAgendaComplete; }
	const FString& GetPrivateAgenda() const { return PrivateAgenda; }
	bool HasRole() const { return bRoleReceived; }
	bool IsKiller() const { return bIsKiller; }
	const FString& GetTargetNames() const { return TargetNames; }

	/** This player's agenda sentence, and whether it is done. Sent only to this controller. */
	UFUNCTION(Client, Reliable)
	void ClientReceiveAgendaState(const FString& AgendaText, bool bInComplete);

	/** This player's role. Sent only to this controller. Target names are included for the killer. */
	UFUNCTION(Client, Reliable)
	void ClientReceiveRole(bool bInIsKiller, const FString& InTargetNames);

	/** Result of this player's last request. Sent only to this controller. */
	UFUNCTION(Client, Reliable)
	void ClientInteractMessage(const FString& Message);

	/** This player's own vote line. Sent only to this controller. */
	UFUNCTION(Client, Reliable)
	void ClientReceiveVoteChoice(const FString& Line);

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	void ShowPhaseWidget();

	/** Widget Blueprint that draws the match text. Empty shows no match text. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMurderAtMidnightPhaseWidget> PhaseWidgetClass;

	/** Phase and countdown. Created for the local player only. */
	UPROPERTY()
	TObjectPtr<UMurderAtMidnightPhaseWidget> PhaseWidget;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	UFUNCTION(Server, Reliable)
	void ServerRequestInteract(FVector ViewStart, FVector ViewAim);

	UFUNCTION(Server, Reliable)
	void ServerRequestDrop(FVector ViewAim);

	UFUNCTION(Server, Reliable)
	void ServerRequestKill(FVector ViewStart, FVector ViewAim);

	UFUNCTION(Server, Reliable)
	void ServerRequestVote();

	UFUNCTION(Server, Reliable)
	void ServerRequestSkipPhase();

	UFUNCTION(Server, Reliable)
	void ServerRequestCastVote(FVector ViewStart, FVector ViewAim);

	/** Asks the server to repeat this player's own agenda. The client does not choose the text. */
	UFUNCTION(Server, Reliable)
	void ServerRequestPrivateAgenda();

	bool TraceAlongView(const FVector& ViewStart, const FVector& ViewAim, FHitResult& OutHit, float MaxDistance = 400.0f);

	bool IsDead() const;

	/** Not replicated. Filled by ClientInteractMessage. */
	FString LastInteractMessage;

	/** Not replicated. Filled by ClientReceiveAgendaState. */
	FString PrivateAgenda;

	/** Not replicated. Filled by ClientReceiveAgendaState. */
	bool bAgendaComplete = false;

	/** Not replicated. Filled by ClientReceiveRole. */
	bool bRoleReceived = false;

	/** Not replicated. Filled by ClientReceiveRole. */
	bool bIsKiller = false;

	/** Not replicated. Killer target names. Filled by ClientReceiveRole. */
	FString TargetNames;

	/** Not replicated. Filled by ClientReceiveVoteChoice. */
	FString VoteChoice;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;
};
