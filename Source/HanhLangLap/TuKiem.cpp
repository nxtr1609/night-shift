#include "TuKiem.h"

#include "BanDo.h"
#include "BongNguoi.h"
#include "Components/PointLightComponent.h"
#include "Cua.h"
#include "DaoDien.h"
#include "DieuKhien.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HanhLang.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NguoiChoi.h"
#include "SinhVat.h"
#include "UnrealClient.h"

using namespace BanDo;

ATuKiem::ATuKiem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

void ATuKiem::Kiem(bool bDat, const FString& MoTa)
{
	++SoPhep;
	if (bDat) ++SoDat;
	KetQua.Add(FString::Printf(TEXT("[%s] %s"), bDat ? TEXT("ĐẠT") : TEXT("SAI"), *MoTa));
	UE_LOG(LogTemp, Display, TEXT("TuKiem %s %s"), bDat ? TEXT("DAT") : TEXT("SAI"), *MoTa);
}

void ATuKiem::Chup(const FString& Ten, bool bGiaoDien)
{
	FScreenshotRequest::RequestScreenshot(FPaths::Combine(ThuMuc, Ten + TEXT(".png")), bGiaoDien, false);
	KhungCho = 3;   // ảnh được lưu ở khung hình kế tiếp
}

void ATuKiem::Xong()
{
	if (bXong) return;
	bXong = true;
	KetQua.Insert(FString::Printf(TEXT("Kết quả: %d/%d phép đạt (%.0f s, trung bình %.0f khung/giây)"), SoDat, SoPhep, ThoiGian,
		TongKhungGiay > 0.f ? SoKhung / TongKhungGiay : 0.f), 0);
	FFileHelper::SaveStringToFile(FString::Join(KetQua, TEXT("\n")), *FPaths::Combine(ThuMuc, TEXT("KetQua.txt")), FFileHelper::EEncodingOptions::ForceUTF8);
	FPlatformMisc::RequestExit(false);
}

void ATuKiem::NhinVe(const FVector& Diem, float Dt, float TocQuay)
{
	ANguoiChoi* N = Cast<ANguoiChoi>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!N || !N->Controller) return;
	const FRotator Dich = (Diem - N->ViTriMat()).Rotation();
	N->Controller->SetControlRotation(FMath::RInterpTo(N->Controller->GetControlRotation(), Dich, Dt, TocQuay));
}

FVector ATuKiem::LanTranh(const FVector& Tu, const FVector& Dich) const
{
	// Làn cố định theo trục giữa hành lang: chọn làn có quãng trống dài nhất, ưu tiên giữa và gần làn đang đi
	const float S = BanDo::QuangDuong(Tu);
	const FVector Huong = BanDo::HuongDuong(S + 120.f);
	const FVector Ngang(-Huong.Y, Huong.X, 0.f);
	const FVector Giua = BanDo::DiemDuong(S + 240.f, Tu.Z);
	const float LechNay = FVector::DotProduct(Tu - BanDo::DiemDuong(S, Tu.Z), Ngang);
	FCollisionQueryParams Q;
	Q.AddIgnoredActor(UGameplayStatics::GetPlayerPawn(this, 0));
	float TotNhat = -1e9f;
	FVector Ket = Dich;
	for (float L : { 0.f, -45.f, 45.f, -80.f, 80.f })
	{
		// Bước 1: lách ngang sang làn L tại chỗ; bước 2: từ đó đi thẳng tới trước
		const FVector Z(0, 0, 15.f);
		const FVector A = Tu + Z;
		const FVector C = Tu + Ngang * (L - LechNay) + Z;
		const FVector B = Giua + Ngang * L + Z;
		const FCollisionShape Nang = FCollisionShape::MakeCapsule(28.f, 55.f);
		FHitResult H;
		float Trong = FVector::Dist(C, B);
		if (FMath::Abs(L - LechNay) > 5.f && GetWorld()->SweepSingleByChannel(H, A, C, FQuat::Identity, ECC_Pawn, Nang, Q) && !H.bStartPenetrating)
			Trong = 0.f;
		else if (GetWorld()->SweepSingleByChannel(H, C, B, FQuat::Identity, ECC_Pawn, Nang, Q))
			Trong = H.bStartPenetrating ? 0.f : H.Distance;
		const float Diem = Trong - FMath::Abs(L) * 0.2f - FMath::Abs(L - LechNay) * 0.25f;
		if (Diem > TotNhat) { TotNhat = Diem; Ket = B - FVector(0, 0, 15.f); }
	}
	return Ket;
}

