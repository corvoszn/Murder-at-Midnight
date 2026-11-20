// Copyright Epic Games, Inc. All Rights Reserved.

#include "MurderAtMidnightOnlineSubsystem.h"

#include "EOSShared.h"
#include "IEOSSDKManager.h"
#include "IOnlineSubsystemEOS.h"
#include "OnlineSubsystemEOSTypesPublic.h"
#include "MurderAtMidnight.h"
#include "MurderAtMidnightPlayerController.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Engine/Engine.h"
#include "eos_sdk.h"

namespace
{
	const TCHAR* ListenMap = TEXT("/Game/FirstPerson/Lvl_FirstPerson?listen");
	const FString LobbyKeyword = TEXT("MurderAtMidnight");
	const FString LobbyBucket = TEXT("MurderAtMidnight");

	void EOS_CALL OnCreateDeviceIdCallback(const EOS_Connect_CreateDeviceIdCallbackInfo* Data)
	{
		UObject* Object = Data ? static_cast<UObject*>(Data->ClientData) : nullptr;
		if (UMurderAtMidnightOnlineSubsystem* Subsystem = Cast<UMurderAtMidnightOnlineSubsystem>(Object))
		{
			Subsystem->OnDeviceIdReady();
		}
	}
}

void UMurderAtMidnightOnlineSubsystem::Host()
{
	Pending = EPendingAction::Host;
	BeginLogin();
}

void UMurderAtMidnightOnlineSubsystem::Join()
{
	Pending = EPendingAction::Join;
	BeginLogin();
}

IOnlineSubsystem* UMurderAtMidnightOnlineSubsystem::GetOnline() const
{
	return Online::GetSubsystem(GetWorld(), TEXT("EOS"));
}

IOnlineSessionPtr UMurderAtMidnightOnlineSubsystem::GetSessions() const
{
	IOnlineSubsystem* Subsystem = GetOnline();
	return Subsystem ? Subsystem->GetSessionInterface() : nullptr;
}

void UMurderAtMidnightOnlineSubsystem::TellLocalPlayer(const FString& Line)
{
	UE_LOG(LogMurderAtMidnight, Display, TEXT("%s"), *Line);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(100, 8.f, FColor::White, Line);
	}

	UGameInstance* Instance = GetGameInstance();
	UWorld* World = Instance ? Instance->GetWorld() : nullptr;
	AMurderAtMidnightPlayerController* Controller = World
		? Cast<AMurderAtMidnightPlayerController>(World->GetFirstPlayerController())
		: nullptr;
	if (Controller && Controller->IsLocalController())
	{
		Controller->ShowLocalStatus(Line);
	}
}

void UMurderAtMidnightOnlineSubsystem::BindDelegates()
{
	if (bDelegatesBound)
	{
		return;
	}

	IOnlineSessionPtr Sessions = GetSessions();
	if (!Sessions.IsValid())
	{
		return;
	}

	CreateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UMurderAtMidnightOnlineSubsystem::OnCreateSessionComplete));
	FindHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UMurderAtMidnightOnlineSubsystem::OnFindSessionsComplete));
	JoinHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UMurderAtMidnightOnlineSubsystem::OnJoinSessionComplete));
	DestroyHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UMurderAtMidnightOnlineSubsystem::OnDestroySessionComplete));
	bDelegatesBound = true;
}

