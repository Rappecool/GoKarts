// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKartMovementComponent.h"

#include "GoKart.generated.h"

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

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
	FGoKartState ServerState;

	UPROPERTY(EditAnywhere)
	class UGoKartMovementComponent* MovementComponent;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FGoKartMove Move);

	void MoveForward(float Value);
	void MoveRight(float Value);
	UFUNCTION()
	void OnRep_ServerState();

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

	FGoKartState GetServerState()const;
};
