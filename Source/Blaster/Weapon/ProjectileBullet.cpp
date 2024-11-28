// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"

#include "Animation/AnimInstanceProxy.h"
#include "Blaster/BlasterComponent/LagCompensationComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpluse, const FHitResult& Hit)
{
	ABlasterCharacter* OwnerCharacter = Cast<ABlasterCharacter>(GetOwner());
	if (OwnerCharacter)
	{
		ABlasterPlayerController* OwnerController = Cast<ABlasterPlayerController>(OwnerCharacter->Controller);
		if (OwnerController)
		{
			// ApplyDamage trigger a damage event, so we need a callback to bind to the damage event
			if (OwnerCharacter->HasAuthority() && !bUseServerSideRewind)
			{
				UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
				Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpluse, Hit);
				return;
			}
			ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(OtherActor);
			if (bUseServerSideRewind && OwnerCharacter && OwnerCharacter->GetLagCompensationComponent() && HitCharacter)
			{
				OwnerCharacter->GetLagCompensationComponent()->ProjectileServerRequestScore(
					HitCharacter,
					TraceStart,
					InitialVelocity,
					OwnerController->GetServerTime() - OwnerController->GetSingleTripTime());
			}
		}
	}
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpluse, Hit); // call destory, so call it last
}

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();	
}
