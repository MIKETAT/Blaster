#include "BlasterPlayerController.h"

#include "Blaster/BlasterGameState.h"
#include "Blaster/BlasterComponent/CombatComponent.h"
#include "Blaster/BlasterTypes/Announcement.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/HUD/Announcement.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Blaster/HUD/ReturnToMainMenu.h"
#include "Blaster/HUD/SniperScopeOverlayWidget.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/BlasterTypes/Announcement.h"
#include "Blaster/Utils/DebugUtil.h"
#include "Blaster/Weapon/Weapon.h"
#include "Components/AudioComponent.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundCue.h"

// client加入游戏时请求server的状态
void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		CoolDownTime = GameMode->CoolDownTime;
		MatchTime = GameMode->MatchTime;
		LevelStartTime = GameMode->LevelStartTime;
		MatchState = GameMode->GetMatchState();
		bShowTeamScore = GameMode->IsTeamMatch();	//
		ClientJoinGame(MatchState, MatchTime, WarmupTime, CoolDownTime, LevelStartTime, bShowTeamScore);
	}
	/*UE_LOG(LogTemp, Error, TEXT("in ServerCheckMatchState, MatchState = %s, MatchTime = %f, WarmupTime = %f, CoolDownTime = %f, LevelStartTime = %f"),
			*MatchState.ToString(), MatchTime, WarmupTime, CoolDownTime, LevelStartTime);*/
}

void ABlasterPlayerController::ClientJoinGame_Implementation(FName state, float matchTime, float warmupTime, float coolDownTime, float levelStartTime, bool bTeamMatch)
{
	DebugUtil::PrintMsg(this, FString::Printf(TEXT("ClientJoinGame_Implementation, bShowTeamScore is %hs"), bShowTeamScore ? "true" : "false"));
	MatchTime = matchTime;
	WarmupTime = warmupTime;
	CoolDownTime = coolDownTime;
	LevelStartTime = levelStartTime;
	MatchState = state;
	bShowTeamScore = bTeamMatch;	// TeamMatch中client上这个变量一直不对，似乎和client的Controller加入游戏的时机有关。
	SetMatchState(MatchState, bShowTeamScore);
	/*UE_LOG(LogTemp, Error, TEXT("in ClientJoinGame_Implementation, MatchState = %s, MatchTime = %f, WarmupTime = %f, CoolDownTime = %f, LevelStartTime = %f"),
			*MatchState.ToString(), MatchTime, WarmupTime, CoolDownTime, LevelStartTime);*/
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

void ABlasterPlayerController::BroadcastElim(const FString& AttackerName, const FString& VictimName)
{
	ClientElimAnnouncement(AttackerName, VictimName);
}

void ABlasterPlayerController::ClientElimAnnouncement_Implementation(const FString& AttackerName, const FString& VictimName)
{
	// todo: Attacker Victim Self 三者的关系影响输出文本
	if (BlasterHUD)
	{
		BlasterHUD->AddElimAnnouncement(AttackerName, VictimName);
	}
}

void ABlasterPlayerController::HighPingWarning(bool bShouldWarning)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bValid = BlasterHUD && BlasterHUD->CharacterOverlay
	&& BlasterHUD->CharacterOverlay->HighPingImage
	&& BlasterHUD->CharacterOverlay->HighPingWarningAnim;
	if (!bValid)	return;
	if (bShouldWarning)
	{
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(1.f);
		BlasterHUD->CharacterOverlay->PlayAnimation(BlasterHUD->CharacterOverlay->HighPingWarningAnim, 0, 5);
	} else
	{
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if (BlasterHUD->CharacterOverlay->IsAnimationPlaying(BlasterHUD->CharacterOverlay->HighPingWarningAnim))
		{
			BlasterHUD->CharacterOverlay->StopAnimation(BlasterHUD->CharacterOverlay->HighPingWarningAnim);
		}
	}
}

