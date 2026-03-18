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
	void Init();

public:	

	UFUNCTION(BlueprintCallable) bool TryToAttachToGrappleSocket();

	UFUNCTION(BlueprintCallable) void DeattachFromGrappleSocket();

	UFUNCTION(BlueprintCallable) AGrappleSocket* FindClosestGrappleSocket() const;

	UFUNCTION(BlueprintCallable) AGrappleSocket* GetCurrentGrappleSocket() const { return CurrentGrappleSocket; }

	UFUNCTION(BlueprintCallable) UCableComponent* GetGrappleRope() const { return GrappleRope; }

	UFUNCTION(BlueprintCallable) bool GetDoingGrapplingAction() const { return bDoingGrapplingAction; }

	UFUNCTION(BlueprintCallable) void ConstructGrappleRope() const;

private:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true")) TObjectPtr<UCableComponent> GrappleRope = nullptr;
	
	UPROPERTY() AGrappleSocket* CurrentGrappleSocket = nullptr;
	
	UPROPERTY() AMoverPawn* OwnerPawn = nullptr;

	UPROPERTY() bool bDoingGrapplingAction = false;
};
