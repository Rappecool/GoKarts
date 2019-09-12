// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"

USTRUCT()
struct FGoKartMove
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	float Throttle;
	UPROPERTY()
	float SteeringThrow;

	UPROPERTY()
	float DeltaTime;

	UPROPERTY()
	float Time;
};

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

UCLASS()
class KRAZYKARTS_API AGoKart : public APawn
{
	GENERATED_BODY()

private:
	TArray<FGoKartMove> UnacknowledgedMoves;

	/** The current speed as a string eg 10 km/h */
	UPROPERTY(Category = Display, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FText SpeedDisplayString;

	FVector Velocity;
	//Mass of car in (kg).
	UPROPERTY(EditAnywhere)
	float Mass = 1000;

	float Throttle;
	float SteeringThrow;

	//The force applied to the car when the throttle is fully down. (N)
	UPROPERTY(EditAnywhere)
	float MaxDrivingForce = 10000;
	//Minimum turning radius of our car at full lock(m).
	UPROPERTY(EditAnywhere)
	float MinTurningRadius = 10;
	//Higher means more drag.
	UPROPERTY(EditAnywhere)
	//Arrived at this value since Airresistance/Speed^2 = DragCoefficient, 10 000/25 = 16.
	float DragCoefficient = 16;
	UPROPERTY(EditAnywhere)
	//Arrived at this value from wikipedia, car tire on concrete.
	float RollingResistanceCoefficient = 0.015;

	float AccelerationDueToGravity = 0;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FGoKartMove Move);

	void MoveForward(float Value);
	void MoveRight(float Value);

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
	FGoKartState ServerState;

	FVector GetAirResistance();
	FVector GetRollResistance();
	void UpdateLocationFromVelocity(float DeltaTime, FHitResult &HitResult);
	void ApplyRotation(float DeltaTime, float SteeringThrow);

	UFUNCTION()
	void OnRep_ServerState();

	//Simulates by reference instead of copying each time.
	void SimulateMove(const FGoKartMove& Move);
	FGoKartMove CreateMove(float DeltaTime);
	void ClearAcknowledgedMoves(FGoKartMove LastMove);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	AGoKart();
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
