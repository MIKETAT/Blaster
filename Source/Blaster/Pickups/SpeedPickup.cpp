// Fill out your copyright notice in the Description page of Project Settings.


#include "SpeedPickup.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/BlasterComponent/BuffComponent.h"


void ASpeedPickup::OnSphereOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComponent, int32 OtherIndex, bool mFromSweep, const FHitResult& SweepHitResult)
{
	Super::OnSphereOverlap(OverlapComponent, OtherActor, OtherComponent, OtherIndex, mFromSweep, SweepHitResult);
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		UBuffComponent* Buff = BlasterCharacter->GetBuff();
		if (Buff)
		{
			Buff->BuffSpeed(BaseSpeedBuff, CrouchSpeedBuff, SpeedBuffTime);
		}
	}
	Destroy();
}
