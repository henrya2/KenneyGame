// Fill out your copyright notice in the Description page of Project Settings.

#include "KenneyGame.h"
#include "PaperHeroCharacter.h"
#include "PaperFlipbookComponent.h"

APaperHeroCharacter::APaperHeroCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bUseControllerRotationYaw = false;
	UCapsuleComponent* MyCapsuleComponent = GetCapsuleComponent();
	MyCapsuleComponent->SetCapsuleSize(40.0f, 62.0f);

	UPaperFlipbookComponent* SpriteComp = GetSprite();
	SpriteComp->SetRelativeLocation(FVector(0, 0, -64));

	CameraBoom = ObjectInitializer.CreateDefaultSubobject<USpringArmComponent>(this, TEXT("CameraBoom"));
	CameraBoom->SetRelativeRotation(FRotator(0, -90.0f, 0));
	CameraBoom->AttachTo(RootComponent);
	CameraBoom->TargetArmLength = 500.0f;
	CameraBoom->SocketOffset.Z = 75.0f;
	CameraBoom->bEnableCameraLag = false;
	CameraBoom->bEnableCameraRotationLag = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bDoCollisionTest = false;

	SideViewCamera = ObjectInitializer.CreateAbstractDefaultSubobject<UCameraComponent>(this, TEXT("SideViewCamera"));
	SideViewCamera->AttachTo(CameraBoom);
	SideViewCamera->ProjectionMode = ECameraProjectionMode::Orthographic;
	SideViewCamera->OrthoWidth = 1600;

	UCharacterMovementComponent* CharMovement = GetCharacterMovement();
	CharMovement->NavAgentProps.bCanCrouch = true;
	CharMovement->CrouchedHalfHeight = 50.0f;
	CharMovement->bConstrainToPlane = true;
	CharMovement->SetPlaneConstraintNormal(FVector(0, -1.0f, 0));
	CharMovement->JumpZVelocity = 1000.0f;
	CharMovement->SetWalkableFloorAngle(45.0f);
	CharMovement->GravityScale = 2.0f;
	CharMovement->GroundFriction = 3.0f;
	CharMovement->AirControl = 0.8f;

	HeroMoveDirection = EPaperHeroCharacterMoveDirection::Right;

	OnCharacterMovementUpdated.AddDynamic(this, &APaperHeroCharacter::DoCharMoveUpdated);

	static ConstructorHelpers::FObjectFinder<UPaperFlipbook> WalkFlipbookFinder(TEXT("PaperFlipbook'/Game/KenneySpritePack/Players/Green/alienGreen_walk.alienGreen_walk'"));
	if (WalkFlipbookFinder.Succeeded())
	{
		WalkFlipbook = WalkFlipbookFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UPaperFlipbook> IdleFlipbookFinder(TEXT("PaperFlipbook'/Game/KenneySpritePack/Players/Green/alienGreen_idle.alienGreen_idle'"));
	if (IdleFlipbookFinder.Succeeded())
	{
		IdleFlipbook = IdleFlipbookFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UPaperFlipbook> JumpFlipbookFinder(TEXT("PaperFlipbook'/Game/KenneySpritePack/Players/Green/alienGreen_jump_Flipbook.alienGreen_jump_Flipbook'"));
	if (JumpFlipbookFinder.Succeeded())
	{
		JumpFlipbook = JumpFlipbookFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UPaperFlipbook> DuckFlipbookFinder(TEXT("PaperFlipbook'/Game/KenneySpritePack/Players/Green/alienGreen_duck_Flipbook.alienGreen_duck_Flipbook'"));
	if (DuckFlipbookFinder.Succeeded())
	{
		DuckFlipbook = DuckFlipbookFinder.Object;
	}
}

void APaperHeroCharacter::SetupPlayerInputComponent(UInputComponent* InputComponent)
{
	Super::SetupPlayerInputComponent(InputComponent);

	InputComponent->BindAxis("MoveRight", this, &APaperHeroCharacter::MoveRight);
	InputComponent->BindAxis("MoveUp", this, &APaperHeroCharacter::MoveUp);

	InputComponent->BindAction("Jump", IE_Pressed, this, &APaperHeroCharacter::StartJump);
	InputComponent->BindAction("Jump", IE_Released, this, &APaperHeroCharacter::StopJump);
}

void APaperHeroCharacter::MoveRight(float Value)
{
	FVector RightVector(1.0f, 0, 0);
	AddMovementInput(RightVector, Value);

	EPaperHeroCharacterMoveDirection NewDir;
	if (Value > 0)
	{
		NewDir = EPaperHeroCharacterMoveDirection::Right;
	}
	else if (Value < 0)
	{
		NewDir = EPaperHeroCharacterMoveDirection::Left;
	}
	else
	{
		NewDir = EPaperHeroCharacterMoveDirection::KeepDir;
	}

	SetMoveDirection(NewDir);
}

void APaperHeroCharacter::MoveUp(float Value)
{
	//FString MsgStr = FString::Printf(TEXT("MoveUP's Value: %f"), Value);
	//GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Blue, MsgStr);

	if (!GetCharacterMovement()->IsFalling())
	{
		if (Value <= -0.3f)
		{
			Crouch();
		}
		else
		{
			UnCrouch();
		}
	}
}

void APaperHeroCharacter::StartJump()
{
	UnCrouch();
	Jump();
}

void APaperHeroCharacter::StopJump()
{
	StopJumping();
}

void APaperHeroCharacter::DoCharMoveUpdated(float DeltaSeconds, FVector OldLocation, FVector OldVelocity)
{
	FVector Velocity = GetVelocity();
	float Speed = FMath::Sqrt(Velocity.X * Velocity.X + Velocity.Z * Velocity.Z);

	bool bFalling = GetCharacterMovement()->IsFalling();

	if (bFalling)
	{
		GetSprite()->SetFlipbook(JumpFlipbook);
		UnCrouch();
	}
	else if (GetCharacterMovement()->IsCrouching())
	{
		GetSprite()->SetFlipbook(DuckFlipbook);
	}
	else
	{
		if (Speed > 0)
		{
			GetSprite()->SetFlipbook(WalkFlipbook);
		}
		else
		{
			GetSprite()->SetFlipbook(IdleFlipbook);
		}
	}
}

void APaperHeroCharacter::OnRep_HeroMoveDirection(EPaperHeroCharacterMoveDirection LastDir)
{
	UpdateSpriteDir(HeroMoveDirection);
}

void APaperHeroCharacter::SetMoveDirection(EPaperHeroCharacterMoveDirection NewDir)
{
	HeroMoveDirection = NewDir;

	UpdateSpriteDir(NewDir);

	if (Role < ROLE_Authority)
	{
		ServerSetMoveDirection(NewDir);
	}
}

bool APaperHeroCharacter::ServerSetMoveDirection_Validate(EPaperHeroCharacterMoveDirection NewDir)
{
	return true;
}

void APaperHeroCharacter::ServerSetMoveDirection_Implementation(EPaperHeroCharacterMoveDirection NewDir)
{
	SetMoveDirection(NewDir);
}

void APaperHeroCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(APaperHeroCharacter, HeroMoveDirection, COND_SkipOwner);
}

void APaperHeroCharacter::UpdateSpriteDir(EPaperHeroCharacterMoveDirection NewDir)
{
	if (HeroMoveDirection == EPaperHeroCharacterMoveDirection::Left)
	{
		GetSprite()->SetRelativeRotation(FRotator(0, 180.0f, 0));
	}
	else if (HeroMoveDirection == EPaperHeroCharacterMoveDirection::Right)
	{
		GetSprite()->SetRelativeRotation(FRotator(0, 0, 0));
	}
}

bool APaperHeroCharacter::CanJumpInternal_Implementation() const
{
	const bool bCanHoldToJumpHigher = (GetJumpMaxHoldTime() > 0.0f) && IsJumpProvidingForce();

	return GetCharacterMovement() && (GetCharacterMovement()->IsMovingOnGround() || bCanHoldToJumpHigher) && GetCharacterMovement()->IsJumpAllowed();
}

bool APaperHeroCharacter::CanCrouch()
{
	return !bIsCrouched && !bPressedJump && GetCharacterMovement() && GetCharacterMovement()->IsMovingOnGround() && GetCharacterMovement()->CanEverCrouch() && (GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() > GetCharacterMovement()->CrouchedHalfHeight) && GetRootComponent() && !GetRootComponent()->IsSimulatingPhysics();
}
