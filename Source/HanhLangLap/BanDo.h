// Kích thước và tọa độ chung của hành lang chữ L (cm, Z hướng lên, sàn ở Z = 0).
//
//   Buồng đầu R0 ─cửa─ đoạn A (đi theo +X, 0 → 1800) ─ góc ─ đoạn B (rẽ phải, đi theo +Y tới 1200) ─cửa─ buồng cuối R1
//   R0 và R1 là hai buồng cầu thang giống hệt nhau: đóng cửa sau lưng ở R1 thì người chơi được dời về R0 (vòng mới).
#pragma once

#include "CoreMinimal.h"

namespace BanDo
{
	constexpr float Cao = 280.f;          // trần
	constexpr float NuaRong = 120.f;      // nửa bề rộng hành lang
	constexpr float DaiA = 1800.f;        // đoạn A: X 0 → 1800 (góc tới 2040)
	constexpr float GocX = 1920.f;        // tâm góc rẽ
	constexpr float CuoiB = 1200.f;       // đoạn B: Y 120 → 1200
	constexpr float NuaBuong = 150.f;

	// Buồng cầu thang: gốc ở tâm, trục X cục bộ = hướng đi (cửa vào ở -X, cửa ra ở +X)
	inline FTransform BuongDau() { return FTransform(FRotator::ZeroRotator, FVector(-150.f, 0.f, 0.f)); }
	inline FTransform BuongCuoi() { return FTransform(FRotator(0.f, 90.f, 0.f), FVector(GocX, CuoiB + 150.f, 0.f)); }
	// Phép dời từ buồng cuối về buồng đầu
	inline FTransform DoiVong() { return BuongCuoi().Inverse() * BuongDau(); }

	// Quãng đường đã đi dọc hành lang (âm = trong buồng đầu)
	inline float QuangDuong(const FVector& P)
	{
		if (P.X > 1800.f && P.Y > 0.f) return GocX + P.Y;
		return FMath::Min(P.X, GocX);
	}
	// Điểm trên đường giữa hành lang ứng với quãng đường S
	inline FVector DiemDuong(float S, float Z = 0.f)
	{
		if (S <= GocX) return FVector(S, 0.f, Z);
		return FVector(GocX, S - GocX, Z);
	}
	inline FVector HuongDuong(float S) { return S <= GocX ? FVector(1, 0, 0) : FVector(0, 1, 0); }
	constexpr float TongDuong = GocX + CuoiB;   // tới cửa buồng cuối

	inline bool TrongBuongCuoi(const FVector& P) { return P.Y > CuoiB + 15.f && FMath::Abs(P.X - GocX) < 150.f; }
	inline bool TrongBuongDau(const FVector& P) { return P.X < -10.f; }
}