void ATuKiem::Tick(float Dt)
{
	Super::Tick(Dt);
	ThoiGian += Dt;
	if (bXong) return;
	if (KhungCho > 0 && CheDo != TEXT("trailer")) { --KhungCho; return; }
	ADaoDien* DD = ADaoDien::Lay(this);
	ADieuKhien* PC = Cast<ADieuKhien>(UGameplayStatics::GetPlayerController(this, 0));
	if (!DD || !PC || !DD->HL) return;
	if (!bBatDau)
	{
		if (ThoiGian < 2.f) return;   // chờ shader / nạp
		bBatDau = true;
		PC->bMayChoi = true;
		DD->BatDau(FMath::Max(VongDau, 1));
		VongTruoc = DD->Vong;
		Kiem(DD->TrangThai == ETrangThai::Choi, TEXT("Vào game từ menu"));
		Kiem(DD->HL->Den.Num() >= 13, FString::Printf(TEXT("Dựng đủ đèn (%d)"), DD->HL->Den.Num()));
		int32 Thieu = 0;
		for (auto& KV : DD->HL->VatDung)
			if (UStaticMeshComponent* C = Cast<UStaticMeshComponent>(KV.Value.C)) if (!C->GetStaticMesh()) { ++Thieu; KetQua.Add(TEXT("  thiếu mẫu: ") + KV.Key.ToString()); }
		Kiem(Thieu == 0, FString::Printf(TEXT("Mọi đồ vật có mẫu 3D (%d thiếu)"), Thieu));
		// Máy chơi kiểm vòng lặp / sự kiện, không kiểm tìm đường: cho đi xuyên đồ vật
		if (CheDo != TEXT("day") && CheDo != TEXT("duong") && CheDo != TEXT("keo") && CheDo != TEXT("nhen") && CheDo != TEXT("nhinlai") && CheDo != TEXT("trailer"))
			for (auto& KV : DD->HL->VatDung)
				if (UPrimitiveComponent* C = Cast<UPrimitiveComponent>(KV.Value.C)) C->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		return;
	}
	if (ThoiGian > 3.f) { ++SoKhung; TongKhungGiay += Dt; }
	if (CheDo == TEXT("menu"))
	{
		// Chụp menu chính (máy không bấm gì)
		if (DD->TrangThai != ETrangThai::Menu) { DD->VeMenu(); ThoiVong = 0.f; return; }
		if ((ThoiVong += Dt) < 2.f) return;
		Chup(TEXT("menu"));
		Kiem(true, TEXT("Chụp menu chính"));
		bXong = false;
		CheDo = TEXT("xong");
		return;
	}
	if (CheDo == TEXT("xong")) { Xong(); return; }
	if (CheDo == TEXT("nhinlai"))
	{
		// Vòng 9: quay lại nhìn 3 lần (mỗi lần giữ 0,5 s rồi quay lên trước) → lần thứ 3 phải bị bắt
		ANguoiChoi* N = Cast<ANguoiChoi>(UGameplayStatics::GetPlayerPawn(this, 0));
		if (ThoiVong == 0.f)
		{
			DD->BatDau(9);
			N->SetActorLocation(FVector(700.f, 0.f, 92.f), false, nullptr, ETeleportType::TeleportPhysics);
		}
		ThoiVong += Dt;
		const float Chu = FMath::Fmod(ThoiVong, 1.6f);
		const int32 Lan = FMath::FloorToInt(ThoiVong / 1.6f);
		if (Lan < 3 && DD->SoLanBiBat == 0) N->Controller->SetControlRotation(FRotator(0.f, Chu > 0.8f ? 180.f : 0.f, 0.f));
		if (Lan == 0 && Chu > 1.3f && !bDaChupA) { bDaChupA = true; Kiem(DD->SoLanNhinLai == 1, FString::Printf(TEXT("Nhìn lại lần 1: nó hiện ra (đếm %d)"), DD->SoLanNhinLai)); }
		if (Lan == 1 && Chu > 1.3f && !bDaChupB) { bDaChupB = true; Kiem(DD->SoLanNhinLai == 2, FString::Printf(TEXT("Nhìn lại lần 2 (đếm %d)"), DD->SoLanNhinLai)); }
		if (ThoiVong > 6.5f)
		{
			Kiem(DD->SoLanBiBat == 1, FString::Printf(TEXT("Nhìn lại lần 3 thì bị bắt (bị bắt %d lần)"), DD->SoLanBiBat));
			CheDo = TEXT("xong");
		}
		return;
	}
	if (CheDo == TEXT("nhen"))
	{
		// Sinh vật 8 chân: từ sàn → tường → trần; kiểm các mặt đã bò qua, chụp ảnh trên tường và trên trần
		ANguoiChoi* N = Cast<ANguoiChoi>(UGameplayStatics::GetPlayerPawn(this, 0));
		if (ThoiVong == 0.f)
		{
			DD->BatDau(1);
			N->SetActorLocation(FVector(250.f, -40.f, 92.f), false, nullptr, ETeleportType::TeleportPhysics);
			TArray<FVector> Dichs = { FVector(560.f, NuaRong - 2.f, 210.f), FVector(420.f, 0.f, Cao - 2.f), FVector(120.f, 0.f, Cao - 2.f) };
			DD->Nhen->Bat(FVector(640.f, NuaRong - 60.f, 1.f), FVector::UpVector, FVector(0.f, 1.f, 0.f), Dichs, 160.f);
		}
		ThoiVong += Dt;
		N->Controller->SetControlRotation((DD->Nhen->Neo + FVector(0, 0, 20.f) - N->ViTriMat()).Rotation());
		const FVector PT = DD->Nhen->PhapNeo;
		if (!bDaChupA && PT.Y < -0.8f && ThoiVong > 0.4f) { bDaChupA = true; Chup(TEXT("nhen_tren_tuong")); return; }
		if (!bDaChupB && PT.Z < -0.8f) { bDaChupB = true; ChoDung = ThoiVong; }
		if (bDaChupB && ChoDung > 0.f && ThoiVong > ChoDung + 0.6f) { ChoDung = -1.f; Chup(TEXT("nhen_tren_tran")); return; }
		if (!DD->Nhen->DangHien() || ThoiVong > 14.f)
		{
			bool bTuong = false, bTran = false;
			for (const FVector& M : DD->Nhen->MatDaQua) { if (M.Y < -0.8f) bTuong = true; if (M.Z < -0.8f) bTran = true; }
			Kiem(bTuong, TEXT("Sinh vật từ sàn bò lên được tường"));
			Kiem(bTran, TEXT("Sinh vật từ tường bò lên được trần"));
			Kiem(!DD->Nhen->DangHien(), FString::Printf(TEXT("Sinh vật bò hết các đích rồi biến mất (%.1f s, %d lần đổi mặt)"), ThoiVong, DD->Nhen->MatDaQua.Num() - 1));
			CheDo = TEXT("xong");
		}
		return;
	}
	if (CheDo == TEXT("keo"))
	{
		// Nắm ghế bằng chuột trái rồi quay người 70°: ghế phải đi theo
		ANguoiChoi* N = Cast<ANguoiChoi>(UGameplayStatics::GetPlayerPawn(this, 0));
		USceneComponent* Ghe = DD->HL->Vat(TEXT("Ghe2"));
		if (ThoiVong == 0.f)
		{
			DD->BatDau(1);
			N->SetActorLocation(FVector(720.f, 30.f, 92.f), false, nullptr, ETeleportType::TeleportPhysics);
			N->Controller->SetControlRotation((Ghe->GetComponentLocation() - N->ViTriMat()).Rotation());
			ViTriXeLan = Ghe->GetComponentLocation();
		}
		ThoiVong += Dt;
		if (ThoiVong < 0.4f) return;
		if (!bDaNhinLai) { bDaNhinLai = true; DD->NamVat(true); Kiem(N->DangNam(), TEXT("Chuột trái nắm được ghế trước mặt")); return; }
		if (ThoiVong < 2.4f)
		{
			FRotator R = N->Controller->GetControlRotation();
			R.Yaw += Dt * 35.f;
			N->Controller->SetControlRotation(R);
			return;
		}
		const float Doi = FVector::Dist2D(Ghe->GetComponentLocation(), ViTriXeLan);
		Kiem(Doi > 50.f, FString::Printf(TEXT("Ghế đi theo khi kéo (dời %.0f cm)"), Doi));
		Chup(TEXT("keo_ghe"));
		DD->NamVat(false);
		Kiem(!N->DangNam(), TEXT("Nhả chuột thì thả ghế"));
		CheDo = TEXT("xong");
		return;
	}
	if (CheDo == TEXT("day"))
	{
		ANguoiChoi* N = Cast<ANguoiChoi>(UGameplayStatics::GetPlayerPawn(this, 0));
		USceneComponent* Xe = DD->HL->Vat(TEXT("XeLan"));
		if (!N || !Xe) { Kiem(false, TEXT("Thiếu xe lăn")); Xong(); return; }
		if (ThoiVong == 0.f)
		{
			DD->BatDau(4);
			N->SetActorLocation(FVector(700.f, 0.f, 92.f), false, nullptr, ETeleportType::TeleportPhysics);
			N->Controller->SetControlRotation(FRotator::ZeroRotator);
			ViTriXeLan = Xe->GetComponentLocation();
		}
		ThoiVong += Dt;
		if (ThoiVong < 0.5f) return;
		N->Controller->SetControlRotation(FRotator::ZeroRotator);
		N->DiChuyen(FVector2D(0.f, 1.f), false);
		if (ThoiVong < 4.5f) return;
		const float Doi = FVector::Dist2D(Xe->GetComponentLocation(), ViTriXeLan);
		Kiem(Doi > 40.f, FString::Printf(TEXT("Đi vào xe lăn chắn giữa hành lang thì đẩy được nó (xe dời %.0f cm, người tới X=%.0f)"), Doi, N->GetActorLocation().X));
		Kiem(N->GetActorLocation().X > 950.f, TEXT("Người chơi đi qua được chỗ xe lăn"));
		if (UPrimitiveComponent* PX = Cast<UPrimitiveComponent>(Xe))
			KetQua.Add(FString::Printf(TEXT("  xe: mô phỏng=%d khối=%.1f kg va chạm=%d lớp=%s cha=%s"), PX->IsSimulatingPhysics(), PX->GetMass(), (int32)PX->GetCollisionEnabled(), *PX->GetCollisionProfileName().ToString(), PX->GetAttachParent() ? *PX->GetAttachParent()->GetName() : TEXT("-")));
		Chup(TEXT("day_xe_lan"));
		CheDo = TEXT("xong");
		return;
	}
	if (CheDo == TEXT("hu")) { TickHu(Dt); return; }
	if (CheDo == TEXT("duong")) { TickDuong(Dt); return; }
	if (CheDo == TEXT("trailer")) { TickTrailer(Dt); return; }
	if (CheDo == TEXT("anh")) TickAnh(Dt);
	else TickChoi(Dt);
}

