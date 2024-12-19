// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadWidget.h"

#include "Blaster/BlasterComponent/CombatComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	/*
	FString RemoteRole;
	switch (InPawn->GetRemoteRole())
	{
	case ENetRole::ROLE_Authority:
		RemoteRole = FString("RemoteRole: Authority");
		break;
	case ENetRole::ROLE_AutonomousProxy:
		RemoteRole = FString("RemoteRole: AutonomousProxy");
		break;
	case ENetRole::ROLE_SimulatedProxy:
		RemoteRole = FString("RemoteRole: SimulatedProxy");
		break;
	case ENetRole::ROLE_None:
		RemoteRole = FString("RemoteRole: None");
		break;
	}

	FString Role;
	switch (InPawn->GetLocalRole())
	{
	case ENetRole::ROLE_Authority:
		Role = FString("Role: Authority");
		break;
	case ENetRole::ROLE_AutonomousProxy:
		Role = FString("Role: AutonomousProxy");
		break;
	case ENetRole::ROLE_SimulatedProxy:
		Role = FString("Role: SimulatedProxy");
		break;
	case ENetRole::ROLE_None:
		Role = FString("Role: None");
		break;
	}

	ABlasterPlayerState* BPState = Cast<ABlasterPlayerState>(InPawn->GetPlayerState());
	
	FString RoleText;
	if (!BPState)
	{
		RoleText = FString("BPState is null");
	}
	else if (BPState->GetTeam() == ETeam::ET_TeamBlue)
	{
		RoleText = FString("Team:Blue");
	} else if (BPState->GetTeam() == ETeam::ET_TeamRed) {
		RoleText = FString("Team: Red");
	} else
	{
		RoleText = FString("No Team");
	}

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
	FString EquipWeaponName = FString::Printf(TEXT("Default"));
	if (BlasterCharacter && BlasterCharacter->GetCombat() && BlasterCharacter->GetCombat()->GetEquippedWeapon() != nullptr)
	{
		EquipWeaponName = BlasterCharacter->GetCombat()->GetEquippedWeapon()->GetWeaponName().ToString();
	}
	*/
	if (ABlasterCharacter* MyCharacter = Cast<ABlasterCharacter>(InPawn))
	{
		SetDisplayText(MyCharacter->GetName());	
	} else
	{
		SetDisplayText(FString::Printf(TEXT("NoName")));	
	}
}

void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();
	Super::NativeDestruct();
}
