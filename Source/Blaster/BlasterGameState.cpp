// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameState.h"

#include "Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "PlayerState/BlasterPlayerState.h"

void ABlasterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABlasterGameState, TopScorePlayer);
}

void ABlasterGameState::OnRep_TopScorePlayer()
{
	
}

void ABlasterGameState::UpdateTopScorePlayer(ABlasterPlayerState* PlayerState)
{
	if (PlayerState == nullptr)	return;
	if (PlayerState == nullptr || PlayerState->GetScore() < TopScore)	return;
	if (TopScorePlayer.Num() == 0)
	{
		TopScorePlayer.Add(PlayerState);
		TopScore = PlayerState->GetScore();
	} else
	{
		if (PlayerState->GetScore() == TopScore)
		{
			TopScorePlayer.Add(PlayerState);
		} else if (PlayerState->GetScore() > TopScore)
		{
			TopScorePlayer.Empty();
			TopScorePlayer.Add(PlayerState);
			TopScore = PlayerState->GetScore();
		}
	}
}
