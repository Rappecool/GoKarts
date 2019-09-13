// Fill out your copyright notice in the Description page of Project Settings.

#include "GoKart.h"
#include "GoKartMovementComponent.h"

#include "Components/InputComponent.h"
#include "Runtime/Engine/Classes/GameFramework/SpringArmComponent.h"
#include "Runtime/Engine/Classes/Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "UnrealNetwork.h"
#include "Engine/World.h"

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//enables replication on actor.
	bReplicates = true;

	MovementComponent = CreateDefaultSubobject<UGoKartMovementComponent>(TEXT("MovementComponent"));
}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		NetUpdateFrequency = 1;
	}
}

void AGoKart::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGoKart, ServerState);
}

void AGoKart::ClearAcknowledgedMoves(FGoKartMove LastMove)
{
	TArray<FGoKartMove> NewMoves;

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		if (Move.Time > LastMove.Time)
		{
			NewMoves.Add(Move);
		}
	}
	UnacknowledgedMoves = NewMoves;
}

FString GetEnumText(ENetRole Role)
{
	switch (Role)
	{
	case ROLE_None:
		return "None";
	case ROLE_SimulatedProxy:
		return "ROLE_SimulatedProxy";
	case ROLE_AutonomousProxy:
		return "ROLE_AutonomousProxy";
	case ROLE_Authority:
		return "ROLE_Authority";
	default:
		return "Error";
	}
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!ensure(MovementComponent != nullptr))
		return;

		//TODO: use reference to movementcomponent to call.
	if (Role == ROLE_AutonomousProxy)
	{
		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		//simulate move locally. (As to not simulate move on server twice.)
		MovementComponent->SimulateMove(Move);
		UnacknowledgedMoves.Add(Move);
		//Moves client on server, client calls Server_SendMove -> Makes RPC to Server_SendMove_Implementation on server. TODO: add HasAuthority?
		Server_SendMove(Move);	
	}

	//We are the client, simulated are other clients/server in game.
	if (Role == ROLE_SimulatedProxy)
	{
		MovementComponent->SimulateMove(ServerState.LastMove);
	}

	//TODO: Move GetRemoteRole out of Tick.
	//We are the server and in control of the pawn.
	if (Role == ROLE_Authority && GetRemoteRole() == ROLE_SimulatedProxy)
	{
		FGoKartMove Move = MovementComponent->CreateMove(DeltaTime);
		Server_SendMove(Move);
	}

	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(Role), this, FColor::White, DeltaTime);
}

void AGoKart::OnRep_ServerState()
{
	if (!ensure(MovementComponent != nullptr))
		return;

		//Sets/overrides values from client on server.
	SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	//Clears Moves that have been acknowledged.
	ClearAcknowledgedMoves(ServerState.LastMove);

		//all moves that haven't been acknowledged, simulate.
	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulateMove(Move);
	}
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);
}

FGoKartState AGoKart::GetServerState() const
{
	return ServerState;
}

void AGoKart::MoveForward(float Value)
{
	MovementComponent->SetThrottle(Value);
}

void AGoKart::MoveRight(float Value)
{
	MovementComponent->SetSteeringThrow(Value);
}

void AGoKart::Server_SendMove_Implementation(FGoKartMove Move)
{
	if (!ensure(MovementComponent != nullptr))
		return;

	MovementComponent->SimulateMove(Move);

	ServerState.LastMove = Move;

	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
}

bool AGoKart::Server_SendMove_Validate(FGoKartMove Move)
{
	//CLEAN one liner for checking that Value is within range -1 to 1.
	//return FMath::Abs(Value) <= 1;

		//TODO: Make better validation.
	return true;
}