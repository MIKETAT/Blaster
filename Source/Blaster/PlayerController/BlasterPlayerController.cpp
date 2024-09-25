#include "BlasterPlayerController.h"

#include "Blaster/BlasterGameState.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/HUD/Announcement.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Blaster/HUD/SniperScopeOverlayWidget.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

// client加入游戏时请求server的状态
void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	if (HasAuthority())
	{
		UE_LOG(LogTemp, Error, TEXT("Server"));
	} else
	{
		UE_LOG(LogTemp, Error, TEXT("Client"));
	}
	ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		CoolDownTime = GameMode->CoolDownTime;
		MatchTime = GameMode->MatchTime;
		LevelStartTime = GameMode->LevelStartTime;
		MatchState = GameMode->GetMatchState();
		UE_LOG(LogTemp, Error, TEXT("in ServerCheckMatchState, MatchState = %s, MatchTime = %f, WarmupTime = %f, CoolDownTime = %f, LevelStartTime = %f"),
			*MatchState.ToString(), MatchTime, WarmupTime, CoolDownTime, LevelStartTime);
		ClientJoinGame_Implementation(MatchState, MatchTime, WarmupTime, CoolDownTime, LevelStartTime);
	} 
}

void ABlasterPlayerController::ClientJoinGame_Implementation(FName state, float matchTime, float warmupTime, float coolDownTime, float levelStartTime)
{
	MatchTime = matchTime;
	WarmupTime = warmupTime;
	CoolDownTime = coolDownTime;
	LevelStartTime = levelStartTime;
	MatchState = state;
	SetMatchState(MatchState);
	if (HasAuthority())
	{
		UE_LOG(LogTemp, Error, TEXT("Server"));
	} else
	{
		UE_LOG(LogTemp, Error, TEXT("Client"));
	}
	UE_LOG(LogTemp, Error, TEXT("in ClientJoinGame_Implementation, MatchState = %s, MatchTime = %f, WarmupTime = %f, CoolDownTime = %f, LevelStartTime = %f"),
			*MatchState.ToString(), MatchTime, WarmupTime, CoolDownTime, LevelStartTime);
	if (BlasterHUD && MatchState == MatchState::WaitingToStart)
	{
		BlasterHUD->AddAnnouncement();
	}
}

float ABlasterPlayerController::GetServerTime()
{
	if (HasAuthority())		return GetWorld()->GetTimeSeconds();
	return GetWorld()->GetTimeSeconds() + ServerClientDelta;
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	ServerCheckMatchState();
}

void ABlasterPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsLocalController() && TimeSyncDuration > SyncFrequency)
	{
		RequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncDuration = 0.f;
	}
	PollInit();
	SetHUDTime();
}

// 时机很早
void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		RequestServerTime(GetWorld()->GetTimeSeconds());	
	}
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABlasterPlayerController, MatchState);
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->HealthBar && BlasterHUD->CharacterOverlay->HealthText)
	{
		// calc health percent and set
		const float HealthPercent = Health / MaxHealth;
		BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		// set health text
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->ScoreAmount;
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("Score: %d"), FMath::FloorToInt(Score));
		BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->DefeatsAmount;
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("Defeats: %d"), Defeats);
		BlasterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
}

// 显示当前子弹数
void ABlasterPlayerController::SetHUDAmmo(int32 Ammo, int32 CarriedAmmo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->AmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("Ammo:%d/%d"), Ammo, CarriedAmmo);
		BlasterHUD->CharacterOverlay->AmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

// todo Ammo/CarriedAmmo 使用同一个TextBlock实现
/*void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bHUDValid)
	{
		FString CarriedAmmoText = FString::Printf(TEXT("Carried : %d"), Ammo);
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(CarriedAmmoText));
	}
}*/

