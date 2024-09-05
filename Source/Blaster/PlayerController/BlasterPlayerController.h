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
	void SetHUDCountDown(float CountDown);
	void SetHUDTime();
	void SetHUDAnnouncementCountDown(float CountDown);
	void SetHUDSniperScope(bool bShowScope);
	virtual void OnPossess(APawn* InPawn) override;
	void HandleMatchWaitToStart();
	void HandleMatchStart();
	void HandleMatchCoolDown();
	void SetMatchState(FName State);
	FString GetCurrentTopPlayerInfo();
	void PollInit();
	// Client -> Server
	UFUNCTION(Server, Reliable)
	void RequestServerTime(float ClientRequestTime);

	// Server -> Client
	UFUNCTION(Client, Reliable)
	void ReportServerTime(float ClientRequestTime, float ServerReceivedRequestTime);

	// todo 倒计时不准的问题，为什么都是在server端的调用  网络游戏的调试问题
	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientJoinGame(FName state, float matchTime, float warmupTime, float coolDownTime, float levelStartTime);

	virtual float GetServerTime();	// synced with server clock

protected:
	virtual void BeginPlay() override;
private:
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

	int32 CountDownTime = 0;

	UPROPERTY(EditAnywhere)
	float SyncFrequency = 5.f;			// sync 间隔
	float TimeSyncDuration = 0.f;		// 距离上次sync的间隔时间 
	float ServerClientDelta = 0.f;		// server client 相差时间

	float LevelStartTime = 0.f;
	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float CoolDownTime = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	// init HUD
	bool bInitialize = false;
	
	UFUNCTION()
	void OnRep_MatchState();
};
