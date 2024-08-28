// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileRocket.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectileRocket::AProjectileRocket()
{
	// 碰撞由CollisionBox完成
	ProjectileMovementComponent->InitialSpeed = 6000.f;
	ProjectileMovementComponent->MaxSpeed = 6000.f;
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpluse, const FHitResult& Hit)
{
	APawn* FiringPawn = GetInstigator();
	if (FiringPawn)
	{
		AController* InstigatorController = FiringPawn->GetController();
		if (InstigatorController)
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,
				120.f,
				10.f,
				GetActorLocation(),
				200.f,
				500.f,
				1.f,
				UDamageType::StaticClass(),
				TArray<AActor*>(),
				this,
				InstigatorController);
		}
	}
	
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpluse, Hit);
}