void UMurderAtMidnightOnlineSubsystem::BeginLogin()
{
	IOnlineSubsystem* Subsystem = GetOnline();
	IOnlineIdentityPtr Identity = Subsystem ? Subsystem->GetIdentityInterface() : nullptr;
	if (!Identity.IsValid())
	{
		Pending = EPendingAction::None;
		TellLocalPlayer(TEXT("EOS is not ready."));
		return;
	}

	if (Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn)
	{
		StartPending();
		return;
	}

	if (bLoginInFlight || bWaitingForDevice)
	{
		return;
	}

	TellLocalPlayer(TEXT("Signing in."));

	IOnlineSubsystemEOS* Eos = static_cast<IOnlineSubsystemEOS*>(Subsystem);
	const IEOSPlatformHandlePtr Platform = Eos ? Eos->GetEOSPlatformHandle() : nullptr;
	const EOS_HPlatform PlatformHandle = Platform.IsValid() ? static_cast<EOS_HPlatform>(*Platform) : nullptr;
	EOS_HConnect Connect = PlatformHandle ? EOS_Platform_GetConnectInterface(PlatformHandle) : nullptr;
	if (!Connect)
	{
		Pending = EPendingAction::None;
		TellLocalPlayer(TEXT("EOS is not ready."));
		return;
	}

	EOS_Connect_CreateDeviceIdOptions Options = {};
	Options.ApiVersion = EOS_CONNECT_CREATEDEVICEID_API_LATEST;
	Options.DeviceModel = "PC Windows";
	bWaitingForDevice = true;
	EOS_Connect_CreateDeviceId(Connect, &Options, this, &OnCreateDeviceIdCallback);
}

void UMurderAtMidnightOnlineSubsystem::OnDeviceIdReady()
{
	bWaitingForDevice = false;

	IOnlineSubsystem* Subsystem = GetOnline();
	IOnlineIdentityPtr Identity = Subsystem ? Subsystem->GetIdentityInterface() : nullptr;
	if (!Identity.IsValid() || Pending == EPendingAction::None)
	{
		Pending = EPendingAction::None;
		TellLocalPlayer(TEXT("EOS is not ready."));
		return;
	}

	if (Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn)
	{
		StartPending();
		return;
	}

	FOnlineAccountCredentials Credentials;
	Credentials.Type = TEXT("device:DeviceIdAccessToken");
	Credentials.Id = TEXT("Player");
	bLoginInFlight = true;
	LoginHandle = Identity->AddOnLoginCompleteDelegate_Handle(
		0, FOnLoginCompleteDelegate::CreateUObject(this, &UMurderAtMidnightOnlineSubsystem::OnLoginComplete));
	Identity->Login(0, Credentials);
}

void UMurderAtMidnightOnlineSubsystem::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	(void)UserId;
	bLoginInFlight = false;

	if (IOnlineSubsystem* Subsystem = GetOnline())
	{
		if (IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface())
		{
			Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginHandle);
		}
	}

	if (!bWasSuccessful)
	{
		Pending = EPendingAction::None;
		TellLocalPlayer(Error.IsEmpty() ? TEXT("Sign in failed.") : Error);
		return;
	}

	StartPending();
}

void UMurderAtMidnightOnlineSubsystem::StartPending()
{
	BindDelegates();

	IOnlineSessionPtr Sessions = GetSessions();
	if (!Sessions.IsValid())
	{
		Pending = EPendingAction::None;
		TellLocalPlayer(TEXT("EOS sessions are not ready."));
		return;
	}

	if (Sessions->GetNamedSession(NAME_GameSession))
	{
		bDestroyThenContinue = true;
		Sessions->DestroySession(NAME_GameSession);
		return;
	}

	if (Pending == EPendingAction::Host)
	{
		CreateLobby();
	}
	else if (Pending == EPendingAction::Join)
	{
		FindLobby();
	}
}

