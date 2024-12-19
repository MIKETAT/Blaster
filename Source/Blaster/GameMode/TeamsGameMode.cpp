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
				DebugUtil::PrintMsg(FString::Printf(TEXT("Team Blue Add one")), FColor::Green);
			} else
			{
				BGameState->RedTeam.AddUnique(BPState);
				BPState->SetTeam(ETeam::ET_TeamRed);
				DebugUtil::PrintMsg(FString::Printf(TEXT("Team Red Add one")), FColor::Green);
			}
		}
	}
}

// 有团队得分达到要求时结束游戏
bool ATeamsGameMode::ShouldEndGame()
{
	ABlasterGameState* BGState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	if (BGState)
	{
		return BGState->GetBlueTeamScore() >= TeamWinningScore || BGState->GetRedTeamScore() >= TeamWinningScore;	
	}
	return false;
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
	// TeamMatch模式下Super不会在有玩家死亡时直接判断游戏是否结束
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
	if (ShouldEndGame())
	{
		EndMatch();
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
		DebugUtil::LogMsg(FString("On The Same Team"));
		DebugUtil::PrintMsg(FString::Printf(TEXT("Team is %d"), AttackerPlayerState->GetTeam()), FColor::Cyan);
		DebugUtil::PrintMsg(FString::Printf(TEXT("Team is %d"), VictimPlayerState->GetTeam()), FColor::Cyan);
		return 0.f;
	}
	return BaseDamage;
}
