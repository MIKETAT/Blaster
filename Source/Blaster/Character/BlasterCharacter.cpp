#include "BlasterCharacter.h"

#include "Blaster/BlasterComponent/CombatComponent.h"
#include "Blaster/BlasterComponent/BuffComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include <Kismet/KismetMathLibrary.h>
#include <Sound/SoundCue.h>

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Blaster/Blaster.h"
#include "Blaster/BlasterGameState.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

ABlasterCharacter::ABlasterCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CamreaBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->SetWalkableFloorAngle(50.f);

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);
	OverheadWidget->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	OverheadWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);
	
	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);
	
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);

	// Camera Collision
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 900.f);
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Grenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();
	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();	
	}
}

void ABlasterCharacter::MulticastGainTheLead_Implementation()
{
	if (CrownSystem == nullptr)	return;
	if (CrownComponent == nullptr)
	{
		CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			CrownSystem,
			GetCapsuleComponent(),
			FName(),
			GetActorLocation() + FVector(0.f, 0.f, 100.f),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false);
	}
	if (CrownSystem)
	{
		CrownComponent->Activate();
	}
}

void ABlasterCharacter::MulticastLostTheLead_Implementation()
{
	if (CrownComponent)
	{
		CrownComponent->Deactivate();	
	}
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	// tood 在哪可以set
	GetMovementComponent()->NavAgentProps.bCanCrouch = true;

	// init
	Health = MaxHealth;
	if (Combat == nullptr)
	{
		Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	}
	UpdateHealthHUD();
	UpdateShieldHUD();
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
	}
	if (AttachedGrenade)
	{
		AttachedGrenade->SetVisibility(false);
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// 1. 使用AimOffset  only for player controlling the character
	// 2. 使用SImProxyTurn   for simProxy or character not controlled on server 
	if (GetLocalRole() > ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);	
	} else
	{
		TimeSinceLastReplication += DeltaTime;
		if (TimeSinceLastReplication > 0.3f)	// call OnRep_ReplicatedMovement every 0.3 seconds
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();	// but calc pitch every frame
	}
	if (IsLocallyControlled())
	{
		HideCharacterIfCameraClose();
	}
	// initialization
	PollInit();
	UpdateHealthHUD();
	UpdateShieldHUD();
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
	if (Buff)
	{
		Buff->Character = this;
		Buff->SetInitialSpeed(GetCharacterMovement()->MaxWalkSpeed,
							GetCharacterMovement()->MaxWalkSpeedCrouched);
		Buff->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity);
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* OverlapWeapon)
{
if (OverlappingWeapon)
	{
		 OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = OverlapWeapon;
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool ABlasterCharacter::isWeaponEquipped() const
{
	return Combat && Combat->EquippedWeapon;
}

bool ABlasterCharacter::isAiming() const
{
	return Combat && Combat->bIsAiming;
}

AWeapon* ABlasterCharacter::GetEquippedWeapon() const
{
	if (Combat == nullptr)	return nullptr;
	return Combat->EquippedWeapon;
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (!Combat || !Combat->EquippedWeapon)	return;
	//PlayAnimMontage(FireWeaponMontage);
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayReloadMontage()
{
	if (!Combat || !Combat->EquippedWeapon)	return;
	//PlayAnimMontage(FireWeaponMontage);
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
			case EWeaponType::EWT_AssaultRifle:
				SectionName = FName("Rifle");
				break;
		case EWeaponType::EWT_RocketLauncher:
				SectionName = FName("RocketLauncher");	// 
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = FName("Pistol");	
			break;
		case EWeaponType::EWT_SubmachineGun:
			SectionName = FName("Pistol");	// Submachinegun 和 pistol 一样的换弹动画
			break;
		case EWeaponType::EWT_Shotgun:
			SectionName = FName("Shotgun");
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = FName("SniperRifle");
			break;
		case EWeaponType::EWT_GrenadeLauncher:
			SectionName = FName("GrenadeLauncher");
			break;
		default:
			SectionName = FName("Rifle");
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void ABlasterCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if (Combat == nullptr)	return FVector();
	return Combat->HitTarget;
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	if (Combat == nullptr)	return ECombatState::ECS_UnOccupied;
	return Combat->CombatState;
}

float ABlasterCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void ABlasterCharacter::UpdateHealthHUD()
{
	BlasterPlayerController = GetBlasterPlayerController();
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::UpdateShieldHUD()
{
	BlasterPlayerController = GetBlasterPlayerController();
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDShield(Shield, MaxShield);
	}
}

void ABlasterCharacter::DropOrDestroyWeapons()
{
	if (Combat)
	{
		Combat->DropOrDestroyWeapon(Combat->EquippedWeapon);
		Combat->DropOrDestroyWeapon(Combat->SecondaryWeapon);
	}
}

void ABlasterCharacter::Elim(bool bPlayerLeftGame)
{
	DropOrDestroyWeapons();
	MulticastElim(bPlayerLeftGame);
}

// deal with initialization
void ABlasterCharacter::PollInit()
{
	if (BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState)
		{
			BlasterPlayerState->AddToScore(0.f);
			BlasterPlayerState->AddToDefeats(0);

			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			if (BlasterGameState && BlasterGameState->TopScorePlayer.Contains(BlasterPlayerState))
			{
				MulticastGainTheLead();
			}
		}
	}
}

void ABlasterCharacter::ServerLeaveGame_Implementation()
{
	BlasterGameMode = GetBlasterGameMode();
	BlasterPlayerState = GetBlasterPlayerState();
	if (BlasterGameMode && BlasterPlayerState)
	{
		BlasterGameMode->PlayerLeftGame(BlasterPlayerState);
	}
	
}

void ABlasterCharacter::MulticastElim_Implementation(bool bPlayerLeftGame)
{
	bLeftGame = bPlayerLeftGame;
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDAmmo(0, 0);
	}
	bElimmed = true;
	PlayElimMontage();
	
	// play dissolve animation
	if (DissolveMaterialInstance)
	{
		// create DynamicDissolbeMaterial
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);	// todo this的作用
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), .55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	// Disable Movement
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	if (BlasterPlayerController)
	{
		DisableInput(BlasterPlayerController);
	}

	// Disable Collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Spawn Elim Bot
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation());
	}
	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation());
	}
	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}
	bool bUseSniperAiming = IsLocallyControlled() && BlasterPlayerController &&
		Combat && Combat->bIsAiming && Combat->EquippedWeapon &&
		Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle; 
	if (bUseSniperAiming)
	{
		BlasterPlayerController->SetHUDSniperScope(false);
	}
	//  为了广播玩家离开死亡这一情况，需要ElimTimerFinish在server和client上都执行
	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&ABlasterCharacter::ElimTimerFinish,
		ElimDelay);
}

