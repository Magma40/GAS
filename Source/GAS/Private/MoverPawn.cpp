// Fill out your copyright notice in the Description page of Project Settings.
#include "MoverPawn.h"
#include "GrapplerComponent.h"
#include "Components/CapsuleComponent.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"

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
	SkeletalMeshComponent->SetupAttachment(CapsuleComponent);

	//Construct Character Mover Component Reference for safer getting outside this script, and for safe keeping
	CharacterMoverComponent = CreateDefaultSubobject<UCharacterMoverComponent>(TEXT("CharacterMover"));
	AddOwnedComponent(CharacterMoverComponent);
	
	//Construct Grappler Component Reference for safer getting outside this script, and for safe keeping
	GrapplerComponent = CreateDefaultSubobject<UGrapplerComponent>(TEXT("GrapplerComponent"));
	AddOwnedComponent(GrapplerComponent);
}

