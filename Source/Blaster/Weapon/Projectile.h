// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	UPROPERTY()
	float Damage = 20.f;		//
	
	/**
	 *	Server side Rewind
	 * 
	 */
	bool bUseServerSideRewind = false;
	FVector TraceStart;
	FVector_NetQuantize100 InitialVelocity;
	
	UPROPERTY(EditAnywhere)
	float InitialSpeed = 15000.f;
protected:
	virtual void BeginPlay() override;
	void StartDestroyTimer();
	void DestroyTimerFinish();
	void SpawnTrailSystem();
	void ExplodeDamage();
	// 这些callback都必须是 UFUNCTION
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpluse, const FHitResult& Hit);
	
	FTimerHandle DestroyTimer;

	UPROPERTY(EditAnywhere)
	float DestroyTime;
	
	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer;

	UPROPERTY()
	class UParticleSystemComponent* TracerComponent;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;
	
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* ProjectileMesh;
	
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem;

	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;

private:
	UPROPERTY(EditAnywhere)
	float DamageInnerRadius = 200.f;
	
	UPROPERTY(EditAnywhere)
	float DamageOuterRadius = 500.f;
};
