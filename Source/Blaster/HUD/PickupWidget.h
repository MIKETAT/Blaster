// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blaster/Weapon/Weapon.h"
#include "PickupWidget.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UPickupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void SetupPickupText(AWeapon* OwnerWeapon);

protected:
	virtual void NativeDestruct() override;
private:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* PickupText;
	
};
