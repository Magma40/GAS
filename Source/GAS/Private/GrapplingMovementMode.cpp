// Fill out your copyright notice in the Description page of Project Settings.


#include "GrapplingMovementMode.h"

/*
 * NOTE: This class is not in use
 */

void UGrapplingMovementMode::GenerateMove_Implementation(const FMoverTickStartData& StartState,const FMoverTimeStep& TimeStep, FProposedMove& OutProposedMove) const
{
	const float DeltaTime = TimeStep.StepMs * 0.001f;
	
	if (const FMoverDefaultSyncState* DefaultState = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>())
	{
		const FVector Position = DefaultState->GetLocation_WorldSpace();
		const FVector Velocity =  DefaultState->GetVelocity_WorldSpace();

		FVector ToPlayer = Position - GrapplePoint;
		FVector RopeDir = ToPlayer.GetSafeNormal();

		FVector Tangent;

		if (!Velocity.IsNearlyZero())
		{
			FVector Axis = FVector::CrossProduct(RopeDir, Velocity).GetSafeNormal();
			Tangent = FVector::CrossProduct(Axis, RopeDir).GetSafeNormal();
		}
		else
		{
			Tangent = FVector::CrossProduct(RopeDir, FVector::UpVector).GetSafeNormal();
		}

		if (const FCharacterDefaultInputs* MoveInputs = StartState.InputCmd.InputCollection.FindDataByType<FCharacterDefaultInputs>())
		{
			const FVector MoveInput  = MoveInputs->GetMoveInput();
			
			float InputAmount = MoveInput.X;

			OutProposedMove.LinearVelocity += Tangent * InputAmount * SwingForce * DeltaTime;
		}
		

	}

}

void UGrapplingMovementMode::SimulationTick_Implementation(const FSimulationTickParams& Params, FMoverTickEndData& OutputState)
{

	if (const FMoverDefaultSyncState* DefaultState = OutputState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>())
	{
		FVector Position = DefaultState->GetLocation_WorldSpace();
		FVector Velocity =  DefaultState->GetVelocity_WorldSpace();

		FVector ToPlayer = Position - GrapplePoint;
		float Distance = ToPlayer.Size();

		if (Distance < KINDA_SMALL_NUMBER)
			return;

		FVector RopeDir = ToPlayer / Distance;
		
		if (Distance > RopeLength)
		{
			FVector Corrected = GrapplePoint + RopeDir * RopeLength;
			DefaultState->GetTransform_WorldSpace().SetLocation(Corrected);

			float OutwardSpeed = FVector::DotProduct(Velocity, RopeDir);
			if (OutwardSpeed > 0)
			{
				Velocity -= RopeDir * OutwardSpeed;
			}
		}
		float RadialSpeed = FVector::DotProduct(Velocity, RopeDir);
		Velocity -= RopeDir * RadialSpeed;
		
		DefaultState->GetVelocity_WorldSpace().Set(Velocity.X, Velocity.Y, Velocity.Z);
	}
}

