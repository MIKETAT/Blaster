// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
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
public:
	ABlasterGameMode();
	virtual void PlayerEliminated(class ABlasterCharacter* EliminatedCharacter,
		class ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController);

	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);
	virtual void Tick(float DeltaSeconds) override;

	virtual float CalculateDamage(AController* AttackerController, AController* VictimController, float BaseDamage);

	void PlayerLeftGame(ABlasterPlayerState* LeavingPlayerState);

	bool ShouldEndGame();

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

	bool bTeamMatch = false;
private:
	float CountDownTime;

	UPROPERTY(EditAnywhere)
	USoundCue* LobbyMusic;

	UPROPERTY()
	UAudioComponent* LobbyMusicComp;

	UPROPERTY(EditAnywhere)
	int32 FullScore = 5;
};