// start the timeline, and bind callback(UpdateDissolveMaterial) to dissolvetrack
void ABlasterCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		// add the curve to the timeline
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		// 这里就是让timeline使用DissolveCurve， 并且会调用与DissolveTrack 绑定的callback(UpdateDissolveMaterial),传入curve的输出
		DissolveTimeline->Play();
	}
}


// as timeline playing, call this function every frame
void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ABlasterCharacter::ElimTimerFinish()
{
	// Respawn Character
	if (GetBlasterGameMode() && !bLeftGame)
	{
		GetBlasterGameMode()->RequestRespawn(this, Controller);	
	}
	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}
	if (bLeftGame && IsLocallyControlled())
	{
		OnLeftGame.Broadcast();
	}
}


void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ABlasterCharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Equip"), IE_Pressed, this, &ABlasterCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Drop"), IE_Pressed, this, &ABlasterCharacter::DropButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Pressed, this, &ABlasterCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Aim"), IE_Pressed, this, &ABlasterCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Aim"), IE_Released, this, &ABlasterCharacter::AimButtonReleased);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &ABlasterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Released, this, &ABlasterCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction(TEXT("Reload"), IE_Pressed, this, &ABlasterCharacter::ReloadButtonPressed);
	PlayerInputComponent->BindAction(TEXT("ThrowGrenade"), IE_Pressed, this, &ABlasterCharacter::GrenadeButtonPressed);
	PlayerInputComponent->BindAction(TEXT("Hover"), IE_Pressed, this, &ABlasterCharacter::HoverButtonPressed);
	
	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ABlasterCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ABlasterCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ABlasterCharacter::Turn);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ABlasterCharacter::LookUp);
}

