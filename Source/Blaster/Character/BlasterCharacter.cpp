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
#include "Blaster/BlasterComponent/LagCompensationComponent.h"
#include "Blaster/PlayerStart/TeamPlayerStart.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/Utils/DebugUtil.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "Components/BoxComponent.h"
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
	FollowCamera->bUsePawnControlRotation = false;	//

	ConstructHitBox();

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->SetWalkableFloorAngle(50.f);

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);
	OverheadWidget->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	OverheadWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BlasterCombatComp = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	BlasterCombatComp->SetIsReplicated(true);
	
	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensaton"));
	LagCompensation->SetIsReplicated(false);	//only on server
	
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

void ABlasterCharacter::ConstructHitBox()
{
	DebugUtil::LogMsg(FString::Printf(TEXT("Constructing Hit Box")));
	head = CreateDefaultSubobject<UBoxComponent>(TEXT("head"));
	head->SetupAttachment(GetMesh(), FName("head"));
	HitCollisionBoxes.Add(FName("head"), head);
	
	pelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("pelvis"));
	pelvis->SetupAttachment(GetMesh(), FName("pelvis "));
	HitCollisionBoxes.Add(FName("pelvis"), pelvis);

	spine_02 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_02"));
	spine_02->SetupAttachment(GetMesh(), FName("spine_02"));
	HitCollisionBoxes.Add(FName("spine_02"), spine_02);

	spine_03 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_03"));
	spine_03->SetupAttachment(GetMesh(), FName("spine_03"));
	HitCollisionBoxes.Add(FName("spine_03"), spine_03);

	upperarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_l"));
	upperarm_l->SetupAttachment(GetMesh(), FName("upperarm_l"));
	HitCollisionBoxes.Add(FName("upperarm_l"), upperarm_l);

	upperarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("upperarm_r"));
	upperarm_r->SetupAttachment(GetMesh(), FName("upperarm_r"));
	HitCollisionBoxes.Add(FName("upperarm_r"), upperarm_r);

	lowerarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_l"));
	lowerarm_l->SetupAttachment(GetMesh(), FName("lowerarm_l"));
	HitCollisionBoxes.Add(FName("lowerarm_l"), lowerarm_l);

	lowerarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_r"));
	lowerarm_r->SetupAttachment(GetMesh(), FName("lowerarm_r"));
	HitCollisionBoxes.Add(FName("lowerarm_r"), lowerarm_r);

	hand_l = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_l"));
	hand_l->SetupAttachment(GetMesh(), FName("hand_l"));
	HitCollisionBoxes.Add(FName("hand_l"), hand_l);

	hand_r = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_r"));
	hand_r->SetupAttachment(GetMesh(), FName("hand_r"));
	HitCollisionBoxes.Add(FName("hand_r"), hand_r);

	backpack = CreateDefaultSubobject<UBoxComponent>(TEXT("backpack"));
	backpack->SetupAttachment(GetMesh(), FName("backpack"));
	HitCollisionBoxes.Add(FName("backpack"), backpack);

	blanket = CreateDefaultSubobject<UBoxComponent>(TEXT("blanket"));
	blanket->SetupAttachment(GetMesh(), FName("backpack"));
	HitCollisionBoxes.Add(FName("blanket"), blanket);

	thigh_l = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_l"));
	thigh_l->SetupAttachment(GetMesh(), FName("thigh_l"));
	HitCollisionBoxes.Add(FName("thigh_l"), thigh_l);

	thigh_r = CreateDefaultSubobject<UBoxComponent>(TEXT("thigh_r"));
	thigh_r->SetupAttachment(GetMesh(), FName("thigh_r"));
	HitCollisionBoxes.Add(FName("thigh_r"), thigh_r);

	calf_l = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_l"));
	calf_l->SetupAttachment(GetMesh(), FName("calf_l"));
	HitCollisionBoxes.Add(FName("calf_l"), calf_l);

	calf_r = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_r"));
	calf_r->SetupAttachment(GetMesh(), FName("calf_r"));
	HitCollisionBoxes.Add(FName("calf_r"), calf_r);

	foot_l = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_l"));
	foot_l->SetupAttachment(GetMesh(), FName("foot_l"));
	HitCollisionBoxes.Add(FName("foot_l"), foot_l);

	foot_r = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_r"));
	foot_r->SetupAttachment(GetMesh(), FName("foot_r"));
	HitCollisionBoxes.Add(FName("foot_r"), foot_r);

	for (const auto& Box : HitCollisionBoxes)
	{
		if (Box.Value)
		{
			Box.Value->SetCollisionObjectType(ECC_HitBox);
			Box.Value->SetCollisionResponseToAllChannels(ECR_Ignore);
			Box.Value->SetCollisionResponseToChannel(ECC_HitBox, ECR_Block);
			Box.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	
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
			GetMesh(),
			FName(),
			GetActorLocation() + FVector(0.f, 0.f, 120.f),
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
	/*// test
	FFramePackage Package;
	if (LagCompensation)
	{
		LagCompensation->SaveFramePackage(Package);
		LagCompensation->ShowFramePackage(Package, FColor::Orange);
	}*/
	
	// tood 在哪可以set
	GetMovementComponent()->NavAgentProps.bCanCrouch = true;

	// init
	Health = MaxHealth;
	if (BlasterCombatComp == nullptr)
	{
		//Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
		DebugUtil::PrintMsg(this, FString::Printf(TEXT("In BlasterCharacter BeginPlay Combat is null")));
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

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
	if (BlasterCombatComp && BlasterCombatComp->bHoldingTheFlag)
	{
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
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
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// 1. 使用AimOffset  only for player controlling the character
	// 2. 使用SImProxyTurn   for simProxy or character not controlled on server 
	RotateInPlace(DeltaTime);
	HideCharacterIfCameraClose();
	
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
	DOREPLIFETIME(ABlasterCharacter, Shield);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (BlasterCombatComp)
	{
		BlasterCombatComp->Character = this;
	}
	if (Buff)
	{
		Buff->Character = this;
		Buff->SetInitialSpeed(GetCharacterMovement()->MaxWalkSpeed,
							GetCharacterMovement()->MaxWalkSpeedCrouched);
		Buff->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity);
	}
	if (LagCompensation)
	{
		LagCompensation->Character = this;
		if (Controller)
		{
			LagCompensation->Controller = Cast<ABlasterPlayerController>(Controller);
		}
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

void ABlasterCharacter::SetHoldingTheFlag(bool bHolding)
{
	if (BlasterCombatComp)
	{
		BlasterCombatComp->bHoldingTheFlag = bHolding;
	}
}

bool ABlasterCharacter::isWeaponEquipped() const
{
	return BlasterCombatComp && BlasterCombatComp->EquippedWeapon;
}

bool ABlasterCharacter::isAiming() const
{
	return BlasterCombatComp && BlasterCombatComp->bIsAiming;
}

AWeapon* ABlasterCharacter::GetEquippedWeapon() const
{
	if (BlasterCombatComp == nullptr)	return nullptr;
	return BlasterCombatComp->EquippedWeapon;
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (!BlasterCombatComp || !BlasterCombatComp->EquippedWeapon)	return;
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
	if (!BlasterCombatComp || !BlasterCombatComp->EquippedWeapon)	return;
	//PlayAnimMontage(FireWeaponMontage);
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;
		switch (BlasterCombatComp->EquippedWeapon->GetWeaponType())
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

void ABlasterCharacter::PlayHoverMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HoverMontage)
	{
		DebugUtil::PrintMsg(this, FString::Printf(TEXT("%s PlayHoverMontage"), *this->GetName()));
		AnimInstance->Montage_Play(HoverMontage);
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
	DebugUtil::PrintMsg(this, FString::Printf(TEXT("%s PlayThrowGrenadeMontage"), *this->GetName()));
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void ABlasterCharacter::PlaySwapMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && SwapMontage)
	{
		AnimInstance->Montage_Play(SwapMontage);
		DebugUtil::PrintMsg(this, FString::Printf(TEXT("%s Play Swap Montage"), *this->GetName()));
	}
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if (BlasterCombatComp == nullptr)	return FVector();
	return BlasterCombatComp->HitTarget;
}

bool ABlasterCharacter::IsHoldingTheFlag() const
{
	if (BlasterCombatComp == nullptr)	return false;
	return BlasterCombatComp->bHoldingTheFlag;
}

bool ABlasterCharacter::IsLocallyReloading()
{
	if (!BlasterCombatComp)	return false;
	return BlasterCombatComp->bLocallyReloading;
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	if (BlasterCombatComp == nullptr)	return ECombatState::ECS_UnOccupied;
	return BlasterCombatComp->CombatState;
}

ETeam ABlasterCharacter::GetTeam()
{
	BlasterPlayerState = GetBlasterPlayerState();
	if (BlasterPlayerState == nullptr)
	{
		return ETeam::ET_None;
	}
	return BlasterPlayerState->GetTeam();
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
	if (BlasterCombatComp)
	{
		BlasterCombatComp->DropOrDestroyWeapon(BlasterCombatComp->EquippedWeapon);
		BlasterCombatComp->DropOrDestroyWeapon(BlasterCombatComp->SecondaryWeapon);
		BlasterCombatComp->DropOrDestroyWeapon(BlasterCombatComp->TheFlag);
		BlasterCombatComp->EquippedWeapon = nullptr;
		BlasterCombatComp->SecondaryWeapon = nullptr;
		BlasterCombatComp->TheFlag = nullptr;
	}
}

void ABlasterCharacter::Elim(bool bPlayerLeftGame)
{
	DropOrDestroyWeapons();
	MulticastElim(bPlayerLeftGame);
}

// Set Team Player Start Point once PlayerState(Team) is initialized
void ABlasterCharacter::SetSpawnPoint()
{
	TArray<AActor*> AllPlayerStart;
	UGameplayStatics::GetAllActorsOfClass(this, ATeamPlayerStart::StaticClass(), AllPlayerStart);
	TArray<ATeamPlayerStart*> MyTeamPlayerStarts;
	for (auto Start : AllPlayerStart)
	{
		ATeamPlayerStart* TeamPlayerStart = Cast<ATeamPlayerStart>(Start);
		if (TeamPlayerStart && TeamPlayerStart->Team == GetTeam())
		{
			MyTeamPlayerStarts.Add(TeamPlayerStart);
		}
	}
	// pick on spot
	if (!MyTeamPlayerStarts.IsEmpty())
	{
		ATeamPlayerStart* ChosenPlayerStart = MyTeamPlayerStarts[FMath::RandRange(0, MyTeamPlayerStarts.Num()-1)];
		SetActorLocationAndRotation(ChosenPlayerStart->GetActorLocation(), ChosenPlayerStart->GetActorRotation());
	}
}

void ABlasterCharacter::OnPlayerStateInitialized()
{
	BlasterPlayerState->AddToScore(0.f);
	BlasterPlayerState->AddToDefeats(0);
	SetTeamColor(BlasterPlayerState->GetTeam());
}

// deal with initialization
void ABlasterCharacter::PollInit()
{
	if (BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState)
		{
			OnPlayerStateInitialized();
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
		BlasterCombatComp && BlasterCombatComp->bIsAiming && BlasterCombatComp->EquippedWeapon &&
		BlasterCombatComp->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle; 
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
	if (BlasterCombatComp && BlasterCombatComp->bHoldingTheFlag)
	{
		DebugUtil::PrintMsg(this, FString::Printf(TEXT("Hover Return")));
		return;
	}
	ServerHoverButtonPressed();
	if (!HasAuthority())
	{
		PlayHoverMontage();	
	}
}

void ABlasterCharacter::ServerHoverButtonPressed_Implementation()
{
	bHovering = !bHovering;
	PlayHoverMontage();
}

void ABlasterCharacter::OnRep_bHovering()
{
	if (bHovering)
	{
		
	}
}

void ABlasterCharacter::EquipButtonPressed()
{
	if (!BlasterCombatComp)					return;
	if (BlasterCombatComp->bHoldingTheFlag)	return;
	if (BlasterCombatComp->CombatState == ECombatState::ECS_UnOccupied)
	{
		ServerEquipButtonPressed();	
	}
	bool bSwap = BlasterCombatComp->CanSwapWeapon()
		&& !HasAuthority()
		&& BlasterCombatComp->CombatState == ECombatState::ECS_UnOccupied
		&& OverlappingWeapon == nullptr;
	if (bSwap)
	{
		PlaySwapMontage();
		BlasterCombatComp->CombatState = ECombatState::ECS_SwapWeapons;
		bFinishSwapping = false;
	} else
	{
		/*DebugUtil::PrintMsg(TEXT("bSwap is false"));
		if (!Combat->CanSwapWeapon())
			DebugUtil::PrintMsg(TEXT("bSwap is false"));
		if (!HasAuthority())
			DebugUtil::PrintMsg(TEXT("!HasAuthority()"));
		if (Combat->CombatState != ECombatState::ECS_UnOccupied)
			DebugUtil::PrintMsg(TEXT("Combat->CombatState != ECombatState::ECS_UnOccupied"));
		if (OverlappingWeapon != nullptr)
			DebugUtil::PrintMsg(TEXT("OverlappingWeapon != nullptr"));*/
	}
}

void ABlasterCharacter::DropButtonPressed()
{
	if (BlasterCombatComp && BlasterCombatComp->bHoldingTheFlag)	return;
	ServerDropButtonPressed();
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (BlasterCombatComp && BlasterCombatComp->bHoldingTheFlag)	return;
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
	if (!BlasterCombatComp)					return;
	if (BlasterCombatComp->bHoldingTheFlag)	return;
	BlasterCombatComp->SetAiming(true);
}

void ABlasterCharacter::AimButtonReleased()
{
	if (!BlasterCombatComp)	return;
	if (BlasterCombatComp->bHoldingTheFlag)	return;
	BlasterCombatComp->SetAiming(false);
}

void ABlasterCharacter::ReloadButtonPressed()
{
	if (!BlasterCombatComp)					return;
	if (BlasterCombatComp->bHoldingTheFlag)	return;
	BlasterCombatComp->Reload();
}

void ABlasterCharacter::GrenadeButtonPressed()
{
	if (!BlasterCombatComp)					return;
	if (BlasterCombatComp->bHoldingTheFlag)	return;
	DebugUtil::PrintMsg(this, FString::Printf(TEXT("GrenadeButtonPressed")));
	BlasterCombatComp->ThrowGrenade();
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
	if (BlasterCombatComp && BlasterCombatComp->EquippedWeapon == nullptr) return;
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
	if (BlasterCombatComp == nullptr || BlasterCombatComp->EquippedWeapon == nullptr)	return;
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
	if (BlasterCombatComp && BlasterCombatComp->bHoldingTheFlag)	return;
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
	if (!BlasterCombatComp)					return;
	if (BlasterCombatComp->bHoldingTheFlag)	return;
	BlasterCombatComp->FireButtonPressed(true);
}

void ABlasterCharacter::FireButtonReleased()
{
	if (!BlasterCombatComp)					return;
	if (BlasterCombatComp->bHoldingTheFlag)	return;
	BlasterCombatComp->FireButtonPressed(false);
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (!BlasterCombatComp || !BlasterCombatComp->EquippedWeapon)	return;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName = FName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::SetTeamColor(ETeam TeamToSet)
{
	DebugUtil::PrintMsg(FString::Printf(TEXT("SetTeamColor")), FColor::Green);
	switch (TeamToSet)
		{
		case ETeam::ET_None:
			GetMesh()->SetMaterial(0, OriginalMaterial);
			DissolveMaterialInstance = BlueDissolveMatInst;
			break;
		case ETeam::ET_TeamBlue:
			GetMesh()->SetMaterial(0, BlueMaterial);
			DissolveMaterialInstance = BlueDissolveMatInst;
			break;
		case ETeam::ET_TeamRed:
			GetMesh()->SetMaterial(0, RedMaterial);
			DissolveMaterialInstance = RedDissolveMatInst;
			break;
		default:
			GetMesh()->SetMaterial(0, OriginalMaterial);
			DissolveMaterialInstance = BlueDissolveMatInst;
			break;		
	}
}

// bind this to delegate onTakenAnyDamage, and only on the server
void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatorController, AActor* DamageCauser)
{
	if (bElimmed || !GetBlasterGameMode())	return;
	
	float DamageToApply = GetBlasterGameMode()->CalculateDamage(InstigatorController, GetController(), Damage);
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

// todo 这里需要改。有关client swap weapon的bug
/***
 * Equip Weapon
 * 1. Overlap == nullptr
 *		1) canSwap  -> Swap
 *		2) can't swap -> return
 * 2. Overlap is not null
 *		1) no weapon -> equip
 *		2) one weapon -> equip overlapWeapon and swap current weapon to secondary weapon
 *		3) two weapon -> drop secondary weapon and equip overlap to secondary
 */
void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (!BlasterCombatComp)	return;
	if (OverlappingWeapon)
	{
		BlasterCombatComp->EquipWeapon(OverlappingWeapon);
	} else	// 无 Overlap Weapon且 可以切换武器
	{
		if (BlasterCombatComp->CanSwapWeapon())
		{
			BlasterCombatComp->SwapWeapons();
		} else if (BlasterCombatComp->EquippedWeapon == nullptr && BlasterCombatComp->SecondaryWeapon != nullptr)
		{
			BlasterCombatComp->TakeSecondaryWeapon();
		} else if (BlasterCombatComp->EquippedWeapon != nullptr && BlasterCombatComp->SecondaryWeapon == nullptr)
		{
			BlasterCombatComp->PutSecondaryWeapon();
		}
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

void ABlasterCharacter::ServerDropButtonPressed_Implementation()
{
	if (BlasterCombatComp && BlasterCombatComp->EquippedWeapon)
	{
		BlasterCombatComp->DropWeapon();
	}
}

void ABlasterCharacter::SetCharacterVisibility(bool bVisiable)
{
	if (GetMesh() == nullptr)	return;
	GetMesh()->SetVisibility(bVisiable);
	if (BlasterCombatComp && BlasterCombatComp->EquippedWeapon && BlasterCombatComp->EquippedWeapon->GetWeaponMesh())
	{
		BlasterCombatComp->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = !bVisiable;
	}
}

void ABlasterCharacter::HideCharacterIfCameraClose()
{
	if (!IsLocallyControlled() || FollowCamera == nullptr)		return;
	float distance = (FollowCamera->GetComponentLocation() - GetActorLocation()).Size();
	if (distance < CameraThreshold)
	{
		SetCharacterVisibility(false);
	} else
	{
		SetCharacterVisibility(true);
	}
}


  