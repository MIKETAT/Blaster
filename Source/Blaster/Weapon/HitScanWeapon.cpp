// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"

#include "WeaponTypes.h"
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
		/*World->LineTraceSingleByChannel(
			HitResult,
			Start,
			End,
			ECC_Visibility);*/
		WeaponHit(Start, End, HitResult);
		DebugUtil::PrintMsg(HitResult.BoneName, FColor::Blue);
		if (HitResult.bBlockingHit)
		{
			// 命中人物
			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(HitResult.GetActor());
			// On SimulateProxy 上 Controller为nullptr， 只在ApplyDamage上进行检查
			if (BlasterCharacter && HasAuthority() && InstigatorController)
			{
				float DamageToCause = HitResult.BoneName == FString("head") ? HeadShotDamage : Damage;
				UGameplayStatics::ApplyDamage(
					BlasterCharacter,
					DamageToCause,
					InstigatorController,
					this,
					UDamageType::StaticClass());	
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
		} else
		{
			// 
		}
		
		// 没击中也要有SmokeTrail
		if (BeamParticle)
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

FVector AHitScanWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& TraceTarget)
{
	FVector ToTargetNormalized = (TraceTarget - TraceStart).GetSafeNormal();
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	FVector RandomVec = UKismetMathLibrary::RandomUnitVector() * FMath::RandRange(0.f, SphereRadius);
	FVector RandomEnd = SphereCenter + RandomVec;
	FVector ToTraceEnd = RandomEnd - TraceStart;
	return FVector(TraceStart + TRACE_LENGTH * ToTraceEnd / ToTraceEnd.Size());
}

void AHitScanWeapon::WeaponHit(const FVector& Start, const FVector& HitTarget, FHitResult& OutHit)
{
	DrawDebugPoint(GetWorld(), Start, 9.f, FColor::Purple, true);
	FVector End = bUseScatter ? TraceEndWithScatter(Start, HitTarget) :  Start + (HitTarget - Start) * 1.25f;
	UWorld* World = GetWorld();
	if (World == nullptr)		return;
	World->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility);
}
