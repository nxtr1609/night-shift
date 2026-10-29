#include "GiaoDien.h"

#include "CanvasItem.h"
#include "DaoDien.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"

namespace
{
	UFont* Font() { return GEngine->GetLargeFont(); }
}

FVector2D AGiaoDien::Chuot() const
{
	float X = -1.f, Y = -1.f;
	if (PlayerOwner) PlayerOwner->GetMousePosition(X, Y);
	return FVector2D(X, Y);
}

void AGiaoDien::Chu(const FString& S, float X, float Y, float Co, const FLinearColor& Mau, bool bGiua, bool bBong)
{
	const float TiLe = Co / 15.f * Canvas->ClipY / 1080.f;
	float W = 0.f, H = 0.f;
	Canvas->TextSize(Font(), S, W, H, TiLe, TiLe);
	const float X0 = bGiua ? X - W * 0.5f : X;
	const float Y0 = Y - H * 0.5f;
	if (bBong)
	{
		FCanvasTextItem B(FVector2D(X0 + 2.f, Y0 + 2.f), FText::FromString(S), Font(), FLinearColor(0, 0, 0, Mau.A * 0.8f));
		B.Scale = FVector2D(TiLe, TiLe);
		Canvas->DrawItem(B);
	}
	FCanvasTextItem T(FVector2D(X0, Y0), FText::FromString(S), Font(), Mau);
	T.Scale = FVector2D(TiLe, TiLe);
	Canvas->DrawItem(T);
}

void AGiaoDien::VeNut(const FString& Ten, const FString& ChuNut, float Y)
{
	const float W = Canvas->ClipX, K = Canvas->ClipY / 1080.f;
	const FBox2D Khung(FVector2D(W * 0.5f - 200.f * K, Y - 26.f * K), FVector2D(W * 0.5f + 200.f * K, Y + 26.f * K));
	const bool bTro = Khung.IsInside(Chuot());
	if (bTro)
	{
		FCanvasTileItem N(Khung.Min, Khung.GetSize(), FLinearColor(0.5f, 0.05f, 0.05f, 0.35f));
		N.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(N);
	}
	Chu(ChuNut, W * 0.5f, Y, 30.f, bTro ? FLinearColor(1.f, 0.9f, 0.85f) : FLinearColor(0.75f, 0.72f, 0.7f));
	Nut.Add({ Khung, Ten });
}

FString AGiaoDien::NutTai(const FVector2D& P) const
{
	for (const FNut& N : Nut)
		if (N.Khung.IsInside(P)) return N.Ten;
	return FString();
}

