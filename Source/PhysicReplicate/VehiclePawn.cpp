#include "VehiclePawn.h"

#include "VehicleMovementComponent.h"
#include "WheelFront.h"
#include "WheelRear.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

AVehiclePawn::AVehiclePawn(){
	const static ConstructorHelpers::FObjectFinder<USkeletalMesh> SedanMesh(TEXT("/Game/Vehicle/Sedan/Sedan_SkelMesh"));
	const static ConstructorHelpers::FClassFinder<UAnimInstance> AnimClass(TEXT("/Game/Vehicle/Sedan/Sedan_AnimBP"));
	
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
	
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetSkeletalMesh(SedanMesh.Object);
	MeshComponent->SetAnimClass(AnimClass.Class);
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetCollisionProfileName("Vehicle");
	MeshComponent->SetEnableGravity(true);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(MeshComponent);
	CameraBoom->TargetArmLength = 500.f;
	CameraBoom->SocketOffset = FVector(0,0,30.f);
	CameraBoom->SetRelativeLocation(FVector(0.0,0.0,170.0));
	CameraBoom->bUsePawnControlRotation = true;
	
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(CameraBoom,USpringArmComponent::SocketName);
	
	VehicleMovementComponent = CreateDefaultSubobject<UVehicleMovementComponent>(TEXT("VehicleMovementComponent"));
	VehicleMovementComponent->SetUpdatedComponent(GetRootComponent());
	VehicleMovementComponent->SetOwner(this);
	VehicleMovementComponent->SetIsReplicated(true);

	VehicleMovementComponent->WheelSetups.SetNum(4);
	VehicleMovementComponent->WheelSetups[0].WheelClass = UWheelFront::StaticClass();
	VehicleMovementComponent->WheelSetups[0].BoneName = FName("Wheel_Front_Left");
	
	VehicleMovementComponent->WheelSetups[1].WheelClass = UWheelFront::StaticClass();
	VehicleMovementComponent->WheelSetups[1].BoneName = FName("Wheel_Front_Right");

	VehicleMovementComponent->WheelSetups[2].WheelClass = UWheelRear::StaticClass();
	VehicleMovementComponent->WheelSetups[2].BoneName = FName("Wheel_Rear_Right");

	VehicleMovementComponent->WheelSetups[3].WheelClass = UWheelRear::StaticClass();
	VehicleMovementComponent->WheelSetups[3].BoneName = FName("Wheel_Rear_Left");
}

void AVehiclePawn::BeginPlay(){
	Super::BeginPlay();
}

void AVehiclePawn::Tick(float DeltaTime){
	Super::Tick(DeltaTime);
}

void AVehiclePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent){
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward",this,&AVehiclePawn::MoveFB);
	PlayerInputComponent->BindAxis("MoveRight",this,&AVehiclePawn::MoveRL);
	PlayerInputComponent->BindAxis("LookRight",this,&AVehiclePawn::LookRight);
	PlayerInputComponent->BindAxis("LookUp",this,&AVehiclePawn::LookUp);
}

void AVehiclePawn::MoveFB(float Value){
	VehicleMovementComponent->SetThrottle(Value);
}

void AVehiclePawn::MoveRL(float Value){
	VehicleMovementComponent->SetSteer(Value);
}

void AVehiclePawn::LookRight(float Value){
	AddControllerYawInput(Value * GetWorld()->GetDeltaSeconds() * 45.f);
}

void AVehiclePawn::LookUp(float Value){
	AddControllerPitchInput(Value * GetWorld()->GetDeltaSeconds() * 45.f);
}