void ATuKiem::TickAnh(float Dt)
{
	// Mỗi vòng: áp trạng thái vòng rồi chụp 3 góc cố định (đầu A, góc rẽ nhìn B, giữa A nhìn ngược lại)
	ADaoDien* DD = ADaoDien::Lay(this);
	ANguoiChoi* N = Cast<ANguoiChoi>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!N) return;
	struct FGoc { FVector P; FRotator R; const TCHAR* Ten; };
	static const FGoc Goc[] = {
		{ FVector(60.f, 0.f, 92.f), FRotator(-2.f, 0.f, 0.f), TEXT("a") },
		{ FVector(GocX - 70.f, -70.f, 92.f), FRotator(-2.f, 75.f, 0.f), TEXT("b") },
		{ FVector(1000.f, 40.f, 92.f), FRotator(-4.f, 180.f, 0.f), TEXT("c") },
		{ FVector(1300.f, 0.f, 92.f), FRotator(5.f, 20.f, 0.f), TEXT("d") },
		{ FVector(470.f, 0.f, 92.f), FRotator(8.f, 0.f, 0.f), TEXT("e") },   // cận mặt người đeo mặt nạ
	};
	const int32 SoGoc = UE_ARRAY_COUNT(Goc);
	const int32 V = BuocAnh / (SoGoc + 1) + 1;
	const int32 G = BuocAnh % (SoGoc + 1);
	if (V > ADaoDien::VongCuoiKinhDi + 1) { Kiem(true, TEXT("Chụp đủ ảnh các vòng")); Xong(); return; }
	if (G == 0)
	{
		DD->BatDau(V);
		DD->DoToi = 0.f;
		ChoDung = 1.2f;   // chờ đèn / mưa ổn định
		++BuocAnh;
		return;
	}
	if ((ChoDung -= Dt) > 0.f)
	{
		N->SetActorLocation(Goc[G - 1].P, false, nullptr, ETeleportType::TeleportPhysics);
		N->Controller->SetControlRotation(Goc[G - 1].R);
		if (G == SoGoc && DD->Bong->CheDo != EBong::Dung) DD->Bong->Dung(FVector(650.f, 0.f, 0.f), 180.f, false);
		return;
	}
	Chup(FString::Printf(TEXT("vong%02d_%s"), V, Goc[G - 1].Ten));
	ChoDung = 0.25f;
	++BuocAnh;
}

