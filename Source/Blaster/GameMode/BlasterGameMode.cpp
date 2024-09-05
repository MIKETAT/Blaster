// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"

#include "Blaster/BlasterGameState.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
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

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();
	if (MatchState == MatchState::CoolDown)
	{
		
	} else if (MatchState == MatchState::WaitingToStart)
	{
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
		ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(*it);
		PlayerController->SetMatchState(MatchState);
	}
}

void ABlasterGameMode::HandleMatchCoolDown()
{
	// 保留，用于对自定义state的处理
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
		CountDownTime = CoolDownTime + MatchTime + WarmupTime + LevelStartTime - GetWorld()->GetTimeSeconds();
		if (CountDownTime < 0.f)
		{
			RestartGame();
			// todo restartGame后的处理，是否需要禁用一些输入。考虑不禁用输入但禁用伤害
		}
	}
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
		VictimState->AddToDefeats(1);
		BlasterGameState->UpdateTopScorePlayer(AttackerState);
	}
	// Elim character
	if (EliminatedCharacter)
	{
		EliminatedCharacter->Elim();
		UE_LOG(LogTemp, Error, TEXT("%s is Ellimmed"), *EliminatedCharacter->GetName());
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

