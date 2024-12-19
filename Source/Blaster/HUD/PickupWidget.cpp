// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupWidget.h"

#include "Blaster/Utils/DebugUtil.h"
#include "Blaster/Weapon/Weapon.h"
#include "Components/TextBlock.h"

void UPickupWidget::SetupPickupText(AWeapon* OwnerWeapon)
{
	if (OwnerWeapon == nullptr)
	{
		DebugUtil::PrintMsg(TEXT("OwnerWeapon == nullptr"));
	}
	FString WeaponName = OwnerWeapon->GetWeaponName().ToString();
	FString ShowText = FString::Printf(TEXT("Press E to pick up %s"), *WeaponName);
	PickupText->SetText(FText::FromString(ShowText));
}

void UPickupWidget::NativeDestruct()
{
	RemoveFromParent();
	Super::NativeDestruct();
}