void ABlasterPlayerController::SetHUDCountDown(float CountDown)
{

	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->MatchCountDownText;
	if (bHUDValid)
	{
		if (CountDown < 0.f)
		{
			BlasterHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}
		if (CountDown <= 30.f)
		{
			BlasterHUD->CharacterOverlay->MatchCountDownText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
		}
		
		int32 Minute = FMath::FloorToInt(CountDown / 60);
		int32 Second = CountDown - Minute * 60;
		FString CountDownText = FString::Printf(TEXT("%02d:%02d"), Minute, Second);
		BlasterHUD->CharacterOverlay->MatchCountDownText->SetText(FText::FromString(CountDownText));
	
	}
#if 0
	if (!HasAuthority())
	{
		if (!GetHUD())	UE_LOG(LogTemp, Error, TEXT("OnClient SetHUDCountDown GetHUD is null"));
		if (GetHUD())	UE_LOG(LogTemp, Error, TEXT("OnClient SetHUDCountDown GetHUD %s"), *GetHUD()->GetName());
		if (!BlasterHUD)	UE_LOG(LogTemp, Error, TEXT("OnClient SetHUDCountDown BlasterHUD is null"));
		if (BlasterHUD && !BlasterHUD->Announcement)	UE_LOG(LogTemp, Error, TEXT("OnClient SetHUDCountDown Announcement is null"));
		if (BlasterHUD && BlasterHUD->Announcement && !BlasterHUD->Announcement->WarmupTime)	UE_LOG(LogTemp, Error, TEXT("OnClient SetHUDCountDown WarmupTime is null"));	
	}
#endif
}

void ABlasterPlayerController::SetHUDTime()
{
	if (HasAuthority())
	{
		ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
		if (GameMode && LevelStartTime <= 0.f)
		{
			LevelStartTime = GameMode->LevelStartTime;
		}
	}
	
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart)	TimeLeft = WarmupTime - GetServerTime() + LevelStartTime;
	else if (MatchState == MatchState::InProgress)	TimeLeft = MatchTime - GetServerTime() + LevelStartTime + WarmupTime;
	else if (MatchState == MatchState::CoolDown)	TimeLeft = MatchTime - GetServerTime() + LevelStartTime + WarmupTime + CoolDownTime;
	
	int32 SecondLeft = FMath::CeilToInt(TimeLeft);

#if 0	
	// 输出各个时间，看Client是否同步时间成功
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Error, TEXT("OnClient, SetHUDTime, MatchState = %s, MatchTime = %f, WarmupTime = %f, CoolDownTime = %f, LevelStartTime = %f"),
			*MatchState.ToString(), MatchTime, WarmupTime, CoolDownTime, LevelStartTime);
	}
#endif
	
	
	if (CountDownTime != SecondLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::CoolDown)
		{
			SetHUDAnnouncementCountDown(TimeLeft);
		} else if (MatchState == MatchState::InProgress)
		{
			SetHUDCountDown(TimeLeft);
		}
	}
	CountDownTime = TimeLeft;
}

void ABlasterPlayerController::SetHUDAnnouncementCountDown(float CountDown)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->Announcement &&
		BlasterHUD->Announcement->WarmupTime;
	if (bHUDValid)
	{
		// todo 由于rpc没有传到client，导致clint这里CountDown都是负值，因此倒计时被隐藏
		if (CountDown < 0.f)
		{
			BlasterHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}
		int32 Minute = FMath::FloorToInt(CountDown / 60);
		int32 Second = CountDown - Minute * 60;
		FString CountDownText = FString::Printf(TEXT("%02d:%02d"), Minute, Second);
		BlasterHUD->Announcement->WarmupTime->SetText(FText::FromString(CountDownText));
	}
#if 0
	if (!HasAuthority())
	{
		if (!GetHUD())	UE_LOG(LogTemp, Error, TEXT("OnClient  SetHUDAnnouncementCountDown GetHUD is null"));
		if (GetHUD())	UE_LOG(LogTemp, Error, TEXT("OnClient SetHUDAnnouncementCountDown GetHUD %s"), *GetHUD()->GetName());
		if (!BlasterHUD)	UE_LOG(LogTemp, Error, TEXT("OnClient SetHUDAnnouncementCountDown BlasterHUD is null"));
		if (BlasterHUD && !BlasterHUD->Announcement)	UE_LOG(LogTemp, Error, TEXT("OnClient SetHUDAnnouncementCountDown Announcement is null"));
		if (BlasterHUD && BlasterHUD->Announcement && !BlasterHUD->Announcement->WarmupTime)	UE_LOG(LogTemp, Error, TEXT("OnClient SetHUDAnnouncementCountDown WarmupTime is null"));	
	}
#endif
	
}

