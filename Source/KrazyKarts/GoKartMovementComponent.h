// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.generated.h"

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

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartMovementComponent : public UActorComponent
{
	GENERATED_BODY()

private:
	/** The current speed as a string eg 10 km/h */
	UPROPERTY(Category = Display, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FText SpeedDisplayString;

	//Mass of car in (kg).
	UPROPERTY(EditAnywhere)
	float Mass = 1000;
	FVector Velocity;

	float Throttle;
	float SteeringThrow;

	FGoKartMove LastMove;

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

	FVector GetAirResistance();
	FVector GetRollResistance();

	struct FGoKartMove CreateMove(float DeltaTime);
	void UpdateLocationFromVelocity(float DeltaTime, FHitResult &HitResult);
	void ApplyRotation(float DeltaTime, float SteeringThrow);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Sets default values for this component's properties
	UGoKartMovementComponent();
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//Simulates by reference instead of copying each time.
	void SimulateMove(const struct FGoKartMove& Move);

	FVector GetVelocity() const;
	FGoKartMove GetLastMove() { return LastMove; };

	void SetVelocity(FVector Value);
	void SetThrottle(float Value);
	void SetSteeringThrow(float Value);

		
};
