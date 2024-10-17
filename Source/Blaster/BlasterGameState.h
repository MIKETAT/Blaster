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
	
	UPROPERTY(ReplicatedUsing=OnRep_TopScorePlayer)
	TArray<class ABlasterPlayerState*> TopScorePlayer;

	UFUNCTION()
	void OnRep_TopScorePlayer();

	void UpdateTopScorePlayer(ABlasterPlayerState* Character);

	FORCEINLINE int32 GetTopScore() const { return TopScore; }
protected:

private:
	int32 TopScore;
};