bool ATuKiem::KiemLoiDi(FString& NoiChan) const
{
	ANguoiChoi* N = Cast<ANguoiChoi>(UGameplayStatics::GetPlayerPawn(this, 0));
	FCollisionQueryParams Q;
	Q.AddIgnoredActor(N);
	ADaoDien* DD = ADaoDien::Lay(this);
	if (DD && DD->Bong) Q.AddIgnoredActor(DD->Bong);
	const FCollisionShape Nang = FCollisionShape::MakeCapsule(30.5f, 80.f);   // đúng cỡ người chơi (bán kính 30)
	TArray<float> Truoc;
	for (float S = 50.f; S <= TongDuong - 45.f; S += 15.f)
	{
		const FVector Tam = DiemDuong(S, 95.f);
		const FVector H = HuongDuong(S);
		const FVector Ngang(-H.Y, H.X, 0.f);
		TArray<float> Nay;
		for (float L = -95.f; L <= 95.f; L += 5.f)
		{
			if (GetWorld()->OverlapBlockingTestByChannel(Tam + Ngang * L, FQuat::Identity, ECC_Pawn, Nang, Q)) continue;
			// nối được với làn của đoạn trước (đoạn đầu, hoặc ngay chỗ rẽ thì nối tự do)
			bool bNoi = Truoc.Num() == 0 && S < 60.f;
			const bool bRe = FMath::Abs(S - GocX) < 20.f;
			for (float T : Truoc) if (bRe || FMath::Abs(T - L) <= 20.f) { bNoi = true; break; }
			if (bNoi) Nay.Add(L);
		}
		if (Nay.Num() == 0)
		{
			TSet<FString> Ten;
			for (float L = -95.f; L <= 95.f; L += 5.f)
			{
				TArray<FOverlapResult> O;
				GetWorld()->OverlapMultiByChannel(O, Tam + Ngang * L, FQuat::Identity, ECC_Pawn, Nang, Q);
				for (const FOverlapResult& R : O)
					if (R.GetComponent() && !R.GetComponent()->GetName().StartsWith(TEXT("Luoi"))) Ten.Add(R.GetComponent()->GetName());
			}
			NoiChan = FString::Printf(TEXT("quãng %.0f (X=%.0f Y=%.0f) bởi %s"), S, Tam.X, Tam.Y, *FString::Join(Ten.Array(), TEXT(", ")));
			return false;
		}
		Truoc = Nay;
	}
	return true;
}

void ATuKiem::TickDuong(float Dt)
{
	// Mỗi vòng: áp trạng thái (kể cả đồ đã dời, cáng, xe lăn giữa hành lang...) rồi dò lối đi
	ADaoDien* DD = ADaoDien::Lay(this);
	ANguoiChoi* N = Cast<ANguoiChoi>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!N) return;
	const int32 V = BuocAnh / 2 + 1;
	if (V > ADaoDien::VongCuoiKinhDi + 1) { Xong(); return; }
	if (BuocAnh % 2 == 0)
	{
		DD->BatDau(V);
		N->SetActorLocation(FVector(-230.f, 0.f, 92.f), false, nullptr, ETeleportType::TeleportPhysics);   // đứng trong buồng đầu, khỏi vướng
		// hiện mọi thứ có thể xuất hiện giữa đường trong vòng này (cáng vòng 7...)
		ChoDung = 0.6f;
		++BuocAnh;
		return;
	}
	if ((ChoDung -= Dt) > 0.f) return;
	FString Cho;
	const bool bThong = KiemLoiDi(Cho);
	Kiem(bThong, bThong ? FString::Printf(TEXT("Vòng %d: hành lang luôn có lối đi"), V) : FString::Printf(TEXT("Vòng %d: bị chặn ở %s"), V, *Cho));
	++BuocAnh;
}

