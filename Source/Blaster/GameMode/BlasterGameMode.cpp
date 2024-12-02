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
		if (LobbyMusic)
		{
			LobbyMusicComp = UGameplayStatics::SpawnSound2D(this, LobbyMusic);	
		}
	} else if (MatchState == MatchState::InProgress)
	{
		if (LobbyMusicComp)
		{
			LobbyMusicComp->Stop();
		}
	}
	for (FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); it; it++)
	{
		ABlasterPlayerController* BPlayerController = Cast<ABlasterPlayerController>(*it);
		if (BPlayerController)
		{
			BPlayerController->SetMatchState(MatchState, bTeamMatch);
		}
	}
}

// Display Winner, then jump to RestartGame
void ABlasterGameMode::HandleMatchCoolDown()
{
	// wait cooldown time ends
}



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
	if (AttackerState && VictimState && BlasterGameState && AttackerState != VictimState)
	{
		AttackerState->AddToScore(1.f);
		VictimState->AddToDefeats(1.f);
		// todo: 如何拷贝一个已有的TArray,类似STL的拷贝构造
		TArray<ABlasterPlayerState*> CurrentTopPlayer;
		for (auto it : BlasterGameState->TopScorePlayer)
		{
			CurrentTopPlayer.Emplace(it);
		}
		BlasterGameState->UpdateTopScorePlayer(AttackerState);
		// Gain The Lead
		for (auto it : BlasterGameState->TopScorePlayer)
		{
			ABlasterCharacter* Leader = Cast<ABlasterCharacter>(it->GetPawn());
			if (Leader)
			{
				Leader->MulticastGainTheLead();
			}
		}
		
		// Lost The Lead
		for (auto i = 0; i < CurrentTopPlayer.Num(); i++)
		{
			if (!BlasterGameState->TopScorePlayer.Contains(CurrentTopPlayer[i]))
			{
				ABlasterCharacter* Loser = Cast<ABlasterCharacter>(CurrentTopPlayer[i]->GetPawn());
				if (Loser)
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
	if (BlasterGameState && ShouldEndGame())
	{
		//RestartGame();	// todo Announce  这个条件 FullScore应该放在GameState里吗
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

