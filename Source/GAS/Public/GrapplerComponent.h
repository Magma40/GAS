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
	UGrapplerComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//Function for getting necessary references
	void Init();

public:	

	//Bool check to see if Pawn is eligible to grapple onto the closest Grapple Socket
	UFUNCTION(BlueprintCallable) bool TryToAttachToGrappleSocket();

	//Deattaches from the current attached Grapple Socket
	UFUNCTION(BlueprintCallable) void DetachFromGrappleSocket();

	//Tries to get the closest Grapple Socket in range
	UFUNCTION(BlueprintCallable) AGrappleSocket* FindClosestGrappleSocket() const;

	//Attaches the Pawn to this Grapple Socket
	UFUNCTION(BlueprintCallable) void AttachPawnToGrappleSocket() const;

	//Dettaches the Pawn to this Grapple Socket
	UFUNCTION(BlueprintCallable) void DetachPawnFromGrappleSocket() const;

	//Gets CurrentGrappleSocket reference
	UFUNCTION(BlueprintCallable) AGrappleSocket* GetCurrentGrappleSocket() const { return CurrentGrappleSocket; }

	//Gets Grapple Rope reference
	UFUNCTION(BlueprintCallable) UCableComponent* GetGrappleRope() const { return GrappleRope; }

	//Checks if Pawn is currently doing Grappling Action
	UFUNCTION(BlueprintCallable) bool GetDoingGrapplingAction() const { return bDoingGrapplingAction; }

	//Constructs necessary Grapple Rope variables for current Grapple Socket
	UFUNCTION(BlueprintCallable) void ConstructGrappleRope() const;

	void EnableGrappling() const;

private:

	//Grapple Rope Reference
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true")) TObjectPtr<UCableComponent> GrappleRope = nullptr;

	//A maximum value which the Grappler Rope can get
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true")) float MaxGrapplerRopeLength  = 0.0f;

	//An extra value to add or remove length to get a nice proper length on the Grapple Rope
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true")) float GrapplerRopeExtraShaveOffFromOrAddToLength  = 0.0f;;

	//Cached Default Rope Length
	UPROPERTY() float CachedDefaultRopeLength = 0.0f;
	
	//Current Grapple Socket Reference
	UPROPERTY() TObjectPtr<AGrappleSocket> CurrentGrappleSocket = nullptr;

	//Owner Pawn Reference
	UPROPERTY() TObjectPtr<AMoverPawn> OwnerPawn = nullptr;

	//Bool for checking if Pawn doing Grappling Action
	UPROPERTY() bool bDoingGrapplingAction = false;
};