void ATuKiem::TickTrailer(float Dt)
{
	ADaoDien* DD = ADaoDien::Lay(this);
	ANguoiChoi* N = Cast<ANguoiChoi>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!N) return;
	// Đặt camera: P = vị trí chân (Z 92 → mắt ~166 cm), nhìn về Nhin
	auto Dat = [N](const FVector& P, const FVector& Nhin)
	{
		N->SetActorLocation(FVector(P.X, P.Y, 92.f), false, nullptr, ETeleportType::TeleportPhysics);
		N->Controller->SetControlRotation((Nhin - N->ViTriMat()).Rotation());
	};
	auto Muot = [](float T) { T = FMath::Clamp(T, 0.f, 1.f); return T * T * (3.f - 2.f * T); };
	auto Mot = [](float T, float Moc, float Dt0) { return T >= Moc && T - Dt0 < Moc; };   // đúng một lần khi vừa qua mốc
	struct FCanh { const TCHAR* Ten; float Dai; int32 Vong; TFunction<void(ADaoDien*, float, float)> F; };
	static TArray<FCanh> Canh;
	if (Canh.Num() == 0)
	{
		Canh = {
			{ TEXT("s01_hanh_lang"), 5.5f, 1, [=](ADaoDien* D, float T, float Dt0) { Dat(FMath::Lerp(FVector(60, 0, 0), FVector(520, 0, 0), Muot(T / 5.5f)), FVector(1400, 0, 150)); } },
			{ TEXT("s02_dong_ho"), 4.f, 1, [=](ADaoDien* D, float T, float Dt0) { Dat(FMath::Lerp(FVector(1640, -20, 0), FVector(1730, -10, 0), Muot(T / 4.f)), FMath::Lerp(FVector(2038, 60, 195), FVector(2038, -60, 215), Muot(T / 4.f))); } },
			{ TEXT("s03_cua_mo"), 4.5f, 3, [=](ADaoDien* D, float T, float Dt0)
				{
					if (Mot(T, 0.6f, Dt0)) D->HL->Cua[TEXT("Dau")]->Mo();
					Dat(FMath::Lerp(FVector(-240, 0, 0), FVector(170, 0, 0), Muot((T - 1.2f) / 3.3f)), FVector(1200, 0, 140));
				} },
			{ TEXT("s04_dien_thoai"), 4.f, 5, [=](ADaoDien* D, float T, float Dt0) { Dat(FMath::Lerp(FVector(1450, 10, 0), FVector(1780, 0, 0), Muot(T / 4.f)), FVector(2010, -85, 115)); } },
			{ TEXT("s05_mua_set"), 3.5f, 6, [=](ADaoDien* D, float T, float Dt0)
				{
					// người đứng trên gờ ngoài cửa sổ, chỉ thấy rõ khi sét lóe
					D->bBongNgoaiCuaSo = false;
					if (T < Dt0) D->Bong->Dung(FVector(1100.f, NuaRong + 70.f, 0.f), -90.f);
					if (Mot(T, 1.1f, Dt0) || Mot(T, 2.3f, Dt0)) D->HL->ChopSet(1.3f);
					D->Bong->AnhChieu->SetRelativeLocation(FVector(70.f, 0.f, 150.f));
					D->Bong->AnhChieu->SetIntensity(40.f + D->HL->Set * 3500.f);
					Dat(FMath::Lerp(FVector(860, 30, 0), FVector(900, 40, 0), Muot(T / 3.5f)), FVector(1100, 190, 150));
				} },
			{ TEXT("s06_tay_kinh"), 3.5f, 7, [=](ADaoDien* D, float T, float Dt0) { Dat(FMath::Lerp(FVector(380, -30, 0), FVector(470, -30, 0), Muot(T / 3.5f)), FVector(700, 125, 160)); } },
			{ TEXT("s07_co_giat"), 3.f, 8, [=](ADaoDien* D, float T, float Dt0)
				{
					// đứng co giật giữa đoạn B rồi biến mất
					D->bCoGiatBenKia = false; D->bDoaSet = false;
					if (T < Dt0) D->Bong->CoGiat(FVector(GocX + 10.f, 760.f, 0.f), -90.f);
					if (Mot(T, 2.2f, Dt0)) { D->TatDen(0.25f); D->Bong->An(); }
					Dat(FMath::Lerp(FVector(GocX - 20, 230, 0), FVector(GocX - 20, 300, 0), Muot(T / 3.f)), FVector(GocX + 10, 760, 130));
				} },
			{ TEXT("s08_sam_bong"), 3.f, 8, [=](ADaoDien* D, float T, float Dt0)
				{
					// sét lóe: nó đứng cuối hành lang, mất điện chớp nhoáng rồi có điện lại
					D->bCoGiatBenKia = false; D->bDoaSet = false;
					if (Mot(T, 1.0f, Dt0)) { D->Bong->Dung(FVector(GocX, 1110.f, 0.f), -90.f); D->HL->ChopSet(1.3f); }
					if (Mot(T, 1.35f, Dt0)) { D->TatDen(0.35f); D->Bong->An(); }
					Dat(FMath::Lerp(FVector(GocX, 110, 0), FVector(GocX, 240, 0), Muot(T / 3.f)), FVector(GocX, 1110, 150));
				} },
			{ TEXT("s09_nhen"), 6.f, 10, [=](ADaoDien* D, float T, float Dt0)
				{
					D->bLuatNhinLai = false;
					static FVector Nhin = FVector(700, 120, 120);
					if (T < Dt0) Nhin = FVector(700, 120, 120);
					if (D->Nhen->DangHien()) Nhin = FMath::VInterpTo(Nhin, D->Nhen->Neo + D->Nhen->PhapNeo * 30.f, Dt0, 4.f);
					Dat(FVector(300, -40, 0), Nhin);
				} },
			{ TEXT("s10_tien_lai"), 4.5f, 11, [=](ADaoDien* D, float T, float Dt0) { Dat(FVector(900, 0, 0), FVector(1900, 0, 150)); } },
			{ TEXT("s11_nhin_lai"), 4.f, 9, [=](ADaoDien* D, float T, float Dt0)
				{
					auto Quay = [&](float A, float B, float Tu) { return FMath::Lerp(A, B, Muot((T - Tu) / 0.25f)); };
					float Yaw = 0.f;
					if (T >= 1.0f) Yaw = Quay(0.f, 180.f, 1.0f);
					if (T >= 1.8f) Yaw = Quay(180.f, 0.f, 1.8f);
					if (T >= 2.5f) Yaw = Quay(0.f, 180.f, 2.5f);
					N->SetActorLocation(FVector(1000, 0, 92), false, nullptr, ETeleportType::TeleportPhysics);
					N->Controller->SetControlRotation(FRotator(-3.f, Yaw, 0.f));
				} },
			{ TEXT("s12_duoi"), 4.2f, 12, [=](ADaoDien* D, float T, float Dt0) { if (!N->bKhoa) Dat(FVector(1890, 0, 0), FVector(300, 0, 150)); } },
		};
	}
	if (BuocAnh >= Canh.Num())
	{
		Kiem(true, FString::Printf(TEXT("Quay xong %d cảnh trailer"), Canh.Num()));
		Xong();
		return;
	}
	FString Chon;
	if (FParse::Value(FCommandLine::Get(), TEXT("canh="), Chon) && !Chon.Contains(Canh[BuocAnh].Ten)) { ++BuocAnh; return; }
	const FCanh& C = Canh[BuocAnh];
	const float Buoc = 1.f / 30.f;
	if (KhungTrailer == 0)
	{
		DD->bTrailer = true;
		DD->BatDau(C.Vong);
		DD->DoToi = 0.f;
		ThoiCanh = 0.f;
		C.F(DD, 0.f, Buoc);
	}
	++KhungTrailer;
	// 20 khung chạy trước (nạp texture, đèn ổn định), chưa tính giờ cảnh, chưa chụp
	if (KhungTrailer <= 20) { C.F(DD, 0.f, Buoc); return; }
	C.F(DD, ThoiCanh, Buoc);
	FScreenshotRequest::RequestScreenshot(FPaths::Combine(ThuMuc, C.Ten, FString::Printf(TEXT("f%04d.png"), KhungTrailer - 21)), true, false);
	ThoiCanh += Buoc;
	if (ThoiCanh >= C.Dai)
	{
		KetQua.Add(FString::Printf(TEXT("  %s: %d khung"), C.Ten, KhungTrailer - 20));
		++BuocAnh;
		KhungTrailer = 0;
	}
}

