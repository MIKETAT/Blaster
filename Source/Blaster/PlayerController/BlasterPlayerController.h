// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

class ABlasterCharacter;
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
	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDAmmo(int32 Ammo, int32 CarriedAmmo);
	void SetHUDGrenades(int32 Grenades);
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

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	UFUNCTION(Client, Reliable)
	void ClientJoinGame(FName state, float matchTime, float warmupTime, float coolDownTime, float levelStartTime);

	virtual float GetServerTime();	// synced with server clock

	void BroadcastElim(const FString& AttackerName, const FString& VictimName);

	UFUNCTION(Client, Reliable)
	void ClientElimAnnouncement(const FString& AttackerName, const FString& VictimName);
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	void ShowReturnToMainMenu();
	
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

	/**
	 * Return To Main Menu Widget
	 */
	UPROPERTY(EditAnywhere, Category = HUD)
	TSubclassOf<UUserWidget> ReturnToMainMenuClass;

	UPROPERTY()
	class UReturnToMainMenu* MainMenuWidget;

	UPROPERTY()
	bool bReturnToMainMenuOpen = false;
	
	
	UFUNCTION()
	void OnRep_MatchState();
};
