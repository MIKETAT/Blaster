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
	
	virtual void PlayerEliminated(ABlasterCharacter* EliminatedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController);

	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);
	
	virtual void Tick(float DeltaSeconds) override;

	virtual float CalculateDamage(AController* AttackerController, AController* VictimController, float BaseDamage);

	void PlayerLeftGame(ABlasterPlayerState* LeavingPlayerState);

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float CoolDownTime = 10.f;
	
	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 600.f;
	
	float LevelStartTime;
protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;
	virtual void HandleMatchCoolDown();
	virtual bool ShouldEndGame();
	virtual void EndMatch() override;
	
	UPROPERTY(EditAnywhere)
	USoundCue* LobbyMusic;

	UPROPERTY()
	UAudioComponent* LobbyMusicComp;

	// 是否为团队游戏: 夺旗/团队竞赛
	bool bTeamMatch = false;

	// if it's a team match, when one team's score reaches FullScore then this team win
	UPROPERTY(EditAnywhere)
	int32 FullScore = 2;
	
private:
	float CountDownTime;
	
	bool bHasWinner = false;		// 游戏时间耗尽前是否产生了胜者

	float MatchDuration;
};
