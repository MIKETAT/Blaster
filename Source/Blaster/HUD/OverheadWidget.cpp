// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadWidget.h"

#include "Components/TextBlock.h"

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
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

	//FString PlayerName = InPawn->GetPlayerState()->GetPlayerName();
	//FString RemoteRoleString = FString::Printf(TEXT("Remote Role is %s, and PlayerName is %s"), *Role, *PlayerName);
	//FString RemoteRoleString = FString::Printf(TEXT("Remote Role is %s"), *RemoteRole);
	FString RoleText = FString::Printf(TEXT("%s / %s"), *RemoteRole, *Role);
	SetDisplayText(RoleText);
}

void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();
	Super::NativeDestruct();
}
