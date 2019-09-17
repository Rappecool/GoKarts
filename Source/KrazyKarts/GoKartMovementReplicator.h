// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "GoKartMovementComponent.h"
#include "GoKartMovementReplicator.generated.h"

USTRUCT()
struct FGoKartState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	FVector Velocity;

	UPROPERTY()
	FGoKartMove LastMove;
};

struct FHermiteCubicSpline
{
	FVector StartLocation, StartDerivative, TargetLocation, TargetDerivative;

	FVector InterpolateLocation(float LerpRatio) const
	{
		return FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	}
	FVector InterpolateDerivative(float LerpRatio) const
	{
		return FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	}

};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartMovementReplicator : public UActorComponent
{
	GENERATED_BODY()

private:
	UPROPERTY(VisibleAnywhere)
	class UGoKartMovementComponent* MovementComponent;

	TArray<FGoKartMove> UnacknowledgedMoves;

	float ClientTimeSinceUpdate;
	float ClientTimeBetweenLastUpdates;
	FTransform ClientStartTransform;
	FVector ClientStartVelocity;

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
	FGoKartState ServerState;

	UPROPERTY()
	USceneComponent* MeshOffSetRoot;

	void ClearAcknowledgedMoves(FGoKartMove LastMove);
	void UpdateServerState(const FGoKartMove& Move);
	void ClientTick(float DeltaTime);

	FHermiteCubicSpline CreateCubicSpline();
	void InterpolateLocation(const FHermiteCubicSpline &Spline, float LerpRatio);
	void InterpolateVelocity(const FHermiteCubicSpline &Spline, float LerpRatio);
	void InterpolateRotation(float LerpRatio);

	float VelocityToDerivative();

	UFUNCTION(BlueprintCallable)
	void SetMeshOffsetRoot(USceneComponent* Root) { MeshOffSetRoot = Root; }
	void SetMeshOffsetLocation();

	UFUNCTION()
	void OnRep_ServerState();
	void OnRep_SimulatedProxy_ServerState();
	void OnRep_Autonomous_ServerState();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Sets default values for this component's properties
	UGoKartMovementReplicator();
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FGoKartMove Move);
};
