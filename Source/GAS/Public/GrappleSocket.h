// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrappleSocket.generated.h"

class UCameraComponent;
class UGrapplingSocketWidgetComponent;

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

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//Checks if a pawn is close enough to be able to grapple onto this socket
	bool IsInRangeToGrapple(const APawn* InPawn) const;

	//Gets the distance from a pawn to this socket
	float GetDistanceFromAPawn(const APawn* InPawn) const;

	//Attaches a pawn and its grappler onto this socket
	void AttachToGrappleSocket(APawn* InPawn);

	//Deattaches a pawn and its grappler from this socket
	void DetachToGrappleSocket(APawn* InPawn);
	
private:
	//Player pawn Reference
	UPROPERTY() APawn* Cached_PlayerPawn = nullptr;

	//Camera Actor Reference
	UPROPERTY() UCameraComponent* Cached_Camera = nullptr;

	UPROPERTY() UStaticMeshComponent* StaticMeshComponent = nullptr;
	
	//Grappling Socket Widget Component Reference
	UPROPERTY()  UGrapplingSocketWidgetComponent* GrapplingSocketWidgetComponent = nullptr;

public:
	//Able to change Min Distance To Grapple in the editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Socket") float MinDistanceToGrapple = 0.0f;

	//Able to enable Image in the editor 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Socket") bool bEnableImage = true;

	//Able to enable Text in the editor 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple Socket") bool bEnableText = true;

	UPROPERTY() USceneComponent* Root = nullptr;

};
