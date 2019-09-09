// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"

UCLASS()
class KRAZYKARTS_API AGoKart : public APawn
{
	GENERATED_BODY()

private:
	FVector Velocity;

	/** The current speed as a string eg 10 km/h */
	UPROPERTY(Category = Display, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FText SpeedDisplayString;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveForward(float Value);
	UFUNCTION(Server,Reliable,WithValidation)
	void Server_MoveRight(float Value);

	void MoveForward(float Value);
	void MoveRight(float Value);

	//Mass of car in (kg).
	UPROPERTY(EditAnywhere)
	float Mass = 1000;
	UPROPERTY(EditAnywhere)
	float Throttle;
	UPROPERTY(EditAnywhere)
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

	FVector GetAirResistance();
	FVector GetRollResistance();
	void UpdateLocationFromVelocity(float DeltaTime, FHitResult &HitResult);
	void ApplyRotation(float DeltaTime);

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
