// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DefaultMovementSet/LayeredMoves/LaunchMove.h"
#include "GameFramework/Actor.h"
#include "GrappleSocket.generated.h"

class UCameraComponent;
class UGrapplingSocketWidgetComponent;
class AMoverPawn;
class UCableComponent;
class UCapsuleComponent;

UCLASS()
class GAS_API AGrappleSocket : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGrappleSocket();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual void Tick(float DeltaTime) override;

public:	

	//Updates the widget position and rotation
 	void UpdateWidget();

	//Updates the variables on the Grapple Rope
	void UpdateGrappleRope();

	//Checks if a pawn is close enough to be able to grapple onto this socket
	bool IsInRangeToGrapple(const AMoverPawn* InPawn) const;

	//Gets the distance from a pawn to this socket
	float GetDistanceFromAPawn(const AMoverPawn* InPawn) const;

	//Attaches this socket's Grappler Rope onto the Pawns hand
	void AttachToGrappleSocket(AMoverPawn* InPawn);

	//Attaches a pawn and its grappler onto this socket
	void AttachPawnToGrappleSocket(AMoverPawn* InPawn) const;

	//Deattaches a pawn and its grappler from this socket
	void DetachFromGrappleSocket(AMoverPawn* InPawn, bool bApplyForce);

	//Constructs necessary Grapple Rope variables for current Grapple Socket
	void ConstructGrappleRope(AMoverPawn* InPawn) const;

	//Constructs the FLayeredMove for when pawn needs to push off from the Grapple Socket
	static TSharedPtr<FLayeredMove_Launch> ConstructGrappleMove(const AMoverPawn* InPawn, const FVector& LaunchVelocity);

	//Function to enable physics for grappling swinging motion
	void EnableGrappling() const;
	
private:
	//Player pawn Reference
	UPROPERTY() TObjectPtr<AMoverPawn> Cached_PlayerPawn = nullptr;

	//Camera Actor Reference
	UPROPERTY() TObjectPtr<UCameraComponent> Cached_Camera = nullptr;

	UPROPERTY() TObjectPtr<UStaticMeshComponent> StaticMeshComponent = nullptr;
	
	//Grappling Socket Widget Component Reference
	UPROPERTY()  TObjectPtr<UGrapplingSocketWidgetComponent> GrapplingSocketWidgetComponent = nullptr;

	//Cached Default Rope Length
	UPROPERTY() float CachedDefaultRopeLength = 0.0f;

	//Cached Pawn Location Reference
	UPROPERTY() FVector Cached_PawnLocation = FVector::ZeroVector;

	//Cached Pawn Location Reference
	UPROPERTY() FTimerHandle EnableGrapplingTimerHandle; 

public:
	//Able to change Min Distance To Grapple in the editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Socket") float MinDistanceToGrapple = 0.0f;

	//Able to enable Image in the editor 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Socket") bool bEnableImage = true;

	//Able to enable Text in the editor 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Socket") bool bEnableText = true;

	//A maximum value which the Grappler Rope can get
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true")) float MaxGrapplerRopeLength  = 0.0f;

	//An extra value to add or remove length to get a nice proper length on the Grapple Rope
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true")) float GrapplerRopeExtraShaveOffFromOrAddToLength  = 0.0f;
	
	//Interval for updating Pawn's location and making sure its properly set up
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true")) float IntervalBetweenUpdatingPlayerLocations  = 0.0f;

	//Root Component Reference
	UPROPERTY() TObjectPtr<USceneComponent> Root = nullptr;

	//Grapple Rope Reference
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true")) TObjectPtr<UCableComponent> GrappleRope = nullptr;

	//Grapple Edge Component Reference, this is for the Pawn to grapple onto
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true")) TObjectPtr<UCapsuleComponent>  GrappleEdgeComponent= nullptr;
};
