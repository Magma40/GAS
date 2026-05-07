// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MovementMode.h"
#include "GrapplingMovementMode.generated.h"

/**
 * This class is deprecated and is not in use
 */
UCLASS()
class GAS_API UGrapplingMovementMode : public UBaseMovementMode
{
	GENERATED_BODY()

public:

	FVector GrapplePoint;
	float RopeLength = 0.0f;
	float SwingForce = 3000.0f;

	virtual void GenerateMove_Implementation(const FMoverTickStartData& StartState,const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const override;
	virtual void SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState) override;
};
