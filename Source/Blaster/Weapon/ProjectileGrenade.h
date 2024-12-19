// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileGrenade.generated.h"

UCLASS()
class BLASTER_API AProjectileGrenade : public AProjectile
{
	GENERATED_BODY()
public:
	AProjectileGrenade();
	virtual void Destroyed() override;
	void ClearDestroyTimer();
	void InstantExplode();
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);
private:
	UPROPERTY(EditAnywhere)
	USoundCue* BounceSound;

	UPROPERTY(EditAnywhere)
	int32 BoundTimes = 5;
};
