// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerState.h"

#include "Blaster/Character/BlasterCharacter.h"

// Handle Score change both on server and on client 
void ABlasterPlayerState::ScoreChange(float ScoreAmount)
{
	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDScore(ScoreAmount);
		}
	}
}

// call it on the server
void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
	Score += ScoreAmount;
	ScoreChange(Score);		// todo 填什么？
}


void ABlasterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();
	UE_LOG(LogTemp, Error, TEXT("OnRep_Score"));
	ScoreChange(Score);
}

