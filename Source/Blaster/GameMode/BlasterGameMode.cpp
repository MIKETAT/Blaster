// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"

#include "Blaster/BlasterGameState.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/Utils/DebugUtil.h"
#include "Components/AudioComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

namespace MatchState
{
	const FName CoolDown = FName(TEXT("CoolDown"));	
}

ABlasterGameMode::ABlasterGameMode()
{
	bDelayedStart = true;	// 会停在WaitingToStart阶段等待 StartMatch进入InProgress阶段
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();
	//LevelStartTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (MatchState == MatchState::WaitingToStart)
	{
		CountDownTime = WarmupTime + LevelStartTime - GetWorld()->GetTimeSeconds();
		if (CountDownTime < 0.f)
		{
			StartMatch();
		}
	} else if (MatchState == MatchState::InProgress)
	{
		CountDownTime = MatchTime + WarmupTime + LevelStartTime - GetWorld()->GetTimeSeconds();
		if (CountDownTime < 0.f)
		{
			SetMatchState(MatchState::CoolDown);
		}
	} else if (MatchState == MatchState::CoolDown)
	{
		if (!bHasWinner)
		{
			CountDownTime = CoolDownTime + MatchTime + WarmupTime + LevelStartTime - GetWorld()->GetTimeSeconds();
		} else
		{
			CountDownTime = CoolDownTime + MatchDuration - GetWorld()->GetTimeSeconds();
		}
		if (CountDownTime < 0.f)
		{
			RestartGame();
			// todo restartGame后的处理，是否需要禁用一些输入。考虑不禁用输入但禁用伤害
		}
	}
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();
	if (MatchState == MatchState::CoolDown)
	{
		HandleMatchCoolDown();
	} else if (MatchState == MatchState::WaitingToStart)
	{
		bHasWinner = false;
	} else if (MatchState == MatchState::InProgress)
	{
		
	}
	FString CurrentMap = GetWorld()->GetCurrentLevel()->GetPathName();
	DebugUtil::PrintMsg(FString::Printf(TEXT("On Server, Current Map is %s, MatchState is %s, And Current Controller Num is %d"), *CurrentMap, *MatchState.ToString(), GetWorld()->GetNumPlayerControllers()));
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; It++)
	{
		ABlasterPlayerController* BPlayerController = Cast<ABlasterPlayerController>(*It);
		if (BPlayerController)
		{
			BPlayerController->SetMatchState(MatchState, bTeamMatch);
			FString Text = FString::Printf(TEXT("Server PlayerController->SetMatchState. SetMatchState to %s, teamMatch is %hs, PlayerController Name is %s"),
					*MatchState.ToString(), bTeamMatch ? "true" : "false", *this->GetName());
			DebugUtil::PrintMsg(this, Text);
		}
	}
}

// Display Winner, then jump to RestartGame
void ABlasterGameMode::HandleMatchCoolDown()
{
	// wait cooldown time ends
}

// TeamMatch模式下无队友伤害, 重写此函数根据是否是同一队计算伤害
float ABlasterGameMode::CalculateDamage(AController* AttackerController, AController* VictimController, float BaseDamage)
{
	DebugUtil::PrintMsg(FString("BlasterGameMode, Calculate Damage"), FColor::Red);
	return BaseDamage;
}

void ABlasterGameMode::PlayerLeftGame(ABlasterPlayerState* LeavingPlayerState)
{
	if (LeavingPlayerState == nullptr)	return;
	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	if (BlasterGameState && BlasterGameState->TopScorePlayer.Contains(LeavingPlayerState))
	{
		BlasterGameState->TopScorePlayer.Remove(LeavingPlayerState);
		// todo maybe refresh
	}
	if (ABlasterCharacter* LeavingPlayer = Cast<ABlasterCharacter>(LeavingPlayerState->GetPawn()))
	{
		LeavingPlayer->Elim(true);
	}
}

bool ABlasterGameMode::ShouldEndGame()
{
	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	return BlasterGameState && BlasterGameState->GetTopScore() >= FullScore; 
}

// override EndMatch, jump to COOLDOWN, then Restart
void ABlasterGameMode::EndMatch()
{
	if (!IsMatchInProgress())
	{
		return;
	}
	bHasWinner = true;
	MatchDuration = GetWorld()->GetTimeSeconds();
	SetMatchState(MatchState::CoolDown);
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
	ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	// Add Socore to Attacker
	ABlasterPlayerState* AttackerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimState = AttackerController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;
	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	// Score 相关
	if (AttackerState && VictimState && BlasterGameState && AttackerState != VictimState)
	{
		AttackerState->AddToScore(1.f);
		VictimState->AddToDefeats(1.f);
		TArray<ABlasterPlayerState*> CurrentTopPlayer;
		for (auto it : BlasterGameState->TopScorePlayer)
		{
			CurrentTopPlayer.Emplace(it);
		}
		BlasterGameState->UpdateTopScorePlayer(AttackerState);
		// All Top Player Gain The Lead
		for (auto it : BlasterGameState->TopScorePlayer)
		{
			if (ABlasterCharacter* Leader = Cast<ABlasterCharacter>(it->GetPawn()))
			{
				Leader->MulticastGainTheLead();
			}
		}
		
		// Lost The Lead
		for (auto i = 0; i < CurrentTopPlayer.Num(); i++)
		{
			if (!BlasterGameState->TopScorePlayer.Contains(CurrentTopPlayer[i]))
			{
				// Update后, 之前的TopPlayer现在如果不再是TopPlayer, LOST THE LEAD
				if (ABlasterCharacter* Loser = Cast<ABlasterCharacter>(CurrentTopPlayer[i]->GetPawn()))
				{
					Loser->MulticastLostTheLead();
				}
			}
		}
	}
	// Elim character
	if (EliminatedCharacter)
	{
		EliminatedCharacter->Elim(false);
		UE_LOG(LogTemp, Error, TEXT("%s is Ellimmed"), *EliminatedCharacter->GetName());
	}
	UWorld* World = GetWorld();
	if (!World)	return;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; It++)
	{
		ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(*It);
		if (BlasterPlayerController)
		{
			BlasterPlayerController->BroadcastElim(AttackerState->GetName(), VictimState->GetName());
		}
	}
	// 不是TeamMatch直接在这里判断游戏是否结束
	if (!bTeamMatch && ShouldEndGame())		
	{
		EndMatch();
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();	// unposses
		ElimmedCharacter->Destroy();
	}
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num()-1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}