void ABlasterCharacter::MoveForward(float Value)
{
	if (Controller != nullptr)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::MoveRight(float Value)
{
	if (Controller != nullptr)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ABlasterCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ABlasterCharacter::HoverButtonPressed()
{
	ServerHoverButtonPressed();
}

void ABlasterCharacter::ServerHoverButtonPressed_Implementation()
{
	bHovering = !bHovering;
}

void ABlasterCharacter::OnRep_bHovering()
{
	
}

void ABlasterCharacter::EquipButtonPressed()
{
	ServerEquipButtonPressed();
}

void ABlasterCharacter::DropButtonPressed()
{
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->DropWeapon();
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bIsCrouched)
	{
		UnCrouch();	
	}
	else
	{
		Crouch();
	}	
}

void ABlasterCharacter::AimButtonPressed()
{
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::ReloadButtonPressed()
{
	if (Combat)
	{
		Combat->Reload();
	}	
}

void ABlasterCharacter::GrenadeButtonPressed()
{
	if (Combat)
	{
		Combat->ThrowGrenade();
	}	
}

void ABlasterCharacter::CalculateAO_Pitch()
{
	// Pitch 不受影响
	AO_Pitch = GetBaseAimRotation().Pitch;	// 网络同步时压缩成 unsigned 类型
	if (AO_Pitch > 90.f ) {
		// map pitch from [270, 360) to [-90, 0)
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0);
		// serilized -> compressed -> sent to network -> decompressed to [0, 360) range
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

/**
 * Calculate AO_YAW, should be used when we are not a simulated proxy
 */
void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;
	// 只在idle的时候生效
	bool isInAir = GetCharacterMovement()->IsFalling();
	float Speed = CalculateSpeed();
	if (Speed == 0.f && !isInAir) {
		bRotateRootBone = true;	// only when we are standing and not jumping
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAnimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		bUseControllerRotationYaw = true;
		// Set TurningInPlace
		TurnInPlace(DeltaTime);		// 不运动的时候，根据aimoffset的yaw转身
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
	}
	// store base anim rotation
	// 移动时使用controller 的 Yaw
	if (Speed > 0.f || isInAir) {	// running or jumping
		bRotateRootBone = false;	// shouldn't rotate root bone
		StartingAnimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;	// 运动的时候不转身
	}
	
	CalculateAO_Pitch();
}

/**
* Handle Turning for sim proxies, because sim proxies does not call rotate root bone evert frame
* Play turn animation when turing enough
  在SimProxy上， 由于NetUpdate不能像 tick 一样频繁，所以这个函数无法每个tick都更新
  我们需要在SImProxy的Movement更新时call this function
  we can use OnRep_ReplicatedMovement instead of tick function when checking the delta of simProxy' character rotation
*/
void ABlasterCharacter::SimProxiesTurn()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)	return;
	// in sim proxies, we don't use rotate root bone
	bRotateRootBone = false;

	float Speed = CalculateSpeed();
	if (Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;	// Runing 不需要转身动画
		return;;
	}
	
	ProxyRotationLastFrame = ProxyCurrentRotation;
	ProxyCurrentRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotationLastFrame, ProxyCurrentRotation).Yaw;
	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		} else if (ProxyYaw < TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		} else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	} else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 5.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAnimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ABlasterCharacter::Jump()
{
	if (bIsCrouched)
	{
		UnCrouch();
	} else
	{
		Super::Jump();
	}
}

void ABlasterCharacter::FireButtonPressed()
{
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
	/*// debug delete in one sec
	AGameModeBase* mode = UGameplayStatics::GetGameMode(this);
	if (GEngine && mode)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
			FString::Printf(TEXT("GameMode is %s"), *mode->GetName()));
	}*/
}

void ABlasterCharacter::FireButtonReleased()
{
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}	
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (!Combat || !Combat->EquippedWeapon)	return;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName = FName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

// bind this to delegate onTakenAnyDamage, and only on the server
void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatorController, AActor* DamageCauser)
{
	if (bElimmed)	return;
	float DamageToApply = Damage;
	if (Shield >= DamageToApply)
	{
		Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);
		DamageToApply = 0.f;
	} else
	{
		DamageToApply = FMath::Clamp(DamageToApply - Shield, 0.f, Damage);
		Shield = 0.f;
	}
	Health = FMath::Clamp(Health - DamageToApply, 0.f, MaxHealth);
	UpdateHealthHUD();
	PlayHitReactMontage();
	if (Health > 0.f)	return;
	if (GetBlasterGameMode())
	{
		ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
		GetBlasterGameMode()->PlayerEliminated(this, GetBlasterPlayerController(), AttackerController);	
	} 
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (!Combat)	return;
	if (Combat->ShouldSwapWeapon())
	{
		Combat->SwapWeapons();
	} else if (OverlappingWeapon)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}


void ABlasterCharacter::OnRep_Health(float LastHealth)
{
	// Health改变，set HealthHUD and play HitMontage
	UpdateHealthHUD();
	if (LastHealth > Health)
	{
		PlayHitReactMontage();	
	}
}

void ABlasterCharacter::OnRep_Shield(float LastShield)
{
	UpdateShieldHUD();
	if (LastShield > Shield)
	{
		PlayHitReactMontage();
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastOverlapWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastOverlapWeapon)
	{
		LastOverlapWeapon->ShowPickupWidget(false);
	}
}

/**
*	如果停止移动则不会调用SimProxiesTurn
*	需要间隔一段时间去检查，超过阈值调用SimProxiesTurn
*/
void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastReplication = 0.f;
}

void ABlasterCharacter::SetCharacterVisibility(bool bVisiable)
{
	if (GetMesh() == nullptr)	return;
	GetMesh()->SetVisibility(bVisiable);
	if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
	{
		Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = !bVisiable;
	}
}

void ABlasterCharacter::HideCharacterIfCameraClose()
{
	if (!IsLocallyControlled() && FollowCamera == nullptr)		return;
	float distance = (FollowCamera->GetComponentLocation() - GetActorLocation()).Size();
	if (distance < CameraThreshold)
	{
		SetCharacterVisibility(false);
	} else
	{
		SetCharacterVisibility(true);
	}
}


  