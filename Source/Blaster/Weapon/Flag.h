// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Flag.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AFlag : public AWeapon
{
	GENERATED_BODY()
public:
	AFlag();
	virtual void Drop() override;
	void SetFlagPhysicsAndCollision(bool bEnable);
	void ResetFlag();
protected:
	virtual void BeginPlay() override;
	virtual void OnEquip() override;
	virtual void OnDrop() override;
private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* FlagMesh;
	
	UPROPERTY()
	FTransform InitialTrans;
};
