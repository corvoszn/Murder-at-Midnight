// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MurderAtMidnightOnlineSubsystem.generated.h"

class FUniqueNetId;
class IOnlineSubsystem;

/**
 * Signs this machine into EOS and hosts or joins one lobby.
 * Lives for the whole time the game is open. It stores no match secrets.
 */
UCLASS()
class MURDERATMIDNIGHT_API UMurderAtMidnightOnlineSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Sign in, create an eight-player lobby, and open the map as a listen server. */
	void Host();

	/** Sign in, find a hosted lobby, and travel to it. */
	void Join();

	/** EOS finished creating the local device id. */
	void OnDeviceIdReady();

private:
	enum class EPendingAction : uint8
	{
		None,
		Host,
		Join
	};

	void BeginLogin();
	void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
	void StartPending();
	void CreateLobby();
	void FindLobby();
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void TravelHost();
	void TravelToSession();
	void TellLocalPlayer(const FString& Line);
	void BindDelegates();
	IOnlineSubsystem* GetOnline() const;
	IOnlineSessionPtr GetSessions() const;

	EPendingAction Pending = EPendingAction::None;
	bool bDelegatesBound = false;
	bool bLoginInFlight = false;
	bool bWaitingForDevice = false;
	bool bDestroyThenContinue = false;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	FDelegateHandle LoginHandle;
	FDelegateHandle CreateHandle;
	FDelegateHandle FindHandle;
	FDelegateHandle JoinHandle;
	FDelegateHandle DestroyHandle;
};