void ABlasterPlayerController::CheckPing(float DeltaSeconds)
{
	if (HasAuthority())		return;		// server 无延迟
	HighPingCheckingTime += DeltaSeconds;
	if (HighPingCheckingTime > HighPingCheckFrequency)
	{
		// check
		//DebugUtil::PrintMsg(FString("Check Lag"), FColor::Black);
		ABlasterPlayerState* BlasterPlayerState = Cast<ABlasterPlayerState>(GetPlayerState<ABlasterPlayerState>());
		if (BlasterPlayerState)
		{
			//DebugUtil::PrintMsg(FString::Printf(TEXT("Ping: %f"), BlasterPlayerState->GetPingInMilliseconds()), FColor::Blue);
			if (BlasterPlayerState->GetPingInMilliseconds() > HighPingThreshold)
			{
				HighPingWarning(true);
				HighPingAnimExistingTime = 0.f;
				ServerReportPingStatus(true);
			} else
			{
				ServerReportPingStatus(false);
			}
		}
		HighPingCheckingTime = 0.f;
	} 
	// check anim playing time
	bool bValid = BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->HighPingImage
		&& BlasterHUD->CharacterOverlay->HighPingWarningAnim;
	if (bValid)
	{
		HighPingAnimExistingTime += DeltaSeconds;
		if (HighPingAnimExistingTime > HighPingTime)
		{
			HighPingWarning(false);
		}
	}
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	DebugUtil::PrintMsg(this, FString::Printf(TEXT("Blaster Controller BeginPlay")));
	ServerCheckMatchState();
	bInitialize = false;
}

void ABlasterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent == nullptr)	return;
	InputComponent->BindAction(TEXT("Quit"), IE_Pressed, this, &ABlasterPlayerController::ShowReturnToMainMenu);
}

void ABlasterPlayerController::ShowReturnToMainMenu()
{
	if (ReturnToMainMenuClass == nullptr)	return;
	if (MainMenuWidget == nullptr)
	{
		MainMenuWidget = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuClass);
	}
	if (MainMenuWidget)
	{
		bReturnToMainMenuOpen = !bReturnToMainMenuOpen;
		if (bReturnToMainMenuOpen)
		{
			MainMenuWidget->MenuSetup();
		} else
		{
			MainMenuWidget->MenuTearDown();
		}
	}
}

void ABlasterPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}

void ABlasterPlayerController::OnRep_bShowTeamScore()
{
	DebugUtil::PrintMsg(this, FString::Printf(TEXT("OnRep_bShowTeamScore")));
	if (bShowTeamScore)
	{
		InitTeamScore();
	} else
	{
		HideTeamScore();
	}
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
	CheckPing(DeltaSeconds);
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
	DOREPLIFETIME(ABlasterPlayerController, bShowTeamScore);
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

void ABlasterPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD && BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->ShieldBar && BlasterHUD->CharacterOverlay->ShieldText)
	{
		const float ShieldPercent = Shield / MaxShield;
		BlasterHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);
		FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		BlasterHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
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
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
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
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
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
		FString AmmoText = FString::Printf(TEXT("%d/%d"), Ammo, CarriedAmmo);
		BlasterHUD->CharacterOverlay->AmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void ABlasterPlayerController::SetHUDGrenades(int32 Grenades)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->GrenadesAmount;
	if (bHUDValid)
	{
		FString GrenadesAmount = FString::Printf(TEXT("%d"), Grenades);
		BlasterHUD->CharacterOverlay->GrenadesAmount->SetText(FText::FromString(GrenadesAmount));
	}
}

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
	else if (MatchState == MatchState::CoolDown)
	{
		TimeLeft = MatchDuration + CoolDownTime - GetServerTime();
	}
	
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

void ABlasterPlayerController::SetHUDRedTeamScore(int32 RedScore)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->RedTeamScore;
	if (bValid)
	{
		FText ScoreText = FText::FromString(FString::Printf(TEXT("%d"), RedScore));
		BlasterHUD->CharacterOverlay->RedTeamScore->SetText(ScoreText);
	}
}

void ABlasterPlayerController::SetHUDBlueTeamScore(int32 BlueScore)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->BlueTeamScore;
	if (bValid)
	{
		FText ScoreText = FText::FromString(FString::Printf(TEXT("%d"), BlueScore));
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetText(ScoreText);
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
	if (LobbyMusic)
	{
		LobbyMusicComp = UGameplayStatics::SpawnSound2D(this, LobbyMusic, 0.5);	
	}
}

