// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"

#include "KismetTraceUtils.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "Camera/CameraComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundCue.h"

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
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);	//只需要复制给持有者即可，无需复制给其他玩家，节省带宽
	// todo condition用法详解
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
	if (Character->HasAuthority())
	{
		InitWeaponAmmo();
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
		DrawDebugPoint(
			GetWorld(),
			HitTarget,
			15.f,
			FColor::Purple);
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

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

// on client
void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
		case ECombatState::ECS_UnOccupied:
			// 切回到UnOccupied，此时开火键按住的话就开火
			if (bFireButtonPressed)
			{
				Fire();
			}
			break;
		case ECombatState::ECS_Reloading:
			HandleReload();
			break;
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
	// 无武器 || 武器无法开火(此刻不能开火/没子弹) || 当前状态不能开火
	if (EquippedWeapon == nullptr || !EquippedWeapon->CanFire() || CombatState != ECombatState::ECS_UnOccupied)	return;
	// todo 设置逻辑，子弹数为0时开火发出卡壳的声音
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
	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

// todo reload的动画需要改，只上半身reload，不影响下半身，不会改变状态(蹲下->站起)这种
void UCombatComponent::HandleReload()
{
	CombatState = ECombatState::ECS_Reloading;
	Character->PlayReloadMontage();
}

// on server
void UCombatComponent::UpdateCarriedAmmo()
{
	if (Character == nullptr || EquippedWeapon == nullptr)	return;
	int32 ReloadAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo -= ReloadAmount;
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDCarriedAmmo(CarriedAmmo);
		}
		EquippedWeapon->AddAmmo(ReloadAmount);
	}
}

int32 UCombatComponent::AmountToReload()
{
	if (Character == nullptr || EquippedWeapon == nullptr)	return 0;
	int32 RoomForReload = EquippedWeapon->GetMaxCapacity() - EquippedWeapon->GetAmmo();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 CarriedWeaponAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(CarriedWeaponAmmo, RoomForReload);
		return FMath::Clamp(RoomForReload, 0, Least);	
	}
	return 0;
}

void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading)
	{
		ServerReload();
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (Character == nullptr || EquippedWeapon == nullptr)	return;
	HandleReload();
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr)	return;
	if (Character && CombatState == ECombatState::ECS_UnOccupied)
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
		FCollisionQueryParams Params;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(GetOwner());
		// todo 这里射线检测添加了IgnoreActor, 但是Pitch过大会使动画不正确，考虑限制contorller的rotation
		bool isHit = GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECC_Visibility,
			QueryParams
			);
		// todo 想想这里未击中的话如何处理，让人物持枪动画正常
		// 未击中，设置HitResult的ImpactPoint
		if (!isHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>() && TraceHitResult.GetActor() != this->Character)
		{
			DrawDebugPoint(GetWorld(), TraceHitResult.ImpactPoint, 5.f, FColor::Orange);
			// crosshair hit character, change hud color to red
			HUDPackage.CrosshairsColor = FLinearColor::Red;
		} else
		{
			HUDPackage.CrosshairsColor = FLinearColor::White;
		}
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

		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDCarriedAmmo(CarriedAmmo);
		}
		
		if (EquippedWeapon->EquipSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				EquippedWeapon->EquipSound,
				EquippedWeapon->GetActorLocation()
			);
		}
	}
}

// todo 是否要设置初始自身携带弹药
// todo 是否可以根据enum WeaponType 遍历这些类，得到每种武器的最大弹容量，作为初始值初始化
void UCombatComponent::InitWeaponAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, InitialAmmoAmount);
	
}

// only on server
void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquipped)
{
	if (!Character || !WeaponToEquipped)	return;
	if (EquippedWeapon)
	{
		EquippedWeapon->Drop();	// 已有武器则扔掉替换
	}
	
	EquippedWeapon = WeaponToEquipped;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	
	// todo 为什么这里不在client执行AttachActor，client照样能完成捡起武器
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->ShowPickupWidget(false);
	EquippedWeapon->SetHUDAmmo();
	if (EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquippedWeapon->EquipSound,
			EquippedWeapon->GetActorLocation()
		);
	}
	
	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}
	
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;	// 禁用面向移动方向
	Character->bUseControllerRotationYaw = true;	// Pawn的yaw等于controller的yaw
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr)	return;
	// 重要变量的变化，例如状态的变化都在server执行
	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_UnOccupied;
		UpdateCarriedAmmo();
	}
	// 换弹完成，仍按住了开火
	if (bFireButtonPressed)
	{
		Fire();
	}
}