void ATuKiem::TickHu(float Dt)
{
	// Dàn dựng từng cú hù ở đúng khoảnh khắc rồi chụp (để xem chúng trông ra sao)
	ADaoDien* DD = ADaoDien::Lay(this);
	ANguoiChoi* N = Cast<ANguoiChoi>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!N) return;
	struct FCanh { int32 Vong; FVector P; FVector Nhin; float Cho; const TCHAR* Ten; TFunction<void(ADaoDien*)> Lam; };
	static const TArray<FCanh> Canh = {
		{ 6, FVector(640.f, 40.f, 92.f), FVector(720.f, 130.f, 170.f), 1.0f, TEXT("kinh_mua"), nullptr },
		{ 6, FVector(700.f, 50.f, 92.f), FVector(700.f, 130.f, 165.f), 1.0f, TEXT("kinh_gan"), nullptr },
		{ 5, FVector(1650.f, 10.f, 92.f), FVector(2010.f, -85.f, 115.f), 0.9f, TEXT("ui_dien_thoai"), [](ADaoDien* D) { D->bDienThoaiReo = true; } },
		{ 7, FVector(1020.f, -20.f, 92.f), FVector(1100.f, 130.f, 160.f), 1.6f, TEXT("tay_tren_kinh"), [](ADaoDien* D) { for (int32 K = 0; K < D->HL->SoTayKinh; ++K) D->HL->Hien(FName(*FString::Printf(TEXT("TayKinh%d"), K)), true); } },
		{ 8, FVector(1000.f, 40.f, 92.f), FVector(1885.f, 745.f, 140.f), 1.2f, TEXT("co_giat_ben_kia"), nullptr },
		{ 8, FVector(GocX, 250.f, 92.f), FVector(GocX, 1110.f, 150.f), 0.05f, TEXT("sam_bong_nguoi"), [](ADaoDien* D) { D->Bong->Dung(FVector(GocX, 1110.f, 0.f), -90.f); D->HL->ChopSet(1.3f); } },
		{ 9, FVector(1020.f, -20.f, 92.f), FVector(1100.f, 130.f, 160.f), 1.0f, TEXT("tay_mau_tren_kinh"), nullptr },
		{ 10, FVector(420.f, 0.f, 92.f), FVector(700.f, 130.f, 160.f), 0.75f, TEXT("bo_len_kinh"), [](ADaoDien* D) { D->Bong->Bo(FVector(700.f, NuaRong + 32.f, -170.f), FVector(740.f, NuaRong + 32.f, 330.f), 260.f, -90.f); D->HL->ChopSet(1.f); } },
		{ 4, FVector(1200.f, 0.f, 92.f), FVector(500.f, 0.f, 150.f), 0.05f, TEXT("liec_sau_lung"), [](ADaoDien* D) { D->Bong->Dung(FVector(560.f, 0.f, 0.f), 0.f); } },
		{ 1, FVector(300.f, 0.f, 92.f), FVector(700.f, 0.f, 120.f), 1.3f, TEXT("ma_di"), [](ADaoDien* D) { D->Bong->Di(FVector(900.f, 0.f, 0.f), FVector(350.f, 0.f, 0.f), 120.f, false); } },
		{ 1, FVector(300.f, 0.f, 92.f), FVector(560.f, 0.f, 120.f), 1.0f, TEXT("ma_dung"), [](ADaoDien* D) { D->Bong->Dung(FVector(560.f, 0.f, 0.f), 180.f, false); } },
	};
	if (BuocAnh >= Canh.Num()) { Kiem(true, TEXT("Chụp đủ các cú hù")); Xong(); return; }
	const FCanh& C = Canh[BuocAnh];
	if (ChoDung <= 0.f && AnhCho != C.Ten)
	{
		DD->BatDau(C.Vong);
		DD->DoToi = 0.f;
		N->SetActorLocation(C.P, false, nullptr, ETeleportType::TeleportPhysics);
		N->Controller->SetControlRotation((C.Nhin - N->ViTriMat()).Rotation());
		if (C.Lam) C.Lam(DD);
		AnhCho = C.Ten;
		ChoDung = C.Cho;
		return;
	}
	N->Controller->SetControlRotation((C.Nhin - N->ViTriMat()).Rotation());
	if ((ChoDung -= Dt) > 0.f) return;
	Chup(C.Ten, FString(C.Ten).StartsWith(TEXT("ui_")));
	++BuocAnh;
	ChoDung = 0.f;
	AnhCho.Reset();
}