void ABlasterPlayerController::HandleMatchStart(bool bTeamMatch)
{
	DebugUtil::PrintMsg(this, FString::Printf(TEXT("ABlasterPlayerController HandleMatchStart, is team match ? %hs"), bTeamMatch ? "yes" : "no"));
	DebugUtil::PrintMsg(this, FString::Printf(TEXT("And PlayerController Name is %s,"), *GetName()));
	if (HasAuthority())
	{
		bShowTeamScore = bTeamMatch;
		DebugUtil::PrintMsg(this, FString::Printf(TEXT("IN ABlasterPlayerController::HandleMatchStart(bool bTeamMatch), "
												 "and HasAuthority, bShowTeamScore = bTeamMatc, and bTeamMatch is %hs"), bTeamMatch ? "true" : "false"));
	}
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		BlasterHUD->AddCharacterOverlay();
		if (BlasterHUD->Announcement)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
		// todo 这里有个client就return的逻辑， 有空看看不加怎么样
		if (bTeamMatch)
		{
			InitTeamScore();
		} else
		{
			HideTeamScore();
		}
	}
	// handle warmup music
	if (LobbyMusicComp && LobbyMusicComp->IsPlaying())
	{
		//LobbyMusicComp->Stop();
		LobbyMusicComp->SetActive(false);
		LobbyMusicComp->SetPaused(true);
		DebugUtil::LogMsg(FString::Printf(TEXT("Call Stop Music")));
	}
	
}

void ABlasterPlayerController::HandleMatchCoolDown()
{
	DebugUtil::PrintMsg(this, FString::Printf(TEXT("ABlasterPlayerController::HandleMatchCoolDown")));
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	DisableInput(this);
	if (BlasterHUD)
	{
		if (BlasterHUD->CharacterOverlay)	BlasterHUD->CharacterOverlay->RemoveFromParent();
		bool bIsHUDValid = BlasterHUD->Announcement &&
			BlasterHUD->Announcement->InfoText &&
			BlasterHUD->Announcement->AnnouncementText;
		if (!bIsHUDValid)
		{
			DebugUtil::PrintMsg(this, FString::Printf(TEXT("invalid HUD")));
			return;
		}
		BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
		FString Announcement = Announcement::NextMatchStartIn;
		BlasterHUD->Announcement->AnnouncementText->SetText(FText::FromString(Announcement));
		ABlasterGameState* ABGState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
		FString TopPlayerInfo = bShowTeamScore ? GetTeamInfo(ABGState): GetCurrentTopPlayerInfo();
		BlasterHUD->Announcement->InfoText->SetText(FText::FromString(TopPlayerInfo));	// hide info text
	}
}

void ABlasterPlayerController::SetMatchState(FName State, bool bTeamMatch)
{
	MatchState = State;
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	FString Text = FString::Printf(TEXT("PlayerController SetMatchState to %s, teamMatch is %hs, PlayerController Name is %s"),
		*State.ToString(), bTeamMatch ? "true" : "false", *this->GetName());
	DebugUtil::PrintMsg(this, Text);
	
	if (MatchState == MatchState::WaitingToStart)
	{
		HandleMatchWaitToStart();
	}
	else if (MatchState == MatchState::InProgress)
	{
		HandleMatchStart(bTeamMatch);
	} else if (MatchState == MatchState::CoolDown)
	{
		HandleMatchCoolDown();
		MatchDuration = GetServerTime();
	}
}

void ABlasterPlayerController::InitTeamScore()
{
	DebugUtil::PrintMsg(this, FString::Printf(TEXT("InitTeamScore")));
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (bool bValid = BlasterHUD && BlasterHUD->CharacterOverlay && BlasterHUD->CharacterOverlay->RedTeamScore && BlasterHUD->CharacterOverlay->BlueTeamScore)
	{
		BlasterHUD->CharacterOverlay->RedTeamScore->SetText(FText::FromString("0"));
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetText(FText::FromString("0"));
		DebugUtil::PrintMsg(this, FString::Printf(TEXT("InitTeamScore valid")));
	}
}

void ABlasterPlayerController::HideTeamScore()
{
	DebugUtil::PrintMsg(this, FString::Printf(TEXT("HideTeamScore")));
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bValid =	BlasterHUD
					&& BlasterHUD->CharacterOverlay
					&& BlasterHUD->CharacterOverlay->RedTeamScore
					&& BlasterHUD->CharacterOverlay->BlueTeamScore;
	if (bValid)
	{
		BlasterHUD->CharacterOverlay->RedTeamScore->SetVisibility(ESlateVisibility::Hidden);
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetVisibility(ESlateVisibility::Hidden);
		DebugUtil::PrintMsg(this, FString::Printf(TEXT("HideTeamScore valid")));
	}
}

