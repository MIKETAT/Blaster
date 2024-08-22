// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"

#include "KismetTraceUtils.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/Weapon/Weapon.h"
#include "Camera/CameraComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 400.f;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bIsAiming);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;
		InterpFOV(DeltaTime);
		SetHUDCrosshairs(DeltaTime);
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr)	return;
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;
		if (HUD && EquippedWeapon)
		{
			HUDPackage.CrosshairCenter = EquippedWeapon->CrosshairCenter;
			HUDPackage.CrosshairLeft = EquippedWeapon->CrosshairLeft;
			HUDPackage.CrosshairRight = EquippedWeapon->CrosshairRight;
			HUDPackage.CrosshairTop = EquippedWeapon->CrosshairTop;
			HUDPackage.CrosshairBottom = EquippedWeapon->CrosshairBottom;
			// calc corsshair spread
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			// todo crouching crosshair spread
			FVector2D VelocityMultiplierRange(0.f, 1.f);	// [0, 1]
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());
			if (Character->GetMovementComponent()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2, DeltaTime, 2.f);
			} else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0, DeltaTime, 30.f);
			}
			if (bIsAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.6f, DeltaTime, 30.f);
			} else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}
			// always interp AimFactor to zero
			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 40.f);
			HUDPackage.CrosshairSpread = CrosshairVelocityFactor +
									  CrosshairInAirFactor -
									  CrosshairAimFactor +
									  CrosshairShootingFactor + 0.5f;
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

/**
 *	Fire Section
 */
// 自动武器，bFireButtonPressed 会在一段时间内为true。 而repolicated 只会在变化时复制。因此无法连续播放动画
void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::Fire()
{
	if (EquippedWeapon == nullptr || EquippedWeapon->CanFire() == false)	return;
	EquippedWeapon->SetWeaponFireStatus(false);	// 开火执行时不能同时开火
	ServerFire(HitTarget);
	CrosshairShootingFactor = FMath::Max(CrosshairShootingFactor + 0.2f, 1.f);
	StartFireTimer();
}

void UCombatComponent::StartFireTimer()
{
	if (Character == nullptr || EquippedWeapon == nullptr)	return;
	Character->GetWorldTimerManager().SetTimer(
		EquippedWeapon->GetFireTimer(),
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->GetFireDelay());
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr)	return;
	EquippedWeapon->SetWeaponFireStatus(true);
	// 如果是自动武器且仍在开火
	if (EquippedWeapon->CanAutomaticFire() && bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr)	return;
	if (Character)
	{
		Character->PlayFireMontage(bIsAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}
/**
 * Aiming Section
 */
void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	// trace from center of the screen
	FVector2d ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	FVector2d CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);	// screen space
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	// for each machine, player 0 is the player that's playing the game
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		//Controller,
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition, CrosshairWorldDirection);
	if (bScreenToWorld)
	{
		// performin g linetrace
		FVector Start = CrosshairWorldPosition;
		// linetrace should begin in front of character, avoid shooting behind the character
		float CameraDistance = (Start - Character->GetActorLocation()).Size();
		Start += CrosshairWorldDirection * CameraDistance;
		
		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECC_Visibility);

		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>() && TraceHitResult.GetActor() != this->Character)
		{
			DrawDebugPoint(GetWorld(), TraceHitResult.ImpactPoint, 5.f, FColor::Orange);
			// crosshair hit character, change hud color to red
			HUDPackage.CrosshairsColor = FLinearColor::Red;
		} else
		{
			HUDPackage.CrosshairsColor = FLinearColor::White;
		}
	} else
	{
		UE_LOG(LogTemp, Error, TEXT("bScreenToWorld fasle"));
	}
}

void UCombatComponent::SetAiming(bool isAiming)
{
	bIsAiming = isAiming;	// set before rpc and replicated
	ServerSetAiming(bIsAiming);
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool isAiming)
{
	bIsAiming = isAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (Character == nullptr)	return;
	if (bIsAiming && EquippedWeapon)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	} else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed); // UnZoomSpeed 各个武器一样
	}
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

/**
 * NetWork
 */
void UCombatComponent::OnRep_EquippedWeapon()
{
	if (Character && EquippedWeapon)
	{
		// TODO 为什么下面这段不加上，client依然可以执行捡起武器。
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}
		
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;	// 禁用面向移动方向
		Character->bUseControllerRotationYaw = true;	// Pawn的yaw等于controller的yaw
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquipped)
{
	if (!Character || !WeaponToEquipped)	return;
	// 假设已经有武器，不捡起(暂定，后续可改为更换武器)
	if (Character->isWeaponEquipped())		return;
	
	EquippedWeapon = WeaponToEquipped;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	// todo 为什么这里不在client执行AttachActor，client照样能完成捡起武器
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->ShowPickupWidget(false);
	
	
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;	// 禁用面向移动方向
	Character->bUseControllerRotationYaw = true;	// Pawn的yaw等于controller的yaw
}
