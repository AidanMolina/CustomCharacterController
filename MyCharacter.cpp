// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCharacter.h"
#include "Camera/Cameracomponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

// Sets default values
AMyCharacter::AMyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->bUsePawnControlRotation = true;

	Capsule = GetCapsuleComponent();

	isOnGround = false;
	isOnWall = false;
	canDoubleJump = true;
	canDash = true;
	groundPounding = false;

	traceRadius = Capsule->GetUnscaledCapsuleRadius() * 1.1f;
	jumpVelocity = GetCharacterMovement()->JumpZVelocity;
	wallJumpVector = FVector(0.0f, 0.0f, 0.0f);

}

// Called when the game starts or when spawned
void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	OnWall();
	OnGround();

	if (isOnWall && !isOnGround)
	{
		GetCharacterMovement()->GravityScale = 0.0f;
		GetCharacterMovement()->Velocity.Z = 0.0f;
		GetCharacterMovement()->FallingLateralFriction = 1.0f;
	}
	else
	{
		GetCharacterMovement()->GravityScale = 1.0f;
		GetCharacterMovement()->FallingLateralFriction = 0.0f;
	}

	if (groundPounding)
	{
		LaunchCharacter(FVector(0.0f, 0.0f, -1000.0f), true, true);
	}
}

// Called to bind functionality to input
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	//Bind my player Actions and Axis for movement and jumping.
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMyCharacter::PerformJump);
	PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &AMyCharacter::Dash);
	PlayerInputComponent->BindAction("Pound", IE_Pressed, this, &AMyCharacter::GroundPound);
	PlayerInputComponent->BindAxis("MoveForward", this, &AMyCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMyCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &AMyCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AMyCharacter::LookUp);

}

void AMyCharacter::MoveForward(float Input)
{
	FVector ForwardDirection = GetActorForwardVector();
	AddMovementInput(ForwardDirection, Input);
}

void AMyCharacter::MoveRight(float Input)
{
	FVector RightDirection = GetActorRightVector();
	AddMovementInput(RightDirection, Input);
}

void AMyCharacter::Turn(float Input)
{
	AddControllerYawInput(Input);
}

void AMyCharacter::LookUp(float Input)
{
	AddControllerPitchInput(Input);
}

void AMyCharacter::PerformJump()
{
	//Check if the player exists
	if (this)
	{
		//Check if we are doing a ground jump or a wall jump
		if (isOnGround)
		{
			//If we are on the ground then perform a jump similar to the base DoJump in GetCharacterMovement()
			GetCharacterMovement()->Velocity.Z = jumpVelocity;
			GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		}
		else if(isOnWall && !isOnGround)
		{
			//If we are wall jumping, create a new vector to that will be our angle off the wall.
			FVector jumpAngle = FVector(0.0f, 0.0f, 0.0f);
			//If our Normal Vector's X is above 0, then set our jump angle to maintain our current Y velocity, and set X to be jumpVelocity.
			if (wallJumpVector.X != 0) {
				if (wallJumpVector.X > 0)
				{
					jumpAngle.X = jumpVelocity;
				}
				else
				{
					jumpAngle.X = jumpVelocity * -1;
				}
				jumpAngle.Y = GetCharacterMovement()->Velocity.Y;
			}
			//If our Normal Vector's Y is above 0, then set our jump angle to maintain our current X velocity, and set Y to be jumpVelocity.
			else
			{
				if (wallJumpVector.Y > 0)
				{
					jumpAngle.Y = jumpVelocity;
				}
				else
				{
					jumpAngle.Y = jumpVelocity * -1;
				}
				jumpAngle.X = GetCharacterMovement()->Velocity.X;
			}
			//After these checks, set our jump angle's Z to jumpVelocity.
			//Now we jump away from the wall, at our jumpVelocity while maintaining our current velocity's direction.
			jumpAngle.Z = jumpVelocity;
			GetCharacterMovement()->Velocity = jumpAngle;
			GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		}
		else if (!isOnGround && !isOnWall && canDoubleJump)
		{
			GetCharacterMovement()->Velocity.Z = jumpVelocity;
			canDoubleJump = false;
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, "CAN NOT DOUBLE JUMP!");
		}
	}
}

