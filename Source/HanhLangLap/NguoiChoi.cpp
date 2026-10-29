#include "NguoiChoi.h"

#include "AmThanh.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"

ANguoiChoi::ANguoiChoi()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(30.f, 88.f);
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, CaoMat - 88.f));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(80.f);
	bUseControllerRotationYaw = true;
	UCharacterMovementComponent* M = GetCharacterMovement();
	M->MaxWalkSpeed = TocDi;
	M->BrakingDecelerationWalking = 900.f;
	M->MaxAcceleration = 900.f;
	M->JumpZVelocity = 0.f;
	M->MaxStepHeight = 25.f;
	// Đi vào đồ đẩy được (ghế, xe...) thì đẩy nó đi
	M->bEnablePhysicsInteraction = true;
	M->PushForceFactor = 300000.f;
	M->bPushForceScaledToMass = false;
	GetMesh()->SetVisibility(false);
	Tay = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("Tay"));
	Tay->LinearStiffness = 900.f;
	Tay->LinearDamping = 120.f;
	Tay->InterpolationSpeed = 12.f;
}

bool ANguoiChoi::Nam(UPrimitiveComponent* Vat, const FVector& Diem)
{
	if (!Vat || !Vat->IsSimulatingPhysics()) return false;
	Tay->GrabComponentAtLocationWithRotation(Vat, NAME_None, Diem, Vat->GetComponentRotation());
	KhoangNam = FMath::Clamp(FVector::Dist(ViTriMat(), Diem), 110.f, 200.f);
	XoayNam = (Vat->GetComponentRotation() - FRotator(0.f, GetControlRotation().Yaw, 0.f)).GetNormalized();
	return true;
}

void ANguoiChoi::Tha() { Tay->ReleaseComponent(); }

bool ANguoiChoi::DangNam() const { return Tay->GetGrabbedComponent() != nullptr; }

void ANguoiChoi::DiChuyen(const FVector2D& Truc, bool bChay)
{
	if (bKhoa || !Controller) return;
	GetCharacterMovement()->MaxWalkSpeed = bChay ? TocChay : TocDi;
	const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(Yaw.RotateVector(FVector::ForwardVector), Truc.Y);
	AddMovementInput(Yaw.RotateVector(FVector::RightVector), Truc.X);
}

void ANguoiChoi::Nhin(const FVector2D& Chuot)
{
	if (bKhoa) return;
	AddControllerYawInput(Chuot.X);
	AddControllerPitchInput(-Chuot.Y);
}

void ANguoiChoi::DichChuyen(const FTransform& Doi)
{
	const FVector V = GetCharacterMovement()->Velocity;
	SetActorLocation(Doi.TransformPosition(GetActorLocation()), false, nullptr, ETeleportType::TeleportPhysics);
	GetCharacterMovement()->Velocity = Doi.TransformVectorNoScale(V);
	if (Controller)
	{
		FRotator R = Controller->GetControlRotation();
		R.Yaw += Doi.Rotator().Yaw;
		Controller->SetControlRotation(R);
	}
}

FVector ANguoiChoi::ViTriMat() const { return Camera->GetComponentLocation(); }
FVector ANguoiChoi::HuongNhin() const { return Camera->GetForwardVector(); }

void ANguoiChoi::Tick(float Dt)
{
	Super::Tick(Dt);
	const float Toc = GetVelocity().Size2D();
	// Lắc đầu + tiếng bước chân mỗi nửa chu kỳ
	FVector Lech = FVector::ZeroVector;
	if (Toc > 20.f)
	{
		const float PhaTruoc = PhaBuoc;
		PhaBuoc += Dt * Toc / 72.f * PI;   // mỗi bước ~72 cm
		if (FMath::FloorToInt(PhaTruoc / PI) != FMath::FloorToInt(PhaBuoc / PI))
			AmThanh::Phat2D(this, TEXT("buoc_chan"), Toc > 200.f ? 0.5f : 0.3f, FMath::FRandRange(0.85f, 1.08f));
		const float Bien = Toc > 200.f ? 2.6f : 1.4f;
		Lech = FVector(0.f, FMath::Sin(PhaBuoc) * Bien * 0.5f, FMath::Abs(FMath::Cos(PhaBuoc)) * Bien - Bien * 0.5f);
	}
	if (DoRung > 0.f)
	{
		DoRung = FMath::Max(0.f, DoRung - Dt * 0.8f);
		Lech += FMath::VRand() * DoRung * 3.f;
	}
	// Đồ đang nắm theo trước mặt (giữ độ cao gần sàn, xoay theo người)
	if (DangNam())
	{
		FVector Dich = ViTriMat() + HuongNhin() * KhoangNam;
		Dich.Z = FMath::Clamp(Dich.Z, GetActorLocation().Z - 60.f, GetActorLocation().Z + 40.f);
		Tay->SetTargetLocationAndRotation(Dich, FRotator(0.f, GetControlRotation().Yaw, 0.f) + XoayNam);
	}
	Camera->SetRelativeLocation(FMath::VInterpTo(Camera->GetRelativeLocation(), FVector(0.f, 0.f, CaoMat - 88.f) + Lech, Dt, 12.f));
}
