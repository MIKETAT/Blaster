// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"


namespace MatchState
{
	extern BLASTER_API const FName CoolDown;
}

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

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float CoolDownTime = 10.f;
	
	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 180.f;
	
	float LevelStartTime;
protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;
	virtual void HandleMatchCoolDown();
private:
	float CountDownTime;

	UPROPERTY(EditAnywhere)
	USoundCue* LobbyMusic;

	UPROPERTY()
	UAudioComponent* LobbyMusicComp;
};