void ABlasterPlayerController::SetHUDSniperScope(bool bShowScope)
{
	if (BlasterHUD)
	{
		if (!BlasterHUD->SniperScopeOverlayWidget->IsInViewport())
		{
			BlasterHUD->SniperScopeOverlayWidget->AddToViewport();
		}
		BlasterHUD->ShowSniperScopeOverlay(bShowScope);	
	}
}	

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
	if (BlasterCharacter)
	{
		// we do it here again cause in BeginPlay, not all objects are initialized
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());	
	}
}

void ABlasterPlayerController::HandleMatchWaitToStart()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		BlasterHUD->AddAnnouncement();
	}
	//UGameplayStatics::SpawnSound2D(this)
}

void ABlasterPlayerController::HandleMatchStart()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		BlasterHUD->AddCharacterOverlay();
		if (BlasterHUD->Announcement)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ABlasterPlayerController::HandleMatchCoolDown()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		if (BlasterHUD->CharacterOverlay)	BlasterHUD->CharacterOverlay->RemoveFromParent();
		bool bIsHUDValid = BlasterHUD->Announcement &&
			BlasterHUD->Announcement->InfoText &&
			BlasterHUD->Announcement->AnnouncementText;
		if (bIsHUDValid)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText("Next Match Starts In:");
			BlasterHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			FString TopPlayerInfo = GetCurrentTopPlayerInfo();
			BlasterHUD->Announcement->InfoText->SetText(FText::FromString(TopPlayerInfo));	// hide info text
		}
	}
}

void ABlasterPlayerController::SetMatchState(FName State)
{
	MatchState = State;
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	
	if (MatchState == MatchState::WaitingToStart)
	{
		HandleMatchWaitToStart();
	}
	else if (MatchState == MatchState::InProgress)
	{
		HandleMatchStart();
	} else if (MatchState == MatchState::CoolDown)
	{
		HandleMatchCoolDown();
	}
}

FString ABlasterPlayerController::GetCurrentTopPlayerInfo()
{
	AGameStateBase* base = UGameplayStatics::GetGameState(this);
	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	ABlasterPlayerState* BlasterPlayerState = Cast<ABlasterPlayerState>(GetPlayerState<ABlasterPlayerState>());
	FString TopPlayerInfo;
	
	if (BlasterGameState && BlasterPlayerState)
	{
		TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScorePlayer;
		if (TopPlayers.Num() == 0)
		{
			TopPlayerInfo = FString("No Winner!");
		} else if (TopPlayers.Num() == 1 && TopPlayers[0] == BlasterPlayerState)
		{
			TopPlayerInfo = FString("You Are The Champion!");
		} else if (TopPlayers.Num() == 1)
		{
			TopPlayerInfo = FString("Introducing Your Champions...\n");
			TopPlayerInfo.Append(TopPlayers[0]->GetName() + " !");
		} else
		{
			// tied
			TopPlayerInfo = FString("Introducing Your Champions...\n");
			for (auto TiedPlayer : TopPlayers)
			{
				TopPlayerInfo.Append(TiedPlayer->GetName() + "\n");
			}
		}
	}
	return TopPlayerInfo;
}

void ABlasterPlayerController::PollInit()
{
	if (!bInitialize)
	{
		if (BlasterHUD)
		{
			bInitialize = BlasterHUD->AddAnnouncement();
		}
	}
}

void ABlasterPlayerController::OnRep_MatchState()
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (MatchState == MatchState::WaitingToStart)
	{
		HandleMatchWaitToStart();
	}
	else if (MatchState == MatchState::InProgress)
	{
		HandleMatchStart();
	} else if (MatchState == MatchState::CoolDown)
	{
		HandleMatchCoolDown();
	}
}

// client -> server
void ABlasterPlayerController::RequestServerTime_Implementation(float ClientRequestTime)
{
	float ServerTime = GetWorld()->GetTimeSeconds();
	ReportServerTime(ClientRequestTime, ServerTime);
}

// server -> client
void ABlasterPlayerController::ReportServerTime_Implementation(float ClientRequestTime, float ServerReceivedRequestTime)
{
	float RTT = GetWorld()->GetTimeSeconds() - ClientRequestTime;
	float CurrentServerTime = ServerReceivedRequestTime + (0.5f * RTT);
	ServerClientDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}
