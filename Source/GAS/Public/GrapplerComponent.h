// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "Components/ActorComponent.h"
#include "GrapplerComponent.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(Mover_IsGrappling);

class AMoverPawn;
class AGrappleSocket;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GAS_API UGrapplerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGrapplerComponent();

public:	
	//Bool check to see if Pawn is eligible to grapple onto the closest Grapple Socket
	UFUNCTION(BlueprintCallable) bool TryToAttachToGrappleSocket(AMoverPawn* InPawn);

	//Deattaches from the current attached Grapple Socket
	UFUNCTION(BlueprintCallable) void DetachFromGrappleSocket(AMoverPawn* InPawn);
	
	//Sets the doing Grappling Action
	UFUNCTION(BlueprintCallable) void SetDoingGrapplingAction(const bool bNewValue) { bDoingGrapplingAction = bNewValue; }
	
	//Checks if Pawn is currently doing Grappling Action
	UFUNCTION(BlueprintCallable) bool GetDoingGrapplingAction() const { return bDoingGrapplingAction; }

	//Sets a new Grapple Socket
	UFUNCTION(BlueprintCallable) void  SetCurrentGrappleSocket(AGrappleSocket* NewGrappleSocket) { CurrentGrappleSocket = NewGrappleSocket; }


private:
	//Tries to get the closest Grapple Socket in range
	AGrappleSocket* FindClosestGrappleSocket(const AMoverPawn* InPawn) const;
	
	//Bool for checking if Pawn doing Grappling Action
	UPROPERTY() bool bDoingGrapplingAction = false;

	//Current Grapple socket reference
	UPROPERTY() TObjectPtr<AGrappleSocket> CurrentGrappleSocket = nullptr;
};
