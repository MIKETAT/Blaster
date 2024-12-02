// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"

#include "MultiplayerSessionSubsystem.h"
#include "GameFramework/GameStateBase.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)			return;
	UMultiplayerSessionSubsystem* MPSessionSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionSubsystem>();
	if (!MPSessionSubsystem)	return;
	check(MPSessionSubsystem);
	// enter match map when player meets desired connections
	if (NumberOfPlayers == MPSessionSubsystem->DesiredNumConnections)
	{
		if (UWorld* World = GetWorld())
		{
			bUseSeamlessTravel = true;
			FString MatchType = MPSessionSubsystem->DesiredMatchType;
			if (MatchType.Equals("FreeForAll"))
			{
				World->ServerTravel(FString("/Game/Maps/CaptureTheFlag?listen"));
			} else if (MatchType.Equals("TeamMatch"))
			{
				World->ServerTravel(FString("/Game/Maps/CaptureTheFlag?listen"));
			} else if (MatchType.Equals("CaptureTheFlag"))
			{
				World->ServerTravel(FString("/Game/Maps/CaptureTheFlag?listen"));
			}
		}
	}
	

	

	
	
}
