#include "BongNguoi.h"

#include "Animation/AnimSequence.h"
#include "BanDo.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "VatLieu.h"

ABongNguoi::ABongNguoi()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Goc"));
	Than = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Than"));
	Than->SetupAttachment(RootComponent);
	// Mannequin nhìn theo +Y trong lưới → xoay -90 để nhìn theo +X của actor; kéo cao gầy
	Than->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Than->SetRelativeScale3D(FVector(0.86f, 0.86f, 1.12f));
	Than->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Than->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	MatNa = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MatNa"));
	MatNa->SetupAttachment(RootComponent);
	MatNa->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AnhChieu = CreateDefaultSubobject<UPointLightComponent>(TEXT("AnhChieu"));
	AnhChieu->SetupAttachment(RootComponent);
	AnhChieu->SetRelativeLocation(FVector(55.f, 0.f, 140.f));
	AnhChieu->SetIntensityUnits(ELightUnits::Lumens);
	AnhChieu->SetIntensity(0.f);
	AnhChieu->SetAttenuationRadius(260.f);
	AnhChieu->SetLightColor(FLinearColor(0.6f, 0.7f, 1.f));
}

void ABongNguoi::BeginPlay()
{
	Super::BeginPlay();
	USkeletalMesh* Ma = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/BongMa/bong_ma.bong_ma"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	AnhMa = VatLieu::HoatAnh(TEXT("/Game/BongMa/bong_ma_Anim.bong_ma_Anim"));
	if (Ma && AnhMa)
	{
		// Bóng ma: cô gái tóc đen dáng đi xác sống; mọi kiểu di chuyển đều dùng hoạt ảnh này ở tốc độ khác nhau
		bMa = true;
		DaiMa = AnhMa->GetPlayLength();
		Than->SetSkeletalMesh(Ma);
		// Texture gốc nhúng trong FBX không vào được Unreal → gán lại theo tên ô vật liệu (da / váy dùng chung ảnh váy)
		const TArray<FName> O = Than->GetMaterialSlotNames();
		for (int32 K = 0; K < O.Num(); ++K)
		{
			const FString T = O[K].ToString();
			const TCHAR* Anh = T.StartsWith(TEXT("saclar")) ? TEXT("ma_toc") : (T.StartsWith(TEXT("kafa")) ? TEXT("ma_dau") : TEXT("ma_vay"));
			Than->SetMaterial(K, VatLieu::BeMat(this, Anh, FLinearColor(0.8f, 0.78f, 0.76f), T.StartsWith(TEXT("saclar")) ? 0.35f : 0.55f));
		}
		Than->SetRelativeRotation(FRotator(0.f, GocMa, 0.f));
		Than->SetRelativeScale3D(FVector(1.08f));
		MatNa->SetVisibility(false);
		bCoMatNa = false;
		// Hoạt ảnh đứng / đi / chạy của mannequin đã chuyển sang khung xương bóng ma (Tools/chuyen_hoat_anh.py)
		UAnimSequence* D = VatLieu::HoatAnh(TEXT("/Game/BongMa/Retarget/MM_Idle_Ma.MM_Idle_Ma"));
		UAnimSequence* Di0 = VatLieu::HoatAnh(TEXT("/Game/BongMa/Retarget/MF_Unarmed_Walk_Fwd_Ma.MF_Unarmed_Walk_Fwd_Ma"));
		UAnimSequence* C = VatLieu::HoatAnh(TEXT("/Game/BongMa/Retarget/MF_Unarmed_Jog_Fwd_Ma.MF_Unarmed_Jog_Fwd_Ma"));
		if (D && Di0 && C)
		{
			AnhDung = D; AnhDi = Di0; AnhChay = C;
			bMa = false;   // dùng đường hoạt ảnh thường
		}
		AnChieu();
		An();
		return;
	}
	if (USkeletalMesh* M = VatLieu::Manny())
	{
		Than->SetSkeletalMesh(M);
		UMaterialInstanceDynamic* Da = VatLieu::Mau(this, FLinearColor(0.012f, 0.011f, 0.011f), 0.32f);
		for (int32 K = 0; K < FMath::Max(M->GetMaterials().Num(), Than->GetNumMaterials()); ++K) Than->SetMaterial(K, Da);
	}
	AnhDung = VatLieu::HoatAnh(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
	AnhDi = VatLieu::HoatAnh(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd.MF_Unarmed_Walk_Fwd"));
	AnhChay = VatLieu::HoatAnh(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd.MF_Unarmed_Jog_Fwd"));
	// Mặt nạ (đặt trước xương đầu mỗi khung, xem DatMatNa)
	MatNa->SetStaticMesh(VatLieu::MauTinh(TEXT("misc_toy_phantomMask_a01")));
	MatNa->SetRelativeScale3D(FVector(1.25f));
	An();
}

void ABongNguoi::ChoiHoatAnh(UAnimSequence* A, float TocDoPhat)
{
	if (bMa)
	{
		// Bóng ma chỉ có một hoạt ảnh: đứng = giữ một khung, đi chậm, rượt = đi giật nhanh
		const bool bDung = A == AnhDung;
		Than->PlayAnimation(AnhMa, true);
		Than->SetPlayRate(bDung ? (TocDoPhat > 0.f ? 0.15f : 0.f) : (A == AnhChay ? FMath::Max(TocDoPhat, 0.5f) * 2.4f : 1.3f));
		if (bDung) Than->SetPosition(DaiMa * 0.37f, false);
		return;
	}
	if (!A) return;
	Than->PlayAnimation(A, true);
	Than->SetPlayRate(TocDoPhat);
}

void ABongNguoi::An()
{
	CheDo = EBong::An;
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
}

void ABongNguoi::Dung(const FVector& ViTri, float Yaw, bool bDongBang)
{
	CheDo = EBong::Dung;
	SetActorLocationAndRotation(ViTri, FRotator(0.f, Yaw, 0.f));
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
	ChoiHoatAnh(AnhDung, bDongBang ? 0.f : 0.6f);
	if (bDongBang && AnhDung) Than->SetPosition(0.4f);
}

void ABongNguoi::Di(const FVector& Tu, const FVector& Toi, float TocDo, bool bAn, bool bChay)
{
	CheDo = EBong::Di;
	Dich = Toi;
	Toc = TocDo;
	bAnKhiToi = bAn;
	const FVector H = (Toi - Tu).GetSafeNormal2D();
	SetActorLocationAndRotation(Tu, H.Rotation());
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
	if (bChay) ChoiHoatAnh(AnhChay, TocDo / 380.f);
	else ChoiHoatAnh(AnhDi, TocDo / 150.f * 0.85f);
}

void ABongNguoi::Duoi(float S, float TocDo)
{
	CheDo = EBong::Duoi;
	QuangDuong = S;
	Toc = TocDo;
	SetActorLocationAndRotation(BanDo::DiemDuong(S), BanDo::HuongDuong(S).Rotation());
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
	ChoiHoatAnh(AnhChay, TocDo / 380.f);
}

void ABongNguoi::Bo(const FVector& Tu, const FVector& Toi, float TocDo, float Yaw)
{
	CheDo = EBong::Bo;
	Dich = Toi;
	Toc = TocDo;
	SetActorLocationAndRotation(Tu, FRotator(0.f, Yaw, 0.f));
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
	ChoiHoatAnh(AnhChay, 0.55f);
}

void ABongNguoi::CoGiat(const FVector& ViTri, float Yaw)
{
	CheDo = EBong::CoGiat;
	ViTriGoc = ViTri;
	YawGoc = Yaw;
	ChoGiat = 0.f;
	SetActorLocationAndRotation(ViTri, FRotator(0.f, Yaw, 0.f));
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
	ChoiHoatAnh(AnhChay, 0.f);
}

void ABongNguoi::DatMatNa()
{
	// Mặt nạ cao 17 cm, gốc ở mép dưới, mặt hướng +X: đặt trước xương đầu theo hướng actor
	if (!bCoMatNa) return;
	const FVector Dau = Than->GetSocketLocation(TEXT("head"));
	const FRotator R = GetActorRotation();
	MatNa->SetWorldLocationAndRotation(Dau + R.RotateVector(FVector(14.f, 0.f, -2.f)), R + FRotator(0.f, HuongMatNa, 0.f));
}

void ABongNguoi::Tick(float Dt)
{
	Super::Tick(Dt);
	DatMatNa();
	if (CheDo == EBong::Di)
	{
		const FVector P = GetActorLocation();
		const FVector D = Dich - P;
		if (D.Size2D() < Toc * Dt + 1.f)
		{
			SetActorLocation(Dich);
			if (bAnKhiToi) An();
			else { CheDo = EBong::Dung; ChoiHoatAnh(AnhDung, 0.5f); }
			return;
		}
		SetActorLocation(P + D.GetSafeNormal2D() * Toc * Dt);
	}
	else if (CheDo == EBong::Bo)
	{
		const FVector P = GetActorLocation();
		const FVector D = Dich - P;
		if (D.Size() < Toc * Dt + 1.f) { An(); return; }
		SetActorLocation(P + D.GetSafeNormal() * Toc * Dt);
	}
	else if (CheDo == EBong::CoGiat)
	{
		// Mỗi 0,05–0,25 s giật sang một tư thế khác, đầu và người lệch đi một chút
		if ((ChoGiat -= Dt) <= 0.f)
		{
			ChoGiat = FMath::FRandRange(0.05f, 0.25f);
			Than->SetPosition(FMath::FRandRange(0.f, bMa ? DaiMa : 0.7f), false);
			SetActorLocationAndRotation(ViTriGoc + FVector(FMath::FRandRange(-4.f, 4.f), FMath::FRandRange(-4.f, 4.f), FMath::FRandRange(-6.f, 2.f)),
				FRotator(FMath::FRandRange(-8.f, 8.f), YawGoc + FMath::FRandRange(-30.f, 30.f), FMath::FRandRange(-12.f, 12.f)));
		}
	}
	else if (CheDo == EBong::Duoi)
	{
		QuangDuong += Toc * Dt;
		const FVector P = BanDo::DiemDuong(QuangDuong);
		// Cua mượt ở góc rẽ
		const FVector H = (BanDo::DiemDuong(QuangDuong + 120.f) - P).GetSafeNormal2D();
		SetActorLocationAndRotation(P, FMath::RInterpTo(GetActorRotation(), H.Rotation(), Dt, 8.f));
	}
}
