// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Casing.h"
#include "WeaponTypes.h"
#include "Blaster/BlasterComponent/CombatComponent.h"
#include "Blaster/Utils/DebugUtil.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	WeaponName = FName("Weapon");

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);
	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();	// force a refresh
	EnableCustomDepth(true);
	
	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	// disable here, enable in beginplay on server
	AreaSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickUpWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickUpWidget"));
	PickUpWidget->SetupAttachment(RootComponent);
	WeaponState = EWeaponState::EWS_Initial;
}

void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority())		return;
	Ammo = ServerAmmo;
	--Sequence;
	Ammo -= Sequence;
	SetHUDAmmo();
}

void AWeapon::ClientAddAmmo_Implementation(int32 AmmoToAdd)
{
	if (HasAuthority())		return;
	
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MaxCapacity);
	BlasterOwner = BlasterOwner == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwner;
	if (BlasterOwner && BlasterOwner->GetCombat() && IsFull())
	{
		BlasterOwner->GetCombat()->JumpToShotgunEnd();	// reload finish, jump to end section
	}
	SetHUDAmmo();
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	if (PickUpWidget)
	{
		PickUpWidget->SetVisibility(false);
	}
	// 客户端本地即可设置碰撞体(pickup widget),但是装备武器必须放在server执行
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
}

const FName& AWeapon::GetWeaponName() const
{
	return WeaponName;
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); 
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME_CONDITION(AWeapon, bUseServerSideRewind, COND_OwnerOnly);
}

void AWeapon::Fire(const FVector& HitTarget)
{
	DebugUtil::PrintMsg(FString::Printf(TEXT("Weapon %s Fired"), *GetWeaponName().ToString()), FColor::Purple);
	if (bUseServerSideRewind)
	{
		DebugUtil::PrintMsg(FString::Printf(TEXT("Using Server Side Rewind")), FColor::Purple);
	} else
	{
		DebugUtil::PrintMsg(FString::Printf(TEXT("Not Using Server Side Rewind")), FColor::Purple);
	}
	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}
	if (CasingClass)
	{
		const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject"));
		if (AmmoEjectSocket)
		{
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);
			UWorld* World = GetWorld();
			if (World)
			{
				World->SpawnActor<ACasing>(
					CasingClass,
					SocketTransform.GetLocation(),
					SocketTransform.GetRotation().Rotator());
			}
		}
	}
	SpendRound();
}

// set PickupWidget visibility to false
void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickUpWidget)
	{
		PickUpWidget->SetVisibility(bShowWidget);
		PickUpWidget->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	}
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherIndex, bool mFromSweep, const FHitResult& SweepHitResult)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		if (WeaponType == EWeaponType::EWT_Flag && BlasterCharacter->GetTeam() != Team)	return;
		if (BlasterCharacter->IsHoldingTheFlag())		return;
		BlasterCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherIndex)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		if (WeaponType == EWeaponType::EWT_Flag && BlasterCharacter->GetTeam() != Team)	return;
		if (BlasterCharacter->IsHoldingTheFlag())		return;
		BlasterCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::Drop()
{
	UE_LOG(LogTemp, Error, TEXT("Drop Weapon"));
	SetWeaponState(EWeaponState::EWS_Dropped);
	// Detach from Component
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);
	BlasterOwner = nullptr;
	BlasterController = nullptr;
}

void AWeapon::SetWeaponPhysicsAndCollision(bool bEnable)
{
	WeaponMesh->SetSimulatePhysics(bEnable);
	WeaponMesh->SetEnableGravity(bEnable);
	WeaponMesh->SetCollisionEnabled(bEnable ? ECollisionEnabled::QueryAndPhysics :ECollisionEnabled::NoCollision);
}

void AWeapon::OnEquip()
{
	ShowPickupWidget(false);
	// Disable Physics and Collision
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetWeaponPhysicsAndCollision(false);
	EnableCustomDepth(false);
	BindOrRemoveHighPingDelegate(true);
}

void AWeapon::OnDrop()
{
	SetWeaponPhysicsAndCollision(true);
	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);	
	}
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();	// force a refresh
	EnableCustomDepth(true);
	BindOrRemoveHighPingDelegate(false);
}

void AWeapon::OnEquipSecondary()
{
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetWeaponPhysicsAndCollision(false);
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
	WeaponMesh->MarkRenderStateDirty();	// force a refresh
	//EnableCustomDepth(true);
	BindOrRemoveHighPingDelegate(false);
}

