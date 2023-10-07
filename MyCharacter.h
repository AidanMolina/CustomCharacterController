// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "MyCharacter.generated.h"

UCLASS()
class CHARACTERCONTROLLER_API AMyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMyCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	UPROPERTY(EditAnywhere)
	class UCameraComponent* Camera;

	UPROPERTY(EditAnywhere)
	bool isOnGround;
	bool isOnWall;
	bool canDoubleJump;
	bool canDash;
	bool groundPounding;

	UPROPERTY(EditAnywhere)
	class UCapsuleComponent* Capsule;

	UPROPERTY(EditAnywhere)
	float traceRadius = 0.0f;
	float jumpVelocity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
	float dashVelocity = 0.0f;

	UPROPERTY(EditAnywhere)
	FVector wallJumpVector;

	void MoveForward(float Input);
	void MoveRight(float Input);

	void Turn(float Input);
	void LookUp(float Input);

private:
	void OnWall();
	void OnGround();
	void PerformJump();
	void Dash();
	void ResetDash();
	void GroundPound();

};
