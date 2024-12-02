// Fill out your copyright notice in the Description page of Project Settings.


#include "Flag.h"

#include "WeaponTypes.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Utils/DebugUtil.h"
#include "Components/WidgetComponent.h"

AFlag::AFlag()
{
	bReplicates = true;
	SetReplicates(true);
	
	FlagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlagMesh"));
	FlagMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FlagMesh->SetRelativeScale3D(FVector(0.15, 0.15, 0.15));
	SetRootComponent(FlagMesh);
	if (GetAreaSphere())	GetAreaSphere()->SetupAttachment(FlagMesh);
	if (GetPickupWidget())	GetPickupWidget()->SetupAttachment(FlagMesh);
}

void AFlag::Drop()
{
	DebugUtil::PrintMsg(TEXT("Drop Flag"));
	SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	FlagMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);
	BlasterOwner = nullptr;
	BlasterController = nullptr;
}

void AFlag::SetFlagPhysicsAndCollision(bool bEnable)
{
	FlagMesh->SetSimulatePhysics(bEnable);
	FlagMesh->SetEnableGravity(bEnable);
	FlagMesh->SetCollisionEnabled(bEnable ? ECollisionEnabled::QueryAndPhysics :ECollisionEnabled::NoCollision);
}

void AFlag::BeginPlay()
{
	Super::BeginPlay();
	InitialTrans = GetActorTransform();
	SetReplicateMovement(true);
}

void AFlag::OnEquip()
{
	DebugUtil::PrintMsg(this, TEXT("OnEquip Flag"));
	ShowPickupWidget(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetFlagPhysicsAndCollision(false);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	FlagMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	EnableCustomDepth(false);
}

void AFlag::OnDrop()
{
	DebugUtil::PrintMsg(TEXT("OnDrop Flag"));
	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);	
	}
	SetFlagPhysicsAndCollision(true);
	FlagMesh->SetCollisionResponseToAllChannels(ECR_Block);
	FlagMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	FlagMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	
	FlagMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	FlagMesh->MarkRenderStateDirty();	// force a refresh
	EnableCustomDepth(true);
}

void AFlag::ResetFlag()
{
	ABlasterCharacter* FlagOwner = Cast<ABlasterCharacter>(GetOwner()); 
	if (FlagOwner)
	{
		FlagOwner->SetHoldingTheFlag(false);
		FlagOwner->SetOverlappingWeapon(nullptr);
		FlagOwner->UnCrouch();	//
	}
	if (!HasAuthority())	return;
	SetActorTransform(InitialTrans);
	// detach
	SetWeaponState(EWeaponState::EWS_Initial);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	FlagMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);
	BlasterOwner = nullptr;
	BlasterController = nullptr;
	GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetAreaSphere()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// 试下要不要加
	//SetFlagPhysicsAndCollision(true);
	DebugUtil::PrintMsg(this, FString::Printf(TEXT("ResetFlag")));
}