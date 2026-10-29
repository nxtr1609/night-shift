#include "DieuKhien.h"

#include "DaoDien.h"
#include "GiaoDien.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NguoiChoi.h"

ADieuKhien::ADieuKhien()
{
	bShouldPerformFullTickWhenPaused = true;
}

void ADieuKhien::BeginPlay()
{
	Super::BeginPlay();
	DatCheDoChuot(true);
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = -80.f;
		PlayerCameraManager->ViewPitchMax = 80.f;
	}
}

void ADieuKhien::DatCheDoChuot(bool bHien)
{
	if (bDangHienChuot == bHien) return;
	bDangHienChuot = bHien;
	bShowMouseCursor = bHien;
	if (bHien)
	{
		FInputModeGameAndUI M;
		M.SetHideCursorDuringCapture(false);
		SetInputMode(M);
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
}

void ADieuKhien::BamNut(const FString& Nut)
{
	ADaoDien* DD = ADaoDien::Lay(this);
	if (!DD) return;
	if (Nut == TEXT("moi")) DD->BatDau(1);
	else if (Nut == TEXT("tiep")) DD->BatDau(ADaoDien::Doc());
	else if (Nut == TEXT("tieptuc")) DD->TamDung(false);
	else if (Nut == TEXT("menu")) DD->VeMenu();
	else if (Nut == TEXT("thoat")) UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void ADieuKhien::PlayerTick(float Dt)
{
	Super::PlayerTick(Dt);
	ADaoDien* DD = ADaoDien::Lay(this);
	if (!DD || bMayChoi) return;
	ANguoiChoi* N = Cast<ANguoiChoi>(GetPawn());
	const bool bChoi = DD->TrangThai == ETrangThai::Choi;
	DatCheDoChuot(!bChoi);

	if (WasInputKeyJustPressed(EKeys::Escape))
	{
		if (DD->TrangThai == ETrangThai::Choi) DD->TamDung(true);
		else if (DD->TrangThai == ETrangThai::TamDung) DD->TamDung(false);
		else if (DD->TrangThai == ETrangThai::KetThuc) DD->VeMenu();
		return;
	}

	if (!bChoi)
	{
		if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
		{
			if (DD->TrangThai == ETrangThai::KetThuc)
			{
				if (DD->ThoiKetThuc > 12.f) DD->VeMenu();
				return;
			}
			float X, Y;
			if (AGiaoDien* G = Cast<AGiaoDien>(GetHUD()))
				if (GetMousePosition(X, Y)) BamNut(G->NutTai(FVector2D(X, Y)));
		}
		return;
	}

	if (!N) return;
	// Nhìn
	float DX = 0.f, DY = 0.f;
	GetInputMouseDelta(DX, DY);
	if (!N->bKhoa)
	{
		FRotator R = GetControlRotation();
		R.Yaw += DX * 2.5f * DoNhay;
		R.Pitch = FMath::ClampAngle(FRotator::NormalizeAxis(R.Pitch + DY * 2.5f * DoNhay), -80.f, 80.f);
		SetControlRotation(R);
	}
	// Đi
	FVector2D Truc(0.f, 0.f);
	if (IsInputKeyDown(EKeys::W)) Truc.Y += 1.f;
	if (IsInputKeyDown(EKeys::S)) Truc.Y -= 1.f;
	if (IsInputKeyDown(EKeys::D)) Truc.X += 1.f;
	if (IsInputKeyDown(EKeys::A)) Truc.X -= 1.f;
	N->DiChuyen(Truc, IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift));
	if (WasInputKeyJustPressed(EKeys::E)) DD->TuongTac();
	if (WasInputKeyJustPressed(EKeys::LeftMouseButton)) DD->NamVat(true);
	if (WasInputKeyJustReleased(EKeys::LeftMouseButton)) DD->NamVat(false);
}
