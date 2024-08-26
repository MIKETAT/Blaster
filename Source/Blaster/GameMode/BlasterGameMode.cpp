// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

ABlasterGameMode::ABlasterGameMode()
{
	bDelayedStart = true;	// 会停在WaitingToStart阶段等待 StartMatch进入InProgress阶段
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();
	LevelStartTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();
	for (FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); it; it++)
	{
		ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(*it);
		PlayerController->SetMatchState(MatchState);
	}
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
	}
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter,
                                        ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	// Add Socore to Attacker
	ABlasterPlayerState* AttackerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimState = AttackerController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;
	if (AttackerState && VictimState && AttackerState != VictimState)
	{
		AttackerState->AddToScore(1.f);
		VictimState->AddToDefeats(1);
	}
	// Elim character
	if (EliminatedCharacter)
	{
		EliminatedCharacter->Elim();
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

