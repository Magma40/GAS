// Fill out your copyright notice in the Description page of Project Settings.
#include "MoverPawn.h"
#include "GrapplerComponent.h"
#include "Components/CapsuleComponent.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"

// Sets default values
AMoverPawn::AMoverPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Destroy Components for reconstruction
	//Destroy Capsule Component
	UCapsuleComponent* CapsuleComponentFound = GetComponentByClass<UCapsuleComponent>();
	if (IsValid(CapsuleComponentFound))
	{
		CapsuleComponentFound->DestroyComponent();	
	}

	//Destroy Skeletal Mesh Component
	USkeletalMeshComponent* SkeletalMeshComponentFound = GetComponentByClass<USkeletalMeshComponent>();
	if (IsValid(SkeletalMeshComponentFound))
	{
		SkeletalMeshComponentFound->DestroyComponent();	
	}

	//Destroy GrapplerComponent
	UGrapplerComponent* GrapplerComponentFound = GetComponentByClass<UGrapplerComponent>();
	if (IsValid(GrapplerComponentFound))
	{
		GrapplerComponentFound->DestroyComponent();	
	}

	//Destroy Character Mover Component
	UCharacterMoverComponent* CharacterMoverComponentFound = GetComponentByClass<UCharacterMoverComponent>();
	if (IsValid(CharacterMoverComponentFound))
	{
		CharacterMoverComponentFound->DestroyComponent();	
	}
	
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

	//Construct Character Mover Component Reference for safer getting outside this script, and for safe keeping
	CharacterMoverComponent = CreateDefaultSubobject<UCharacterMoverComponent>(TEXT("CharacterMover"));
	AddOwnedComponent(CharacterMoverComponent);
	
	//Construct Grappler Component Reference for safer getting outside this script, and for safe keeping
	GrapplerComponent = CreateDefaultSubobject<UGrapplerComponent>(TEXT("GrapplerComponent"));
	AddOwnedComponent(GrapplerComponent);

	//Debug log that the object has been constructed
	UE_LOG(LogTemp, Warning, TEXT("%s:AMoverPawn - %s Constructed:"), *StaticClass()->GetName(), *this->GetName());
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