FString ABlasterPlayerController::GetCurrentTopPlayerInfo() const
{
	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	ABlasterPlayerState* BlasterPlayerState = Cast<ABlasterPlayerState>(GetPlayerState<ABlasterPlayerState>());
	FString TopPlayerInfo;
	
	if (BlasterGameState && BlasterPlayerState)
	{
		TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScorePlayer;
		if (TopPlayers.Num() == 0)
		{
			TopPlayerInfo = Announcement::NoWinner;
		} else if (TopPlayers.Num() == 1 && TopPlayers[0] == BlasterPlayerState)
		{
			TopPlayerInfo = Announcement::YouAreTheWinner;
		} else if (TopPlayers.Num() == 1)
		{
			TopPlayerInfo = Announcement::IntroducingYourChampion;
			TopPlayerInfo.Append(TopPlayers[0]->GetName() + " !");
		} else if (TopPlayers.Num() > 1)
		{
			// tied
			TopPlayerInfo = Announcement::PlayerTiedForTheWin;
			TopPlayerInfo.Append("\n");
			for (auto TiedPlayer : TopPlayers)
			{
				TopPlayerInfo.Append(TiedPlayer->GetName() + "\n");
			}
		}
	}
	return TopPlayerInfo;
}

FString ABlasterPlayerController::GetTeamInfo(ABlasterGameState* ABGState) const
{
	if (ABGState == nullptr)	return FString();
	FString TeamInfoText{"DefaultInfo"};
	const int32 RedTeamScore = ABGState->GetRedTeamScore();
	const int32 BlueTeamScore = ABGState->GetBlueTeamScore();

	if (RedTeamScore == 0 && BlueTeamScore == 0)
	{
		TeamInfoText = Announcement::NoWinner;
	} else if (RedTeamScore == BlueTeamScore)
	{
		TeamInfoText = Announcement::Draw;
	} else if (RedTeamScore > BlueTeamScore)
	{
		TeamInfoText = Announcement::RedTeamWins;
		TeamInfoText.Append("\n");
		TeamInfoText.Append(FString::Printf(TEXT("Red Team %d Wins Blue Team %d!"), RedTeamScore, BlueTeamScore));
	} else if (BlueTeamScore > RedTeamScore)
	{
		
		TeamInfoText = Announcement::BlueTeamWins;
		TeamInfoText.Append("\n");
		TeamInfoText.Append(FString::Printf(TEXT("Blue Team %d Wins Red Team %d!"), BlueTeamScore, RedTeamScore));
	}
	return TeamInfoText;
}

void ABlasterPlayerController::PollInit()
{
	if (bInitialize)					return;
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter == nullptr || BlasterCharacter->GetCombat() == nullptr)			return;
	ABlasterPlayerState* BlasterPlayerState = BlasterCharacter->GetBlasterPlayerState();
	if (BlasterPlayerState == nullptr)	return;
	SetHUDScore(BlasterPlayerState->GetScore());
	SetHUDDefeats(BlasterPlayerState->GetPlayerDefeats());
	AWeapon* EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
	if (EquippedWeapon)
	{
		SetHUDAmmo(EquippedWeapon->GetAmmo(), BlasterCharacter->GetCombat()->GetCarriedAmmo());	
	} else
	{
		SetHUDAmmo(0, 0);
	}
	SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
	SetHUDGrenades(BlasterCharacter->GetCombat()->GetGrenades());
	// SetHUDShield()
	bInitialize = true;
}

void ABlasterPlayerController::OnRep_MatchState()
{
	DebugUtil::PrintMsg(this, FString::Printf(TEXT("OnRep_MatchState, Controller Name is %s"), *GetName()));
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (MatchState == MatchState::WaitingToStart)
	{
		HandleMatchWaitToStart();
	}
	else if (MatchState == MatchState::InProgress)
	{
		HandleMatchStart(bShowTeamScore);
	} else if (MatchState == MatchState::CoolDown)
	{
		HandleMatchCoolDown();
		MatchDuration = GetServerTime();
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
	SingleTripTime = 0.5f * RTT;
	float CurrentServerTime = ServerReceivedRequestTime + (0.5f * RTT);
	ServerClientDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}
