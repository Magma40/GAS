// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MoverPawn.generated.h"

class UGrapplerComponent;
class USkeletalMeshComponent;
class UInputAction;
class UCharacterMoverComponent;

UCLASS(Blueprintable, BlueprintType)
class GAS_API AMoverPawn : public APawn
{
	GENERATED_BODY()

protected:
	AMoverPawn();

public:	
	//Grappler Component Reference
	UPROPERTY(BlueprintReadWrite, Category = "Mover Pawn")
	TObjectPtr<UGrapplerComponent> GrapplerComponent = nullptr;

	//Skeletal Mesh Component Reference
	UPROPERTY(BlueprintReadWrite, Category = "Mover Pawn")
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent = nullptr;

	//Capsule Component Reference
	UPROPERTY(BlueprintReadWrite, Category = "Mover Pawn")
	TObjectPtr<UCapsuleComponent> CapsuleComponent = nullptr;

	//Move Action Input Reference
	UPROPERTY(BlueprintReadWrite, Category = "Mover Pawn")
	TObjectPtr<UInputAction> MoveAction = nullptr;

	//Character Mover Component Reference
	UPROPERTY(BlueprintReadWrite, Category = "Mover Pawn")
	TObjectPtr<UCharacterMoverComponent> CharacterMoverComponent = nullptr;
};