void ATuKiem::TickChoi(float Dt)
{
	ADaoDien* DD = ADaoDien::Lay(this);
	ANguoiChoi* N = Cast<ANguoiChoi>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!N) return;
	AHanhLang* HL = DD->HL;
	const FVector P = N->GetActorLocation();
	ThoiVong += Dt;

	if (DD->TrangThai == ETrangThai::KetThuc)
	{
		if (!bDaKiemKetThuc)
		{
			bDaKiemKetThuc = true;
			Kiem(true, FString::Printf(TEXT("Tới màn kết thúc sau %.0f s"), ThoiGian));
		}
		if (DD->ThoiKetThuc < 11.5f) return;
		Chup(TEXT("ket_thuc"));
		Xong();
		return;
	}

	// Sang vòng mới: kiểm tra dời người chơi liền mạch
	if (DD->Vong != VongTruoc)
	{
		const FVector TuongDoiSau = BuongDau().InverseTransformPosition(P);
		Kiem(DD->Vong == VongTruoc + 1 && FVector::Dist2D(TuongDoiSau, TuongDoiTruoc) < 30.f,
			FString::Printf(TEXT("Vòng %d → %d sau %.0f s, dời liền mạch (lệch %.1f cm)"), VongTruoc, DD->Vong, ThoiVong, FVector::Dist2D(TuongDoiSau, TuongDoiTruoc)));
		if (VongTruoc == 3 || VongTruoc == 7 || VongTruoc == 8 || VongTruoc == 10)
			Kiem(CuHuVong >= (VongTruoc == 8 ? 2 : 1), FString::Printf(TEXT("Vòng %d: cú hù xảy ra (%d)"), VongTruoc, CuHuVong));
		CuHuVong = 0;
		if (VongTruoc == 10) Kiem(bThayBongSauLung && bBongBienMat, TEXT("Vòng 10: quay lại nhìn thì nó hiện ra trong tích tắc rồi biến mất (luật \"đừng nhìn lại\")"));
		if (VongTruoc == ADaoDien::VongCuoiKinhDi)
		{
			if (FParse::Param(FCommandLine::Get(), TEXT("thu_bi_bat"))) Kiem(DD->SoLanBiBat == 1, FString::Printf(TEXT("Vòng 12: đi bộ thì bị bắt, làm lại vòng rồi chạy thoát (bị bắt %d lần)"), DD->SoLanBiBat));
			else Kiem(bThayDuoi && DD->SoLanBiBat == 0, FString::Printf(TEXT("Vòng 12: bị rượt và chạy thoát (bị bắt %d lần)"), DD->SoLanBiBat));
		}
		VongTruoc = DD->Vong;
		ThoiVong = 0.f;
		bDaChupA = bDaChupB = bDaNhinLai = bDaNgheMay = false;
		KhongTien = 0.f;
		QuangTruoc = -1e9f;
	}
	if (BanDo::TrongBuongCuoi(P)) TuongDoiTruoc = BuongCuoi().InverseTransformPosition(P);
	CuHuVong = FMath::Max(CuHuVong, DD->SoCuHu);
	if (DD->bDangDuoi) bThayDuoi = true;
	if (DD->Vong == 10 && DD->Bong->DangHien() && bDaNhinLai) bThayBongSauLung = true;
	if (DD->Vong == 10 && bDaNhinLai && bThayBongSauLung && !DD->Bong->DangHien()) bBongBienMat = true;

	// Kẹt quá 60 s một vòng là hỏng
	if (ThoiVong > 75.f)
	{
		Kiem(false, FString::Printf(TEXT("Kẹt ở vòng %d (quãng đường %.0f, vị trí %s)"), DD->Vong, DD->QuangDuong, *P.ToString()));
		Chup(TEXT("ket"));
		Xong();
		return;
	}
	if (ThoiGian > 1500.f) { Kiem(false, TEXT("Quá giờ")); Xong(); return; }

	// Đang dừng chụp ảnh
	if (ChoDung > 0.f)
	{
		ChoDung -= Dt;
		N->DiChuyen(FVector2D::ZeroVector, false);
		if (ChoDung <= 0.f) Chup(AnhCho);
		return;
	}

	const float S = DD->QuangDuong;
	const bool bThuBiBat = FParse::Param(FCommandLine::Get(), TEXT("thu_bi_bat"));
	const bool bChay = DD->Vong >= ADaoDien::VongCuoiKinhDi && S > GocX - 300.f && (!bThuBiBat || DD->SoLanBiBat > 0);
	ACua* Dau = HL->Cua[TEXT("Dau")];
	ACua* Cuoi = HL->Cua[TEXT("Cuoi")];
	FVector Dich;
	ACua* CanMo = nullptr;

	if (BanDo::TrongBuongDau(P))
	{
		Dich = FVector(60.f, 0.f, P.Z);
		if (Dau->DaDong()) CanMo = Dau;
	}
	else if (BanDo::TrongBuongCuoi(P))
	{
		Dich = FVector(GocX, CuoiB + 200.f, P.Z);
	}
	else if (DD->bDienThoaiReo && !bDaNgheMay && S > 1500.f)
	{
		// Đi nghe điện thoại ở quầy trực
		Dich = FVector(1930.f, -60.f, P.Z);
		if (FVector::Dist2D(P, Dich) < 60.f)
		{
			NhinVe(FVector(2010.f, -85.f, 110.f), Dt, 20.f);
			if (FVector::DotProduct(N->HuongNhin(), (FVector(2010.f, -85.f, 110.f) - N->ViTriMat()).GetSafeNormal()) > 0.95f)
			{
				DD->TuongTac();
				bDaNgheMay = true;
				Kiem(!DD->bDienThoaiReo, TEXT("Nghe điện thoại ở vòng 5"));
			}
			N->DiChuyen(FVector2D::ZeroVector, false);
			return;
		}
	}
	else if (S < GocX - 60.f)
	{
		Dich = FVector(GocX, 0.f, P.Z);
		if (!bDaChupA && S > 40.f) { bDaChupA = true; AnhCho = FString::Printf(TEXT("choi_v%02d_a"), DD->Vong); ChoDung = 0.4f; N->Controller->SetControlRotation(FRotator(-2.f, 0.f, 0.f)); return; }
		// Vòng 10: quay lại nhìn sau lưng
		if (DD->Vong == 10 && !bDaNhinLai && S > 1150.f)
		{
			NhinVe(P - FVector(500.f, 0.f, -60.f), Dt, 10.f);
			N->DiChuyen(FVector2D::ZeroVector, false);
			if (N->HuongNhin().X < -0.9f)
			{
				bDaNhinLai = true;
				AnhCho = TEXT("choi_v10_nhin_lai");
				ChoDung = 0.45f;
			}
			return;
		}
	}
	else
	{
		Dich = FVector(GocX, CuoiB - 20.f, P.Z);
		if (!bDaChupB && S > GocX + 120.f) { bDaChupB = true; AnhCho = FString::Printf(TEXT("choi_v%02d_b"), DD->Vong); ChoDung = DD->Vong >= ADaoDien::VongCuoiKinhDi ? 0.f : 0.4f; N->Controller->SetControlRotation(FRotator(-2.f, 90.f, 0.f)); if (ChoDung <= 0.f) Chup(AnhCho); return; }
		if (FVector::Dist2D(P, Cuoi->GetActorLocation()) < 260.f)
		{
			Dich = FVector(GocX, CuoiB + 200.f, P.Z);
			if (Cuoi->DaDong()) CanMo = Cuoi;
		}
	}

	// Mở cửa: nhìn vào cánh cửa rồi bấm E
	if (CanMo && FVector::Dist2D(P, CanMo->GetActorLocation()) < 170.f)
	{
		const FVector Tam = CanMo->GetActorLocation() + FVector(0, 0, 110.f);
		NhinVe(Tam, Dt, 20.f);
		N->DiChuyen(FVector2D::ZeroVector, false);
		if (DD->CuaDangNhin() == CanMo) DD->TuongTac();
		return;
	}

	// Kẹt: lùi lại rồi lách ngang (đổi bên mỗi 1,5 s), không đi tới
	if (S > QuangTruoc + 30.f) { QuangTruoc = S; KhongTien = 0.f; }
	else KhongTien += Dt;
	if (KhongTien > 2.5f && !BanDo::TrongBuongCuoi(P) && !BanDo::TrongBuongDau(P) && !CanMo)
	{
		const float Ben = FMath::Fmod(KhongTien, 3.f) < 1.5f ? 1.f : -1.f;
		N->DiChuyen(FVector2D(Ben, -0.5f), false);
		if (KhongTien > 6.f) { KhongTien = 0.f; QuangTruoc = S - 31.f; }
		return;
	}

	// Đi tới đích, tránh vật cản (trong hành lang: theo làn; gần đích / trong buồng: đi thẳng)
	const FVector DichLan = Dich;
	NhinVe(DichLan + FVector(0, 0, 70.f), Dt, 6.f);
	N->DiChuyen(FVector2D(0.f, 1.f), bChay);

}
