// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DefaultMovementSet/LayeredMoves/LaunchMove.h"
#include "GameFramework/Actor.h"
#include "GrappleSocket.generated.h"

struct FInputActionValue;
class USphereComponent;
class UCameraComponent;
class UGrapplingSocketWidgetComponent;
class AMoverPawn;
class UCableComponent;
class UCapsuleComponent;

UCLASS()
class GAS_API AGrappleSocket : public AActor
{
	GENERATED_BODY()
	
protected:
	AGrappleSocket();

	virtual void BeginPlay() override;
	
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	UFUNCTION()
	void OverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:	

	//Updates the widget position and rotation
 	void UpdateWidget() const;

	//Checks if a pawn is close enough to be able to grapple onto this socket
	bool IsInRangeToGrapple(const AMoverPawn* InPawn) const;

	//Gets the distance from a pawn to this socket
	float GetDistanceFromAPawn(const AMoverPawn* InPawn) const;

	//Attaches this socket's Grappler Rope onto the Pawns hand
	void AttachToGrappleSocket(const AMoverPawn* InPawn);

	//Attaches a pawn and its grappler onto this socket
	void AttachPawnToGrappleSocket(AMoverPawn* InPawn) const;

	//Deattaches a pawn and its grappler from this socket
	void DetachFromGrappleSocket(AMoverPawn* InPawn, bool bApplyForce) const;

	//Constructs necessary Grapple Rope variables for current Grapple Socket
	void ConstructGrappleRope(AMoverPawn* InPawn) const;

	//Constructs the FLayeredMove for when pawn needs to push off from the Grapple Socket
	static TSharedPtr<FLayeredMove_Launch> ConstructGrappleMove(const AMoverPawn* InPawn, const FVector& LaunchVelocity);

	//Function to enable physics for grappling swinging motion
	void EnableGrappling() const;

	//Check if two vectors have similar values within a range
	static bool FVectorAlmostTheSame(const FVector& A, const FVector& B, const float Range);

	//Logic so pawn swing around using WASD while grappling
	void ApplyForceToGrappleMotion(const FInputActionValue& Value);

	
private:
	//Root Component Reference
	UPROPERTY()
	TObjectPtr<USceneComponent> Root = nullptr;
	
	//Player pawn Reference
	UPROPERTY()
	TObjectPtr<AMoverPawn> Cached_PlayerPawn = nullptr;

	//Camera Actor Reference
	UPROPERTY()
	TObjectPtr<UCameraComponent> Cached_Camera = nullptr;

	//Static Mesh Component Reference
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent = nullptr;
	
	//Grappling Socket Widget Component Reference
	UPROPERTY()
	TObjectPtr<UGrapplingSocketWidgetComponent> GrapplingSocketWidgetComponent = nullptr;

	//Cached Binding Handle so we can bind and unbind applying swing force to grapple rope
	UPROPERTY()
	uint32 BindingHandle = 0;

public:
	//Able to change Min Distance To Grapple in the editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Socket")
	float MinDistanceToGrapple = 0.0f;

	//Able to enable Image in the editor 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Socket")
	bool bEnableImage = true;

	//Able to enable Text in the editor 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Socket")
	bool bEnableText = true;

	//A maximum value which the Grappler Rope can get
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Socket")
	float MaxGrapplerRopeLength  = 0.0f;

	//An extra value to add or remove length to get a nice proper length on the Grapple Rope
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Socket")
	float GrapplerRopeExtraShaveOffFromOrAddToLength  = 0.0f;

	//Grapple Rope Reference
	UPROPERTY(EditAnywhere, BlueprintReadOnly,  Category = "Grapple Socket")
	TObjectPtr<UCableComponent> GrappleRope = nullptr;

	//Grapple Edge Component Reference, this is for the Pawn to grapple onto
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple Socket")
	TObjectPtr<UCapsuleComponent>  GrappleEdgeComponent= nullptr;

	//Grapple Area Range Component Reference, this is for the Pawn to grapple onto
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple Socket")
	TObjectPtr<USphereComponent>  GrappleAreaComponent = nullptr;

	//If we want to enable or disable all debugging text
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Socket")
	bool EnableDebuggingText = false;

	//If we want to enable or disable debug text for grappling onto logic
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Socket")
	bool EnableGrappleOntoDebuggingText = false;

	//If we want to enable or disable debug text for construction of rope logic
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Socket")
	bool EnableConstructRopeDebuggingText = false;

	//If we want to enable or disable debug text for grappling logic 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Socket")
	bool EnableGrapplingActionDebuggingText = false;
};
