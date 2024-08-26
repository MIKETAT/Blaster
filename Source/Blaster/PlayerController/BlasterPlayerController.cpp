#include "BlasterPlayerController.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/HUD/Announcement.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Net/UnrealNetwork.h"

float ABlasterPlayerController::GetServerTime()
{
	if (HasAuthority())		return GetWorld()->GetTimeSeconds();
	return GetWorld()->GetTimeSeconds() + ServerClientDelta;
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD)
	{
		BlasterHUD->AddAnnouncement();
	}
}

void ABlasterPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	SetHUDTime();
	if (IsLocalController() && TimeSyncDuration > SyncFrequency)
	{
		RequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncDuration = 0.f;
	}
	PollInit();
	//UE_LOG(LogTemp, Error, TEXT("ticking, state %s"), *MatchState.ToString());
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

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("Ammo : %d"), Ammo);
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

// todo Ammo/CarriedAmmo 使用同一个TextBlock实现
void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
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
}

void ABlasterPlayerController::SetHUDCountDown(int32 CountDown)
{
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->MatchCountDownText;
	if (bHUDValid)
	{
		int32 Minute = CountDown / 60;
		int32 Second = CountDown % 60;
		FString CountDownText = FString::Printf(TEXT("%02d:%02d"), Minute, Second);
		BlasterHUD->CharacterOverlay->MatchCountDownText->SetText(FText::FromString(CountDownText));
	}
}

void ABlasterPlayerController::SetHUDTime()
{
	int32 TimeLeft = FMath::CeilToInt(MatchTime - GetServerTime());
	if (CountDownTime != TimeLeft)
	{
		// 更新HUD
		SetHUDCountDown(TimeLeft);
	}
	CountDownTime = TimeLeft;
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

void ABlasterPlayerController::SetMatchState(FName State)
{
	MatchState = State;
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	
	if (MatchState == MatchState::WaitingToStart)
	{
		if (BlasterHUD)
		{
			BlasterHUD->AddAnnouncement();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		HandleMatchStart();
	}
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
		UE_LOG(LogTemp, Error, TEXT("OnRep_MatchState, state == waittostart"));
		if (BlasterHUD)
		{
			BlasterHUD->AddAnnouncement();
		} else
		{
			UE_LOG(LogTemp, Error, TEXT("OnRep_MatchState, BlasterHUD is null"));
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		HandleMatchStart();
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
