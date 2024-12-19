// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"

#include "WeaponTypes.h"
#include "Blaster/BlasterComponent/LagCompensationComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Utils/DebugUtil.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AHitScanWeapon::Fire(const FVector& HitTarget) 
{
	Super::Fire(HitTarget);
	APawn* InstigatorOwner = Cast<APawn>(GetOwner());
	if (InstigatorOwner == nullptr)	return;
	APlayerController* InstigatorController = Cast<APlayerController>(InstigatorOwner->GetController());
	
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();
		FVector End = Start + (HitTarget - Start) * 1.25f;
		FHitResult HitResult;
		UWorld* World = GetWorld();
		if (!World)		return;
		WeaponHit(Start, End, HitResult);
		//DebugUtil::PrintMsg(HitResult.BoneName, FColor::Blue);
		if (HitResult.bBlockingHit)
		{
			// 命中人物
			ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(HitResult.GetActor());
			// On SimulateProxy 上 Controller为nullptr， 只在ApplyDamage上进行检查
			if (HitCharacter && InstigatorController)
			{
				float DamageToCause = HitResult.BoneName == FString("head") ? HeadShotDamage : Damage;
				// todo 这里的四种情况, server/client + rewind/not rewind 需要考虑, 特别是 server上使用rewind 或者client 不使用rewind两种
				bool bUseAuthDamage = !bUseServerSideRewind || BlasterOwner->IsLocallyControlled();
				if (HasAuthority() && bUseAuthDamage)	
				{
					//DebugUtil::PrintMsg(TEXT("Apply Damage Directly"));
					UGameplayStatics::ApplyDamage(
					HitCharacter,
					DamageToCause,
					InstigatorController,
					this,
					UDamageType::StaticClass());
					//DebugUtil::PrintMsg(FString::Printf(TEXT("Cause Damage %f"), DamageToCause), FColor::Red);
				}
				if (!HasAuthority() && bUseServerSideRewind)  // client and use ServerSideRewind
				{
					BlasterOwner = BlasterOwner == nullptr ? Cast<ABlasterCharacter>(InstigatorOwner) : BlasterOwner;
					BlasterController = BlasterController == nullptr ? Cast<ABlasterPlayerController>(InstigatorController) : BlasterController;
					if (BlasterOwner && BlasterController && BlasterOwner->GetLagCompensationComponent() && BlasterOwner->IsLocallyControlled())
					{
						//DebugUtil::PrintMsg(TEXT("Apply Damage Using Server Request"));
						BlasterOwner->GetLagCompensationComponent()->ServerRequestScore(
							HitCharacter,
							Start,
							HitTarget,
							BlasterController->GetServerTime() - BlasterController->GetSingleTripTime(),
							this);
					}
				}
			}
			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(
					World,
					ImpactParticles,
					HitResult.ImpactPoint,
					HitResult.ImpactNormal.Rotation());
			}
			if (HitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(
					World,
					HitSound,
					HitResult.ImpactPoint);
			}
		}
		if (BeamParticle)	// 没击中也要有SmokeTrail
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BeamParticle,
				SocketTransform);
			if (Beam)
			{
				Beam->SetVectorParameter(FName("Target"), HitResult.bBlockingHit ? HitResult.ImpactPoint : End);
			}
		}
		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				World,
				MuzzleFlash,
				SocketTransform);
		}
		if (FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				World,
				FireSound,
				GetActorLocation());
		}
	}
}

void AHitScanWeapon::WeaponHit(const FVector& Start, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();
	if (!World)		return;
	FVector End = Start + (HitTarget - Start) * 1.25f;
	World->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility);
	FVector BeamEnd = End;
	if (OutHit.bBlockingHit)
	{
		BeamEnd = OutHit.ImpactPoint;
	} else
	{
		OutHit.ImpactPoint = End;
	}
	//DrawDebugSphere(GetWorld(), BeamEnd, 9.f, 10, FColor::Purple, true);
}
