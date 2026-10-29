#include "Cua.h"

#include "AmThanh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "VatLieu.h"

namespace
{
	constexpr float RongCanh = 100.f;
	constexpr float CaoCanh = 210.f;
	constexpr float DayCanh = 5.f;

	UStaticMeshComponent* KhoiCon(AActor* Chu, USceneComponent* Cha, const FVector& Tam, const FVector& CoCm, UMaterialInterface* M)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Chu);
		C->SetStaticMesh(VatLieu::Khoi());
		C->SetupAttachment(Cha);
		C->SetRelativeLocation(Tam);
		C->SetRelativeScale3D(CoCm / 100.f);
		C->SetMaterial(0, M);
		C->RegisterComponent();
		return C;
	}
}

ACua::ACua()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Goc"));
	BanLe = CreateDefaultSubobject<USceneComponent>(TEXT("BanLe"));
	BanLe->SetupAttachment(RootComponent);
	BanLe->SetRelativeLocation(FVector(0.f, -RongCanh * 0.5f, 0.f));
}

void ACua::Dung(const FString& TenBien, bool bCoKinh, const FLinearColor& MauSon)
{
	Canh = KhoiCon(this, BanLe, FVector(0.f, RongCanh * 0.5f, CaoCanh * 0.5f), FVector(DayCanh, RongCanh - 2.f, CaoCanh), VatLieu::BeMat(this, TEXT("cua"), MauSon, 0.55f));
	UMaterialInterface* Inox = VatLieu::Mau(this, FLinearColor(0.6f, 0.6f, 0.58f), 0.3f, 1.f);
	// Tay nắm hai mặt (tọa độ theo bản lề)
	for (float Mat : { 1.f, -1.f })
	{
		KhoiCon(this, BanLe, FVector(Mat * 6.f, RongCanh - 12.f, 100.f), FVector(4.f, 14.f, 3.f), Inox);
		KhoiCon(this, BanLe, FVector(Mat * 4.f, RongCanh - 8.f, 100.f), FVector(4.f, 3.f, 8.f), Inox);
	}
	if (bCoKinh)
		KhoiCon(this, BanLe, FVector(0.f, RongCanh * 0.5f, 158.f), FVector(DayCanh + 1.f, 30.f, 45.f), VatLieu::Mau(this, FLinearColor(0.02f, 0.025f, 0.03f), 0.05f, 0.f));
	if (!TenBien.IsEmpty())
		KhoiCon(this, BanLe, FVector(DayCanh * 0.5f + 0.6f, RongCanh * 0.5f, 172.f), FVector(1.f, 48.f, 15.f), VatLieu::BeMat(this, TenBien, FLinearColor::White, 0.4f, 0.05f));
}

void ACua::Tick(float Dt)
{
	Super::Tick(Dt);
	const float TocDoThuc = bDangDap ? TocDo * 4.f : TocDo;
	Goc = FMath::FInterpConstantTo(Goc, GocDich, Dt, TocDoThuc);
	float Lac = 0.f;
	if (ThoiRung > 0.f)
	{
		ThoiRung -= Dt;
		Lac = FMath::Sin(ThoiRung * 70.f) * 0.8f;
	}
	BanLe->SetRelativeRotation(FRotator(0.f, Goc + Lac, 0.f));
	if (bDangDap && FMath::IsNearlyEqual(Goc, GocDich)) bDangDap = false;
}

void ACua::Mo(bool bAmThanh)
{
	if (FMath::IsNearlyEqual(GocDich, GocMo)) return;
	GocDich = GocMo;
	bDangDap = false;
	if (bAmThanh) AmThanh::Phat3D(this, TEXT("cua_mo"), GetActorLocation() + FVector(0, 0, 100), 0.8f);
}

void ACua::Dong(bool bDap)
{
	if (FMath::IsNearlyZero(GocDich)) return;
	GocDich = 0.f;
	bDangDap = bDap;
	AmThanh::Phat3D(this, bDap ? TEXT("cua_dap") : TEXT("cua_dong"), GetActorLocation() + FVector(0, 0, 100), bDap ? 1.2f : 0.7f);
}

void ACua::DatGoc(float G)
{
	Goc = GocDich = G;
	bDangDap = false;
	BanLe->SetRelativeRotation(FRotator(0.f, Goc, 0.f));
}

void ACua::Rung()
{
	ThoiRung = 0.6f;
	AmThanh::Phat3D(this, TEXT("tay_nam_cua"), GetActorLocation() + FVector(0, 0, 100), 0.8f);
}
