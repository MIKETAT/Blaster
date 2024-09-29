// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthPickup.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/BlasterComponent/BuffComponent.h"

AHealthPickup::AHealthPickup()
{
	bReplicates = true;
}

void AHealthPickup::OnSphereOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherIndex, bool mFromSweep, const FHitResult& SweepHitResult)
{
	Super::OnSphereOverlap(OverlapComponent, OtherActor, OtherComponent, OtherIndex, mFromSweep, SweepHitResult);
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		UBuffComponent* Buff = BlasterCharacter->GetBuff();
		if (Buff)
		{
			Buff->Heal(HealAmount, HealingTime);
		}
	}
	Destroy();
}
