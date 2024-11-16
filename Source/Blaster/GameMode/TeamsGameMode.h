// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlasterGameMode.h"
#include "TeamsGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ATeamsGameMode : public ABlasterGameMode
{
	GENERATED_BODY()
public:
	ATeamsGameMode();
	virtual void Logout(AController* Exiting) override;
	
	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void PlayerEliminated(ABlasterCharacter* EliminatedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController) override;
	
	virtual float CalculateDamage(AController* AttackerController, AController* VictimController, float BaseDamage) override;
protected:
	virtual void HandleMatchHasStarted() override;
};
