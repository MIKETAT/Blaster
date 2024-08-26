// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaSeconds) override;
	virtual void ReceivedPlayer() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDCountDown(int32 CountDown);
	void SetHUDTime();
	virtual void OnPossess(APawn* InPawn) override;
	void HandleMatchStart();
	void SetMatchState(FName State);
	void PollInit();
	// Client -> Server
	UFUNCTION(Server, Reliable)
	void RequestServerTime(float ClientRequestTime);

	// Server -> Client
	UFUNCTION(Client, Reliable)
	void ReportServerTime(float ClientRequestTime, float ServerReceivedRequestTime);

	virtual float GetServerTime();	// synced with server clock
protected:
	virtual void BeginPlay() override;
private:
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

	float MatchTime = 120.f;
	int32 CountDownTime = 0;

	UPROPERTY(EditAnywhere)
	float SyncFrequency = 5.f;			// sync 间隔
	float TimeSyncDuration = 0.f;		// 距离上次sync的间隔时间 
	float ServerClientDelta = 0.f;		// server client 相差时间

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	// init HUD
	bool bInitialize = false;
	
	UFUNCTION()
	void OnRep_MatchState();
};
