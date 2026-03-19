// Fill out your copyright notice in the Description page of Project Settings.


#include "MoverPawn.h"
#include "GrapplerComponent.h"
#include "Components/CapsuleComponent.h"

// Sets default values
AMoverPawn::AMoverPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Construct Capsule Component Reference for safer getting outside this script
	//This needs to be root for Pawn Movement and collision to work properly
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	SetRootComponent(CapsuleComponent);

	//Construct Skeletal Mesh Component Reference for safer getting outside this script
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	if (IsValid(SkeletalMeshComponent))
	{
		SkeletalMeshComponent->SetupAttachment(CapsuleComponent);
	}

	//Construct Grappler Component Reference for safer getting outside this script, and for safe keeping
	GrapplerComponent = CreateDefaultSubobject<UGrapplerComponent>(TEXT("GrapplerComponent"));
	AddOwnedComponent(GrapplerComponent);	
}

// Called when the game starts or when spawned
void AMoverPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMoverPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMoverPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

