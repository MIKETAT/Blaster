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
	void SetHUDRedTeamScore(int32 RedScore);
	void SetHUDBlueTeamScore(int32 BlueScore);
	virtual void OnPossess(APawn* InPawn) override;
	void HandleMatchWaitToStart();
	void HandleMatchStart(bool bTeamMatch = false);
	void HandleMatchCoolDown();
	void SetMatchState(FName State, bool bTeamMatch = false);
	void InitTeamScore();
	void HideTeamScore();
	FString GetCurrentTopPlayerInfo() const;
	FString GetTeamInfo(class ABlasterGameState* ABGState) const;
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

	void HighPingWarning(bool bShouldWarning);
	void CheckPing(float DeltaSeconds);
	FORCEINLINE float GetSingleTripTime() const { return SingleTripTime; }
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	void ShowReturnToMainMenu();

	UPROPERTY(ReplicatedUsing = OnRep_bShowTeamScore)
	bool bShowTeamScore = false;

	UFUNCTION()
	void OnRep_bShowTeamScore();
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
	
	UPROPERTY()
	float HighPingCheckingTime = 0.f;	// Warning的已持续时间

	UPROPERTY()
	float HighPingAnimExistingTime = 0.f;

	UPROPERTY(EditAnywhere)
	float HighPingTime = 5.f;	// Warning动画 需要展示的时间  todo 如何声明为常量？

	UPROPERTY(EditAnywhere)
	float HighPingThreshold = 50.f;

	UPROPERTY(EditAnywhere)
	float HighPingCheckFrequency = 20.f;

	UPROPERTY()
	float SingleTripTime;
	
	UFUNCTION()
	void OnRep_MatchState();
};
