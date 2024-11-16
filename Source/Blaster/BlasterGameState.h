// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BlasterGameState.generated.h"

UCLASS()
class BLASTER_API ABlasterGameState : public AGameState
{
	GENERATED_BODY()
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void TeamRedScores();

	void TeamBlueScores();

	FORCEINLINE int32 GetRedTeamScore() const { return TeamRedScore; }
	FORCEINLINE int32 GetBlueTeamScore() const { return TeamBlueScore; }

	UFUNCTION()
	void OnRep_TopScorePlayer();

	void UpdateTopScorePlayer(class ABlasterPlayerState* Character);

	FORCEINLINE int32 GetTopScore() const { return TopScore; }
	
	UPROPERTY(ReplicatedUsing=OnRep_TopScorePlayer)
	TArray<class ABlasterPlayerState*> TopScorePlayer;
	
	TArray<ABlasterPlayerState*> RedTeam;
	
	TArray<ABlasterPlayerState*> BlueTeam;
protected:

private:
	int32 TopScore;

	/**
	 * Team
	 */


	UPROPERTY(ReplicatedUsing = OnRep_TeamRedScore)
	int32 TeamRedScore = 0;

	UPROPERTY(ReplicatedUsing = OnRep_TeamBlueScore)
	int32 TeamBlueScore = 0;

	UFUNCTION()
	void OnRep_TeamRedScore();
	
	UFUNCTION()
	void OnRep_TeamBlueScore();
};
