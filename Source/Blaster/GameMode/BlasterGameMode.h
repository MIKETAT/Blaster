// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()
	ABlasterGameMode();
public:
	virtual void PlayerEliminated(class ABlasterCharacter* EliminatedCharacter,
		class ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController);

	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);
	virtual void Tick(float DeltaSeconds) override;
protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;
private:
	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;
	
	float LevelStartTime;

	float CountDownTime;
};
