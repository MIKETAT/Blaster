// Fill out your copyright notice in the Description page of Project Settings.


#include "TeamsGameMode.h"

#include "Blaster/BlasterGameState.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/Utils/DebugUtil.h"
#include "Kismet/GameplayStatics.h"

ATeamsGameMode::ATeamsGameMode()
{
	bTeamMatch = true;
}

void ATeamsGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	ABlasterGameState* BGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	if (!BGameState)	return;
	ABlasterPlayerState* BPState = Exiting->GetPlayerState<ABlasterPlayerState>();
	if (!BPState)		return;
	if (BPState->GetTeam() == ETeam::ET_TeamRed)
	{
		BGameState->RedTeam.Remove(BPState);
	} else if (BPState->GetTeam() == ETeam::ET_TeamBlue)
	{
		BGameState->BlueTeam.Remove(BPState);
	}
}

void ATeamsGameMode::HandleMatchHasStarted()
{
	DebugUtil::PrintMsg(FString::Printf(TEXT("ATeamsGameMode, HandleMatchHasStarted")), FColor::Green);
	Super::HandleMatchHasStarted();
	ABlasterGameState* BGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	if (!BGameState)	return;
	for (auto PState : BGameState->PlayerArray)
	{
		ABlasterPlayerState* BPState = Cast<ABlasterPlayerState>(PState);
		if (BPState && BPState->GetTeam() == ETeam::ET_None)
		{
			if (BGameState->RedTeam.Num() >= BGameState->BlueTeam.Num())
			{
				BGameState->BlueTeam.AddUnique(BPState);
				BPState->SetTeam(ETeam::ET_TeamBlue);
			} else
			{
				BGameState->RedTeam.AddUnique(BPState);
				BPState->SetTeam(ETeam::ET_TeamRed);
			}
		}
	}
}

void ATeamsGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	DebugUtil::PrintMsg(FString::Printf(TEXT("PostLogin")), FColor::Green);
	ABlasterGameState* BGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	if (!NewPlayer || !BGameState)	return;
	ABlasterPlayerState* BPState = NewPlayer->GetPlayerState<ABlasterPlayerState>();
	if (BPState && BPState->GetTeam() == ETeam::ET_None)
	{
		if (BGameState->RedTeam.Num() >= BGameState->BlueTeam.Num())
		{
			BGameState->BlueTeam.AddUnique(BPState);
			BPState->SetTeam(ETeam::ET_TeamBlue);
		} else
		{
			BGameState->RedTeam.AddUnique(BPState);
			BPState->SetTeam(ETeam::ET_TeamRed);
		}
	}
}

void ATeamsGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
	ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	Super::PlayerEliminated(EliminatedCharacter, VictimController, AttackerController);
	
	ABlasterGameState* BGState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	ABlasterPlayerState* BPState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState): nullptr;
	if (BGState && BPState)
	{
		if (BPState->GetTeam() == ETeam::ET_TeamBlue)
		{
			BGState->TeamBlueScores();
		} else if (BPState->GetTeam() == ETeam::ET_TeamRed)
		{
			BGState->TeamRedScores();
		}
	}
}

float ATeamsGameMode::CalculateDamage(AController* AttackerController, AController* VictimController, float BaseDamage)
{
	DebugUtil::PrintMsg(FString("TeamsGameMode, Calculate Damage"), FColor::Red);
	if (!AttackerController || !VictimController || AttackerController == VictimController)	return BaseDamage;
	ABlasterPlayerState* AttackerPlayerState = AttackerController->GetPlayerState<ABlasterPlayerState>();
	ABlasterPlayerState* VictimPlayerState = VictimController->GetPlayerState<ABlasterPlayerState>();
	if (!AttackerPlayerState || !VictimPlayerState)		return BaseDamage;
	if (AttackerPlayerState->GetTeam() == VictimPlayerState->GetTeam())
	{
		DebugUtil::PrintMsg(FString("On The Same Team"), FColor::Orange);
		DebugUtil::PrintMsg(FString::Printf(TEXT("Team is %d"), AttackerPlayerState->GetTeam()), FColor::Cyan);
		DebugUtil::PrintMsg(FString::Printf(TEXT("Team is %d"), VictimPlayerState->GetTeam()), FColor::Cyan);
		return 0.f;
	}
	return BaseDamage;
}
