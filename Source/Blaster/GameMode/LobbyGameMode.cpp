// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"

#include "MultiplayerSessionSubsystem.h"
#include "Blaster/Utils/DebugUtil.h"
#include "GameFramework/GameStateBase.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();

	DebugUtil::PrintMsg(FString::Printf(TEXT("ALobbyGameMode::PostLogin, Current Num Player is %d"), NumberOfPlayers));
	
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)			return;
	UMultiplayerSessionSubsystem* MPSessionSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionSubsystem>();
	if (!MPSessionSubsystem)	return;
	check(MPSessionSubsystem);
	// enter match map when player meets desired connections
	int32 DesiredNumConnection = MPSessionSubsystem->DesiredNumConnections;
	DebugUtil::PrintMsg(FString::Printf(TEXT("ALobbyGameMode:: MPSessionSubsystem->DesiredNumConnections is %d"), DesiredNumConnection));
	if (NumberOfPlayers == MPSessionSubsystem->DesiredNumConnections)
	{
		if (UWorld* World = GetWorld())
		{
			bUseSeamlessTravel = true;
			FString MatchType = MPSessionSubsystem->DesiredMatchType;
			DebugUtil::PrintMsg(FString::Printf(TEXT("Ready to start game. MatchType is %s"), *MatchType));
			if (MatchType.Equals("FreeForAll"))
			{
				World->ServerTravel(FString("/Game/Maps/BlasterMap?listen"));
			} else if (MatchType.Equals("TeamMatch"))
			{
				World->ServerTravel(FString("/Game/Maps/TeamMatch?listen"));
			} else if (MatchType.Equals("CaptureTheFlag"))
			{
				World->ServerTravel(FString("/Game/Maps/CaptureTheFlag?listen"));
			}
		}
	}
	

	

	
	
}