void AMyCharacter::Dash()
{
	if (canDash)
	{
		canDash = false;
		FVector DashDirection = GetCharacterMovement()->GetLastInputVector();
		if (GetCharacterMovement()->Velocity == FVector(0.0f, 0.0f, 0.0f))
		{
			DashDirection = Camera->GetForwardVector();
		}
		LaunchCharacter(FVector(DashDirection.X * dashVelocity, DashDirection.Y * dashVelocity, 100.0f), true, true);
		FTimerHandle UnusedHandle;
		GetWorldTimerManager().SetTimer(UnusedHandle, this, &AMyCharacter::ResetDash, 1.0f, false);
	}
}

void AMyCharacter::ResetDash()
{
	canDash = true;
}

//Function to check if the character is colliding with a wall or not.
void AMyCharacter::OnWall()
{
	TArray<FHitResult> HitResults;

	//Get starting position of our line, then get the four directions the lines will shoot.
	const FVector start = GetActorLocation();
	const FVector end = GetActorLocation();

	const FCollisionShape shape = FCollisionShape::MakeSphere(traceRadius);
	FCollisionQueryParams CollisionParameters;
	CollisionParameters.AddIgnoredActor(this);

	bool hit = GetWorld()->SweepMultiByChannel(HitResults, start, end, FQuat::Identity, ECC_Pawn, shape, CollisionParameters);

	DrawDebugSphere(GetWorld(), start, traceRadius, 2.0f, FColor::Blue, false, 2.0f);

	if (hit)
	{
		isOnWall = true;
		for (FHitResult const HitResult : HitResults)
		{
			wallJumpVector = HitResult.Normal;
			canDoubleJump = true;
			groundPounding = false;
			//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, wallJumpVector.ToString());
		}
	}
	else
	{
		isOnWall = false;
	}
}

//Function to check if the character is grounded or not.
void AMyCharacter::OnGround() 
{
	//Get starting position of our line trace and which direction it will be going
	FVector start = GetActorLocation();
	FVector down = GetActorUpVector() * -1;
	
	//Get the height of our capsule so we can reach the bottom of our player collision and not hard code a value, so we can scale player collision
	float height = Capsule->GetUnscaledCapsuleHalfHeight();

	//Calculate the end of our line trace by taking the start position, then adding the direction multiplied by the height of
	//our collision capsule, and making it 10% larger by multiplying it by 1.1 so we can hit the ground accurately.
	FVector end = start + (down * height * 1.1f);

	//Set up our parameters for our line trace, and exclude hitting the player because we do not want a false positive for isOnGround.
	FHitResult hit;
	FCollisionQueryParams CollisionParameters;
	CollisionParameters.AddIgnoredActor(this);

	//If the world exists, start a line trace
	if (GetWorld())
	{
		bool hitSomething = GetWorld()->LineTraceSingleByChannel(hit, start, end, ECC_Pawn, CollisionParameters, FCollisionResponseParams());
		//Draw the line so I can see it.
		DrawDebugLine(GetWorld(), start, end, FColor::Red, false, 2.0f, 0.0f, 10.0f);
		//If we hit something, we're on the ground.
		if (hitSomething && hit.GetActor())
		{
			isOnGround = true;
			canDoubleJump = true;
			groundPounding = false;
			//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, "On The Ground");
		}
		//If we don't hit anything, we're not on the ground.
		else
		{
			isOnGround = false;
			//->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, "NOT ON THE GROUND!");
		}
	}
}

void AMyCharacter::GroundPound()
{
	if (!isOnGround && !isOnWall)
	{
		groundPounding = true;
	}
}



