// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileGrenade.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Utils/DebugUtil.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

AProjectileGrenade::AProjectileGrenade()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Grenade Mesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->bShouldBounce = true;
}

void AProjectileGrenade::Destroyed()
{
	ExplodeDamage();
	Super::Destroyed();
}

void AProjectileGrenade::ClearDestroyTimer()
{
	GetWorldTimerManager().ClearTimer(DestroyTimer);
}

void AProjectileGrenade::InstantExplode()
{
	ClearDestroyTimer();
	Destroy();
}

void AProjectileGrenade::BeginPlay()
{
	AActor::BeginPlay();

	/*DebugUtil::PrintMsg(this, FString::Printf(TEXT("Grenade Start Location is %s"), *GetActorLocation().ToString()));
	DrawDebugSphere(GetWorld(), GetActorLocation(), 10.f, 8, FColor::Blue, false, 20.f);*/
	StartDestroyTimer();
	SpawnTrailSystem();
	ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &AProjectileGrenade::OnBounce);
}

void AProjectileGrenade::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	//DrawDebugPoint(GetWorld(), GetActorLocation(), 3.f, FColor::Red, true, 5.f);
}

void AProjectileGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	/*
	DebugUtil::PrintMsg(this, FString::Printf(TEXT("%s Can Bounce %d"), *GetName(), BoundTimes));
	DebugUtil::PrintMsg(this, FString::Printf(TEXT("ImpactResult.BoneName = %s"), *ImpactResult.GetActor()->GetName()));
	DebugUtil::PrintMsg(this, FString::Printf(TEXT("ImpactVelocity.Length() = %f"), ImpactVelocity.Length()));
	
	if (ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(ImpactResult.GetActor()))
	{
		DebugUtil::PrintMsg(this, FString::Printf(TEXT("HitCharacter = %s"), *HitCharacter->GetName()));
		DebugUtil::PrintMsg(this, FString::Printf(TEXT("HitBone = %s"), *ImpactResult.BoneName.ToString()));
	}
	*/
	BoundTimes--;
	if (BounceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			BounceSound,
			GetActorLocation());
	}
	/*if (ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(ImpactResult.GetActor()))
	{
		InstantExplode();
	}
	if (BoundTimes == 0)
	{
		InstantExplode();
	}*/
}

