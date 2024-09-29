// Fill out your copyright notice in the Description page of Project Settings.


#include "ShieldPickup.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/BlasterComponent/BuffComponent.h"

AShieldPickup::AShieldPickup()
{
	bReplicates = true;
}

void AShieldPickup::OnSphereOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor,
                                    UPrimitiveComponent* OtherComponent, int32 OtherIndex, bool mFromSweep, const FHitResult& SweepHitResult)
{
	Super::OnSphereOverlap(OverlapComponent, OtherActor, OtherComponent, OtherIndex, mFromSweep, SweepHitResult);
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		UBuffComponent* Buff = BlasterCharacter->GetBuff();
		if (Buff)
		{
			Buff->ReplenishShield(ShieldAmount, ShieldReplenishTime);
		}
	}
	Destroy();
}
