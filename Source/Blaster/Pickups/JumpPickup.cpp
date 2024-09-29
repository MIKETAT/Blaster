// Fill out your copyright notice in the Description page of Project Settings.


#include "JumpPickup.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/BlasterComponent/BuffComponent.h"


void AJumpPickup::OnSphereOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor,
                                  UPrimitiveComponent* OtherComponent, int32 OtherIndex, bool mFromSweep, const FHitResult& SweepHitResult)
{
	Super::OnSphereOverlap(OverlapComponent, OtherActor, OtherComponent, OtherIndex, mFromSweep, SweepHitResult);
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		UBuffComponent* Buff = BlasterCharacter->GetBuff();
		if (Buff)
		{
			Buff->BuffJump(JumpZVelocityBuff, JumpBuffTime);
		}
	}
	Destroy();
}
