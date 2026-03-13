// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrapplerComponent.generated.h"

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

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable) bool TryToAttachToGrappleSocket();

	UFUNCTION(BlueprintCallable) void DeattachFromGrappleSocket();

	UFUNCTION(BlueprintCallable) AGrappleSocket* FindClosestGrappleSocket() const;

	UFUNCTION(BlueprintCallable) AGrappleSocket* GetCurrentGrappleSocket() const { return CurrentGrappleSocket; }

	UFUNCTION(BlueprintCallable) UCableComponent* GetGrappleRope() const { return GrappleRope; }

	UFUNCTION(BlueprintCallable) bool GetDoingGrapplingAction() const { return bDoingGrapplingAction; }

	UFUNCTION(BlueprintCallable) void ConstructGrappleRope() const;

private:

	UPROPERTY() UCableComponent* GrappleRope = nullptr;
	
	UPROPERTY() AGrappleSocket* CurrentGrappleSocket = nullptr;
	
	UPROPERTY() APawn* OwnerPawn = nullptr;

	UPROPERTY() bool bDoingGrapplingAction = false;
};
