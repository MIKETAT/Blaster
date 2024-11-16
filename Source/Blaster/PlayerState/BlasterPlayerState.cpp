// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerState.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Utils/DebugUtil.h"
#include "Net/UnrealNetwork.h"

// Handle Score change both on server and on client 
void ABlasterPlayerState::ScoreChange(float ScoreAmount)
{
	Character = Cast<ABlasterCharacter>(GetPawn());
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDScore(ScoreAmount);
		}
	}
}

// Handle Defeats change
void ABlasterPlayerState::DefeatsChange(int32 DefeatsAmount)
{
	Character = Cast<ABlasterCharacter>(GetPawn());
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDDefeats(DefeatsAmount);
		}
	}
}

void ABlasterPlayerState::SetTeam(ETeam TeamToSet)
{
	DebugUtil::PrintMsg(FString::Printf(TEXT("SetTeam")), FColor::Green);
	Team = TeamToSet;
	Character = Cast<ABlasterCharacter>(GetPawn());
	if (Character)
	{
		Character->SetTeamColor(Team);
	}
}

void ABlasterPlayerState::OnRep_Team()
{
	Character = Cast<ABlasterCharacter>(GetPawn());
	if (Character)
	{
		Character->SetTeamColor(Team);
	}
}

// call it on the server
void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);
	ScoreChange(GetScore());
}

void ABlasterPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount;
	DefeatsChange(Defeats);
}


void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABlasterPlayerState, Defeats);
	DOREPLIFETIME(ABlasterPlayerState, Team);
}

void ABlasterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();
	ScoreChange(GetScore());
}

void ABlasterPlayerState::OnRep_Defeats()
{
	DefeatsChange(Defeats);
}