void AWeapon::OnPingTooHigh(bool bPingTooHigh)
{
	bUseServerSideRewind = !bPingTooHigh;
}

void AWeapon::OnWeaponStateChange()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		OnEquip();
		break;
	case EWeaponState::EWS_Dropped:
		OnDrop();
		break;
	case EWeaponState::EWS_EquippedSecondary:
		OnEquipSecondary();
		break;
	default:
		break;		
	}
}

void AWeapon::SetWeaponState(EWeaponState state)
{
	WeaponState = state;
	OnWeaponStateChange();
}

/**
 * on client, WeapnState and AttachActor(Equip Weapon) is Replicated.
 * but we need to make sure WeaponState update first, then attachActor
 * cause SetWeaponState deal with Physics, and you can't AttachActor when Physics is enabled
 */
void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		//AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetWeaponPhysicsAndCollision(false);
		EnableCustomDepth(false);
		break;
	case EWeaponState::EWS_Dropped:
		SetWeaponPhysicsAndCollision(true);
		WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
		WeaponMesh->MarkRenderStateDirty();	// force a refresh
		EnableCustomDepth(true);
		break;
	case EWeaponState::EWS_EquippedSecondary:
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetWeaponPhysicsAndCollision(false);
		WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
		WeaponMesh->MarkRenderStateDirty();	// force a refresh
		EnableCustomDepth(true);
		break;
	default:
		break;
	}
}

/*
void AWeapon::OnRep_Ammo()
{
	BlasterOwner = BlasterOwner == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwner;
	if (BlasterOwner && BlasterOwner->GetCombat())
	{
		BlasterOwner->GetCombat()->JumpToShotgunEnd();
	}
	SetHUDAmmo();
}
*/

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	if (Owner == nullptr)
	{
		BlasterOwner = nullptr;
		BlasterController = nullptr;
	} else
	{
		// now the owner is set, we can get the controller
		SetHUDAmmo();	
	}
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MaxCapacity);
	if (HasAuthority())
	{
		ClientUpdateAmmo(Ammo);
	} else
	{
		++Sequence;
	}
	SetHUDAmmo();
}

void AWeapon::SetHUDAmmo()
{
	BlasterOwner = BlasterOwner == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwner;
	if (BlasterOwner && BlasterOwner->GetCombat())
	{
		BlasterController = BlasterController == nullptr ? Cast<ABlasterPlayerController>(BlasterOwner->Controller) : BlasterController;
		if (BlasterController)
		{
			BlasterController->SetHUDAmmo(Ammo, BlasterOwner->GetCombat()->GetCarriedAmmo());
		}
	}
}

void AWeapon::AddAmmo(int AmmoAmount)
{
	Ammo = FMath::Clamp(Ammo + AmmoAmount, 0, MaxCapacity);
	ClientAddAmmo(AmmoAmount);
	SetHUDAmmo();
}

void AWeapon::EnableCustomDepth(bool bEnable)
{
	if (WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bEnable);
	}
}

FVector AWeapon::TraceEndWithScatter(const FVector& TraceTarget)
{
	// 获取枪口位置":
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (!MuzzleFlashSocket)		return FVector();
	FVector TraceStart = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh()).GetLocation();
	
	FVector ToTargetNormalized = (TraceTarget - TraceStart).GetSafeNormal();
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	FVector RandomVec = UKismetMathLibrary::RandomUnitVector() * FMath::RandRange(0.f, SphereRadius);
	FVector RandomEnd = SphereCenter + RandomVec;
	FVector ToTraceEnd = RandomEnd - TraceStart;
	return FVector(TraceStart + TRACE_LENGTH * ToTraceEnd / ToTraceEnd.Size());
}

void AWeapon::BindOrRemoveHighPingDelegate(bool bBind)
{
	BlasterOwner = BlasterOwner == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwner;
	if (!BlasterOwner)	return;
	BlasterController = BlasterController == nullptr ? Cast<ABlasterPlayerController>(BlasterOwner->Controller) : BlasterController;
	if (!BlasterController || !HasAuthority())	return;
	if (bBind && !BlasterController->HighPingDelegate.IsBound())
	{
		BlasterController->HighPingDelegate.AddDynamic(this, &AWeapon::AWeapon::OnPingTooHigh);
	} else if (!bBind && BlasterController->HighPingDelegate.IsBound())
	{
		BlasterController->HighPingDelegate.RemoveDynamic(this, &AWeapon::AWeapon::OnPingTooHigh);
	}
}
