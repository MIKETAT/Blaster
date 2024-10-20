// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Casing.h"
#include "WeaponTypes.h"
#include "Blaster/BlasterComponent/CombatComponent.h"
#include "Engine/SkeletalMeshSocket.h"

// Sets default values
AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

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

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	if (PickUpWidget)
	{
		PickUpWidget->SetVisibility(false);
	}
	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); 
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME(AWeapon, Ammo);
}

void AWeapon::Fire(const FVector& HitTarget)
{
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

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherIndex, bool mFromSweep, const FHitResult& SweepHitResult)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		BlasterCharacter->SetOverlappingWeapon(this);
	}
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

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherIndex)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
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
}

void AWeapon::OnEquipSecondary()
{
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetWeaponPhysicsAndCollision(false);
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
	WeaponMesh->MarkRenderStateDirty();	// force a refresh
	EnableCustomDepth(true);
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


void AWeapon::OnRep_Ammo()
{
	BlasterOwner = BlasterOwner == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwner;
	if (BlasterOwner && BlasterOwner->GetCombat())
	{
		BlasterOwner->GetCombat()->JumpToShotgunEnd();
	}
	SetHUDAmmo();
}

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
	Ammo = FMath::Clamp(Ammo-1, 0, MaxCapacity);
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
	SetHUDAmmo();
}

void AWeapon::EnableCustomDepth(bool bEnable)
{
	if (WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bEnable);
	}
}
