// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrapplerComponent.generated.h"

class AMoverPawn;
class AGrappleSocket;
class UCableComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GAS_API UGrapplerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGrapplerComponent(const FObjectInitializer& ObjectInitializer);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	//Function for getting necessary references
	void Init();

public:	

	//Bool check to see if Pawn is eligible to grapple onto the closest Grapple Socket
	UFUNCTION(BlueprintCallable) bool TryToAttachToGrappleSocket();

	//Deattaches from the current attached Grapple Socket
	UFUNCTION(BlueprintCallable) void DeattachFromGrappleSocket();

	//Tries to get the closest Grapple Socket in range
	UFUNCTION(BlueprintCallable) AGrappleSocket* FindClosestGrappleSocket() const;

	//Gets CurrentGrappleSocket reference
	UFUNCTION(BlueprintCallable) AGrappleSocket* GetCurrentGrappleSocket() const { return CurrentGrappleSocket; }

	//Gets Grapple Rope reference
	UFUNCTION(BlueprintCallable) UCableComponent* GetGrappleRope() const { return GrappleRope; }

	//Checks if Pawn is currently doing Grappling Action
	UFUNCTION(BlueprintCallable) bool GetDoingGrapplingAction() const { return bDoingGrapplingAction; }

	//Constructs necessary Grapple Rope variables for current Grapple Socket
	UFUNCTION(BlueprintCallable) void ConstructGrappleRope() const;

private:

	//Grapple Rope Reference
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true")) TObjectPtr<UCableComponent> GrappleRope = nullptr;

	//Current Grapple Socket Reference
	UPROPERTY() AGrappleSocket* CurrentGrappleSocket = nullptr;

	//Owner Pawn Reference
	UPROPERTY() AMoverPawn* OwnerPawn = nullptr;

	//Bool for checking if Pawn doing Grappling Action
	UPROPERTY() bool bDoingGrapplingAction = false;
};