void AGiaoDien::DrawHUD()
{
	Super::DrawHUD();
	Nut.Reset();
	ADaoDien* DD = ADaoDien::Lay(this);
	if (!DD) return;
	const float W = Canvas->ClipX, H = Canvas->ClipY, K = H / 1080.f;
	auto Phu = [&](const FLinearColor& C)
	{
		FCanvasTileItem T(FVector2D(0, 0), FVector2D(W, H), C);
		T.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(T);
	};

	if (DD->TrangThai == ETrangThai::Choi && !DD->bTrailer)
	{
		// Chấm ngắm nhỏ + gợi ý
		FCanvasTileItem Cham(FVector2D(W * 0.5f - 2.f, H * 0.5f - 2.f), FVector2D(4.f, 4.f), FLinearColor(1, 1, 1, 0.45f));
		Cham.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Cham);
		if (!DD->GoiY.IsEmpty()) Chu(DD->GoiY, W * 0.5f, H * 0.5f + 46.f * K, 22.f, FLinearColor(0.9f, 0.9f, 0.88f, 0.8f));
	}
	if (DD->TrangThai == ETrangThai::Choi && DD->bDienThoaiReo)
	{
		// Chữ E lớn trong vòng tròn nhấp nháy ngay trên điện thoại (thấy được từ xa)
		const FVector P = Canvas->Project(ADaoDien::ViTriDT() + FVector(0, 0, 35.f));
		if (P.Z > 0.f)
		{
			const float Nhay = 0.55f + 0.45f * FMath::Abs(FMath::Sin(GetWorld()->GetRealTimeSeconds() * 4.f));
			const float R = 64.f * K;
			FCanvasTileItem Nen(FVector2D(P.X - R, P.Y - R), FVector2D(2 * R, 2 * R), FLinearColor(0.f, 0.f, 0.f, 0.55f * Nhay));
			Nen.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(Nen);
			FCanvasBoxItem Vien(FVector2D(P.X - R, P.Y - R), FVector2D(2 * R, 2 * R));
			Vien.LineThickness = 3.f;
			Vien.SetColor(FLinearColor(1.f, 0.9f, 0.85f, Nhay));
			Canvas->DrawItem(Vien);
			Chu(TEXT("E"), P.X, P.Y, 96.f, FLinearColor(1.f, 0.95f, 0.9f, Nhay));
			Chu(TEXT("Answer"), P.X, P.Y + R + 26.f * K, 30.f, FLinearColor(1.f, 0.9f, 0.85f, Nhay));
		}
	}
	if (DD->ThoiPhuDe > 0.f && DD->TrangThai != ETrangThai::Menu)
		Chu(DD->PhuDe, W * 0.5f, H * 0.84f, 28.f, FLinearColor(0.92f, 0.9f, 0.86f, FMath::Clamp(DD->ThoiPhuDe / 0.5f, 0.f, 1.f)));

	if (DD->DoToi > 0.001f) Phu(FLinearColor(0, 0, 0, DD->DoToi));
	if (DD->DoTrang > 0.001f) Phu(FLinearColor(1, 1, 1, DD->DoTrang));

	if (DD->TrangThai == ETrangThai::Menu)
	{
		Phu(FLinearColor(0, 0, 0, 0.55f));
		Chu(TEXT("NIGHT SHIFT"), W * 0.5f, H * 0.28f, 86.f, FLinearColor(0.85f, 0.82f, 0.78f));
		Chu(TEXT("one corridor  ·  one night that never ends"), W * 0.5f, H * 0.36f, 24.f, FLinearColor(0.6f, 0.2f, 0.18f));
		float Y = H * 0.52f;
		VeNut(TEXT("moi"), TEXT("New Game"), Y);
		const int32 Da = ADaoDien::Doc();
		if (Da >= 2 && Da <= ADaoDien::VongCuoiKinhDi + 1) { Y += 64.f * K; VeNut(TEXT("tiep"), TEXT("Continue"), Y); }
		Y += 64.f * K;
		VeNut(TEXT("thoat"), TEXT("Quit"), Y);
		Chu(TEXT("WASD move  ·  Mouse look  ·  Hold Shift run  ·  E open / answer  ·  Hold Left Mouse drag objects  ·  Esc pause"), W * 0.5f, H * 0.9f, 20.f, FLinearColor(0.55f, 0.55f, 0.55f));
		Chu(TEXT("Headphones recommended."), W * 0.5f, H * 0.94f, 18.f, FLinearColor(0.45f, 0.45f, 0.45f));
	}
	else if (DD->TrangThai == ETrangThai::TamDung)
	{
		Phu(FLinearColor(0, 0, 0, 0.6f));
		Chu(TEXT("PAUSED"), W * 0.5f, H * 0.35f, 56.f, FLinearColor(0.85f, 0.82f, 0.78f));
		VeNut(TEXT("tieptuc"), TEXT("Resume"), H * 0.5f);
		VeNut(TEXT("menu"), TEXT("Main Menu"), H * 0.5f + 64.f * K);
	}
	else if (DD->TrangThai == ETrangThai::KetThuc)
	{
		const float T = DD->ThoiKetThuc;
		const FLinearColor Xam(0.2f, 0.2f, 0.22f);
		auto Hien = [&](float Tu) { return FMath::Clamp((T - Tu) / 1.5f, 0.f, 1.f); };
		Chu(TEXT("06:00. Your shift is over."), W * 0.5f, H * 0.38f, 40.f, FLinearColor(Xam.R, Xam.G, Xam.B, Hien(3.5f)), true, false);
		Chu(TEXT("The next morning, they told you room 403 had been empty for three years."), W * 0.5f, H * 0.46f, 26.f, FLinearColor(0.35f, 0.1f, 0.1f, Hien(6.5f)), true, false);
		const float D = Hien(10.f);
		const FLinearColor MC(0.3f, 0.3f, 0.32f, D);
		Chu(TEXT("CREDITS"), W * 0.5f, H * 0.57f, 22.f, MC, true, false);
		Chu(TEXT("\"Ghost Girl Animated\" (https://skfb.ly/o6xVr) by Shaban Hafizsalim is licensed under"), W * 0.5f, H * 0.61f, 17.f, MC, true, false);
		Chu(TEXT("Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/)."), W * 0.5f, H * 0.64f, 17.f, MC, true, false);
		Chu(TEXT("3D props: NVIDIA SimReady Assets (CC BY 4.0)"), W * 0.5f, H * 0.68f, 17.f, MC, true, false);
		Chu(TEXT("Sound effects: made with Kana's Audio Studio MCP (Stable Audio 3)"), W * 0.5f, H * 0.71f, 17.f, MC, true, false);
		Chu(TEXT("Voice: Piper TTS as used in Kana's VoicePi (MIT), LJSpeech voice (public domain)"), W * 0.5f, H * 0.74f, 17.f, MC, true, false);
		Chu(TEXT("Made with Unreal Engine 5.8"), W * 0.5f, H * 0.77f, 17.f, MC, true, false);
		if (T > 12.f) Chu(TEXT("Click to return to the menu"), W * 0.5f, H * 0.88f, 18.f, FLinearColor(0.4f, 0.4f, 0.4f, Hien(12.f)), true, false);
	}
}
