// Fill out your copyright notice in the Description page of Project Settings.


#include "CaptureTheFlagGameMode.h"

#include "Blaster/BlasterGameState.h"
#include "Blaster/CaptureTheFlag/FlagZone.h"
#include "Blaster/Utils/DebugUtil.h"
#include "Blaster/Weapon/Flag.h"

// 保留玩家正常的击杀等逻辑, 但不影响团队得分
void ACaptureTheFlagGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
	ABlasterPlayerController* VictimController,ABlasterPlayerController* AttackerController)
{
	ABlasterGameMode::PlayerEliminated(EliminatedCharacter, VictimController, AttackerController);
	DebugUtil::PrintMsg(FString(TEXT("ACaptureTheFlagGameMode::PlayerEliminated")));
}

void ACaptureTheFlagGameMode::FlagCaptured(const AFlag* Flag, const AFlagZone* Zone)
{
	DebugUtil::PrintMsg(FString(TEXT("ACaptureTheFlagGameMode::FlagCaptured")));
	if (Flag == nullptr || Zone == nullptr)		return;
	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(GameState);
	ACharacter* Player = Cast<ACharacter>(Flag->GetOwner());
	if (Player == nullptr)
	{
		DebugUtil::PrintMsg(Flag, FString::Printf(TEXT("Flag has no owner, invalid score")));
		return;
	} else
	{
		DebugUtil::PrintMsg(Flag, FString::Printf(TEXT("Score Player Name is %s"), *Player->GetName()));
	}
	if (!BlasterGameState)		return;
	
	if (Zone->Team == ETeam::ET_TeamBlue)
	{
		BlasterGameState->TeamBlueScores();
		DebugUtil::PrintMsg(Flag, FString::Printf(TEXT("Team Blue Score: %d"), BlasterGameState->GetBlueTeamScore()));
	} else if (Zone->Team == ETeam::ET_TeamRed)
	{
		BlasterGameState->TeamRedScores();
		DebugUtil::PrintMsg(Flag, FString(TEXT("Team Red Score: %d"), BlasterGameState->GetRedTeamScore()));
	}
}