void UMurderAtMidnightOnlineSubsystem::CreateLobby()
{
	IOnlineSessionPtr Sessions = GetSessions();
	if (!Sessions.IsValid())
	{
		Pending = EPendingAction::None;
		TellLocalPlayer(TEXT("EOS sessions are not ready."));
		return;
	}

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = 8;
	Settings.bIsLANMatch = false;
	Settings.bIsDedicated = false;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bAllowJoinViaPresence = false;
	Settings.bUsesPresence = false;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.bUseLobbiesVoiceChatIfAvailable = true;
	Settings.Set(OSSEOS_BUCKET_ID_ATTRIBUTE_KEY, LobbyBucket, EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(SEARCH_KEYWORDS, LobbyKeyword, EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(SETTING_MAPNAME, FString(TEXT("/Game/FirstPerson/Lvl_FirstPerson")), EOnlineDataAdvertisementType::ViaOnlineService);

	TellLocalPlayer(TEXT("Hosting."));
	if (!Sessions->CreateSession(0, NAME_GameSession, Settings))
	{
		Pending = EPendingAction::None;
		TellLocalPlayer(TEXT("The lobby could not be created."));
	}
}

void UMurderAtMidnightOnlineSubsystem::FindLobby()
{
	IOnlineSessionPtr Sessions = GetSessions();
	if (!Sessions.IsValid())
	{
		Pending = EPendingAction::None;
		TellLocalPlayer(TEXT("EOS sessions are not ready."));
		return;
	}

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = 20;
	SessionSearch->bIsLanQuery = false;
	SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	SessionSearch->QuerySettings.Set(OSSEOS_BUCKET_ID_ATTRIBUTE_KEY, LobbyBucket, EOnlineComparisonOp::Equals);
	SessionSearch->QuerySettings.Set(SEARCH_KEYWORDS, LobbyKeyword, EOnlineComparisonOp::Equals);

	TellLocalPlayer(TEXT("Looking for a host."));
	if (!Sessions->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		Pending = EPendingAction::None;
		TellLocalPlayer(TEXT("The lobby search failed."));
	}
}

void UMurderAtMidnightOnlineSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (Pending != EPendingAction::Host || SessionName != NAME_GameSession)
	{
		return;
	}

	if (!bWasSuccessful)
	{
		Pending = EPendingAction::None;
		TellLocalPlayer(TEXT("The lobby could not be created."));
		return;
	}

	Pending = EPendingAction::None;
	TravelHost();
}

void UMurderAtMidnightOnlineSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (Pending != EPendingAction::Join)
	{
		return;
	}

	if (!bWasSuccessful || !SessionSearch.IsValid() || SessionSearch->SearchResults.Num() == 0)
	{
		Pending = EPendingAction::None;
		TellLocalPlayer(TEXT("No hosted game found."));
		return;
	}

	IOnlineSessionPtr Sessions = GetSessions();
	if (!Sessions.IsValid())
	{
		Pending = EPendingAction::None;
		TellLocalPlayer(TEXT("EOS sessions are not ready."));
		return;
	}

	FOnlineSessionSearchResult Result = SessionSearch->SearchResults[0];
	Result.Session.SessionSettings.bUsesPresence = false;
	Result.Session.SessionSettings.bUseLobbiesIfAvailable = true;
	if (!Sessions->JoinSession(0, NAME_GameSession, Result))
	{
		Pending = EPendingAction::None;
		TellLocalPlayer(TEXT("Could not join the lobby."));
	}
}

void UMurderAtMidnightOnlineSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (Pending != EPendingAction::Join || SessionName != NAME_GameSession)
	{
		return;
	}

	Pending = EPendingAction::None;
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		TellLocalPlayer(TEXT("Could not join the lobby."));
		return;
	}

	TravelToSession();
}

void UMurderAtMidnightOnlineSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (!bDestroyThenContinue || SessionName != NAME_GameSession)
	{
		return;
	}

	bDestroyThenContinue = false;
	if (!bWasSuccessful)
	{
		Pending = EPendingAction::None;
		TellLocalPlayer(TEXT("The previous lobby could not be closed."));
		return;
	}

	StartPending();
}

void UMurderAtMidnightOnlineSubsystem::TravelHost()
{
	UGameInstance* Instance = GetGameInstance();
	UWorld* World = Instance ? Instance->GetWorld() : nullptr;
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	if (Controller && Controller->IsLocalController())
	{
		Controller->ClientTravel(ListenMap, TRAVEL_Absolute);
	}
}

void UMurderAtMidnightOnlineSubsystem::TravelToSession()
{
	IOnlineSessionPtr Sessions = GetSessions();
	FString ConnectString;
	if (!Sessions.IsValid() || !Sessions->GetResolvedConnectString(NAME_GameSession, ConnectString))
	{
		TellLocalPlayer(TEXT("The host has no address yet."));
		return;
	}

	UGameInstance* Instance = GetGameInstance();
	UWorld* World = Instance ? Instance->GetWorld() : nullptr;
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	if (Controller && Controller->IsLocalController())
	{
		Controller->ClientTravel(ConnectString, TRAVEL_Absolute);
	}
}
