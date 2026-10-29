#include "SinhVat.h"

#include "AmThanh.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "VatLieu.h"

namespace
{
	constexpr float TiLe = 1.4f;               // to cỡ một con chó lớn, sải chân ~1,7 m
	constexpr float DaiDui = 40.f * TiLe;      // đùi
	constexpr float DaiCang = 46.f * TiLe;     // cẳng
	constexpr float CaoThan = 24.f * TiLe;     // thân cách mặt bám
	constexpr float NguongBuoc = 20.f * TiLe;  // bàn chân lệch khỏi chỗ lý tưởng bao nhiêu thì bước
	constexpr float NhacCao = 14.f * TiLe;
	constexpr float VuonRa = 52.f * TiLe;

	FVector ChieuLenMat(const FVector& V, const FVector& N) { return (V - N * FVector::DotProduct(V, N)).GetSafeNormal(); }
	float GocGiua(const FVector& A, const FVector& B) { return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(A, B), -1.f, 1.f))); }
	float Muot(float T) { T = FMath::Clamp(T, 0.f, 1.f); return T * T * (3.f - 2.f * T); }
}

ASinhVat::ASinhVat()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Goc"));
	Than = CreateDefaultSubobject<USceneComponent>(TEXT("Than"));
	Than->SetupAttachment(RootComponent);
	TiengBo = CreateDefaultSubobject<UAudioComponent>(TEXT("TiengBo"));
	TiengBo->SetupAttachment(Than);
	TiengBo->bAutoActivate = false;
	AnhChieu = CreateDefaultSubobject<UPointLightComponent>(TEXT("AnhChieu"));
	AnhChieu->SetupAttachment(Than);
	AnhChieu->SetRelativeLocation(FVector(40.f, 0.f, 60.f));
	AnhChieu->SetIntensityUnits(ELightUnits::Lumens);
	AnhChieu->SetIntensity(0.f);
	AnhChieu->SetAttenuationRadius(220.f);
	AnhChieu->SetLightColor(FLinearColor(0.7f, 0.75f, 1.f));
	AnhChieu->SetCastShadows(false);
}

UStaticMeshComponent* ASinhVat::Khoi(UStaticMesh* M, UMaterialInterface* Mat)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetStaticMesh(M);
	C->SetMaterial(0, Mat);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetupAttachment(RootComponent);
	C->RegisterComponent();
	return C;
}

void ASinhVat::Bat(const FVector& Diem, const FVector& N, const FVector& Huong, const TArray<FVector>& CacDich, float TocDo)
{
	if (Chan.Num() == 0)
	{
		MatDa = VatLieu::Mau(this, FLinearColor(0.02f, 0.015f, 0.014f), 0.28f, 0.1f);
		// Thân: bụng to phía sau, đầu ngực phía trước, 6 mắt đỏ
		auto Phan = [&](UStaticMesh* M, const FVector& Vi, const FVector& Co, UMaterialInterface* Ma)
		{
			UStaticMeshComponent* C = Khoi(M, Ma);
			C->AttachToComponent(Than, FAttachmentTransformRules::KeepRelativeTransform);
			C->SetRelativeLocation(Vi * TiLe);
			C->SetRelativeScale3D(Co * TiLe / 100.f);
			PhanThan.Add(C);
		};
		Phan(VatLieu::Cau(), FVector(-30.f, 0.f, 6.f), FVector(48.f, 38.f, 32.f), MatDa);
		Phan(VatLieu::Cau(), FVector(8.f, 0.f, 2.f), FVector(30.f, 28.f, 18.f), MatDa);
		UMaterialInterface* Mat = VatLieu::Sang(this, FLinearColor(1.f, 0.05f, 0.03f), 40.f);
		for (float Y : { -5.f, 5.f, -8.f, 8.f, -3.f, 3.f })
			Phan(VatLieu::Cau(), FVector(21.f, Y, FMath::Abs(Y) > 6.f ? 6.f : 8.f), FVector(2.5f), Mat);
		// 8 chân: gốc dọc hai bên đầu ngực, xòe ra trước / ngang / sau
		const float GocX[4] = { 14.f, 8.f, 2.f, -4.f };
		const float XoayChan[4] = { 50.f, 15.f, -20.f, -55.f };
		for (int32 K = 0; K < 8; ++K)
		{
			const int32 Hang = K / 2;
			const float Ben = (K % 2 == 0) ? -1.f : 1.f;   // chẵn = trái (Y-), lẻ = phải
			FChanNhen C;
			C.GocThan = FVector(GocX[Hang], Ben * 9.f, 2.f) * TiLe;
			C.HuongRa = FRotator(0.f, Ben * (90.f - XoayChan[Hang]), 0.f).RotateVector(FVector::ForwardVector);
			// L1 R2 L3 R4 / R1 L2 R3 L4
			C.Nhom = ((Hang % 2 == 0) == (Ben < 0.f)) ? 0 : 1;
			C.Dui = Khoi(VatLieu::Tru(), MatDa);
			C.Cang = Khoi(VatLieu::Tru(), MatDa);
			C.Khop = Khoi(VatLieu::Cau(), MatDa);
			Chan.Add(C);
		}
		TiengBo->SetSound(VatLieu::AmThanh(TEXT("nhen_bo")));
	}
	Neo = Diem;
	PhapNeo = N.GetSafeNormal();
	Truoc = ChieuLenMat(Huong, PhapNeo);
	Dich = CacDich;
	DichSo = 0;
	Toc = TocDo;
	TocThuc = 0.f;
	bChoDoiMat = false;
	MatDaQua.Reset();
	MatDaQua.Add(PhapNeo);
	LenThan = PhapNeo;
	ViTriThan = Neo + PhapNeo * CaoThan;
	VanTocThan = FVector::ZeroVector;
	XoayThan = FRotationMatrix::MakeFromXZ(Truoc, PhapNeo).ToQuat();
	for (FChanNhen& C : Chan)
	{
		FVector D, Nn;
		C.bNhac = false;
		C.BanChan = TimCho(C, GocTheGioi(C) + XoayThan.RotateVector(C.HuongRa) * VuonRa, D, Nn) ? D : Neo;
		C.PhapTuyen = PhapNeo;
	}
	bHien = true;
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
	TiengBo->Play();
}

void ASinhVat::An()
{
	bHien = false;
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
	TiengBo->Stop();
	AnhChieu->SetIntensity(0.f);
}

bool ASinhVat::Do(const FVector& A, const FVector& B, FHitResult& H) const
{
	FCollisionQueryParams Q(SCENE_QUERY_STAT(SinhVat), true);
	Q.AddIgnoredActor(this);
	if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0)) Q.AddIgnoredActor(P);
	return GetWorld()->LineTraceSingleByChannel(H, A, B, ECC_Visibility, Q);
}

FVector ASinhVat::GocTheGioi(const FChanNhen& C) const
{
	return ViTriThan + XoayThan.RotateVector(C.GocThan);
}

bool ASinhVat::TimCho(const FChanNhen& C, const FVector& Muc, FVector& Diem, FVector& N) const
{
	// Ba tia lần lượt: ① gốc chân → mục tiêu (kéo dài): đụng tường = chỗ đặt ở góc lõm
	// ② từ trên mục tiêu xuống theo chiều thân: mặt bình thường  ③ từ dưới mép quay ngang về phía thân: mặt sau góc lồi
	const FVector Goc = GocTheGioi(C);
	const FVector Len = XoayThan.GetUpVector();
	FHitResult H;
	if (Do(Goc, Goc + (Muc - Goc) * 1.25f, H)) { Diem = H.ImpactPoint; N = H.ImpactNormal; return true; }
	if (Do(Muc + Len * 35.f, Muc - Len * 45.f, H)) { Diem = H.ImpactPoint; N = H.ImpactNormal; return true; }
	const FVector Duoi = Muc - Len * 30.f;
	if (Do(Duoi, Duoi + (ViTriThan - Muc).GetSafeNormal() * 60.f, H)) { Diem = H.ImpactPoint; N = H.ImpactNormal; return true; }
	return false;
}

void ASinhVat::DiChuyenNeo(float Dt)
{
	// Đích hiện tại: hướng muốn đi chiếu lên mặt, xoay hướng đi có giới hạn tốc độ quay
	if (!Dich.IsValidIndex(DichSo)) { An(); return; }
	if (FVector::Dist(Neo, Dich[DichSo]) < 60.f)
	{
		++DichSo;
		if (!Dich.IsValidIndex(DichSo)) { An(); return; }
	}
	const FVector Muon = ChieuLenMat(Dich[DichSo] - Neo, PhapNeo);
	if (!Muon.IsNearlyZero())
	{
		const float Goc = GocGiua(Truoc, Muon);
		const float Toi = FMath::Min(Goc, 200.f * Dt);
		if (Goc > 0.5f)
		{
			const FVector Truc = FVector::CrossProduct(Truoc, Muon).GetSafeNormal();
			if (!Truc.IsNearlyZero()) Truoc = FQuat(Truc, FMath::DegreesToRadians(Toi)).RotateVector(Truoc);
		}
	}
	Truoc = ChieuLenMat(Truoc, PhapNeo);

	// Đang chờ chân trước chạm mặt mới: đứng sát góc, chân trước thò ra dò
	if (bChoDoiMat)
	{
		TocThuc = FMath::FInterpTo(TocThuc, 0.f, Dt, 8.f);
		ThoiCho += Dt;
		const bool bChanTruocCham = Chan[0].bChamMatMoi && Chan[1].bChamMatMoi && !Chan[0].bNhac && !Chan[1].bNhac;
		if (bChanTruocCham || ThoiCho > 1.4f)
		{
			// Đổi mặt: vận chuyển song song hướng đi bằng phép quay pháp tuyến cũ → mới
			const FQuat Q = FQuat::FindBetweenNormals(PhapNeo, PhapMatMoi);
			Truoc = ChieuLenMat(Q.RotateVector(Truoc), PhapMatMoi);
			Neo = DiemMatMoi;
			PhapNeo = PhapMatMoi;
			MatDaQua.Add(PhapNeo);
			bChoDoiMat = false;
			for (FChanNhen& C : Chan) C.bChamMatMoi = false;
		}
		return;
	}
	TocThuc = FMath::FInterpTo(TocThuc, Toc, Dt, 4.f);
	const float Buoc = TocThuc * Dt;

	// Góc lõm: tia phía trước gặp mặt có pháp tuyến khác hẳn → mặt kế tiếp; tới gần thì dừng chờ chân trước
	FHitResult H;
	const FVector Dau = Neo + PhapNeo * 12.f;
	if (Do(Dau, Dau + Truoc * (Buoc + 40.f), H) && GocGiua(H.ImpactNormal, PhapNeo) > 35.f)
	{
		if (H.Distance < 22.f)
		{
			bChoDoiMat = true;
			ThoiCho = 0.f;
			PhapMatMoi = H.ImpactNormal;
			DiemMatMoi = H.ImpactPoint + PhapMatMoi * 1.f + PhapNeo * 4.f;
			return;
		}
	}
	Neo += Truoc * Buoc;
	// Chiếu lại lên mặt: tia dọc pháp tuyến
	if (Do(Neo + PhapNeo * 25.f, Neo - PhapNeo * 40.f, H))
	{
		if (GocGiua(H.ImpactNormal, PhapNeo) < 35.f)
		{
			const FQuat Q = FQuat::FindBetweenNormals(PhapNeo, H.ImpactNormal);
			Truoc = ChieuLenMat(Q.RotateVector(Truoc), H.ImpactNormal);
			PhapNeo = H.ImpactNormal;
			Neo = H.ImpactPoint;
		}
		else
		{
			bChoDoiMat = true; ThoiCho = 0.f;
			PhapMatMoi = H.ImpactNormal; DiemMatMoi = H.ImpactPoint;
		}
		return;
	}
	// Góc lồi (mép): tia xuống trượt ra ngoài → từ dưới mép bắn ngược ra sau tìm mặt bên kia
	const FVector DuoiMep = Neo - PhapNeo * 18.f + Truoc * 6.f;
	if (Do(DuoiMep, DuoiMep - Truoc * 50.f, H))
	{
		bChoDoiMat = true; ThoiCho = 0.f;
		PhapMatMoi = H.ImpactNormal; DiemMatMoi = H.ImpactPoint + H.ImpactNormal * 1.f;
		Neo -= Truoc * Buoc;
	}
	else
	{
		Neo -= Truoc * Buoc;   // không có chỗ bám: đứng lại
	}
}

void ASinhVat::BuocChan(float Dt)
{
	// Tần số bước tăng theo tốc độ; mỗi lúc chỉ một nhóm 4 chân được nhấc, nhóm kia phải bám chắc
	const float TocM = TocThuc / 100.f;
	const float TanSo = FMath::Min(1.4f + 1.15f * TocM, 4.6f);
	const float TiLeNhac = FMath::Lerp(0.36f, 0.5f, FMath::Clamp(TocM / 3.f, 0.f, 1.f));
	const float ThoiNhac = TiLeNhac / TanSo;
	PhaDi = FMath::Fmod(PhaDi + Dt * TanSo, 1.f);
	const int32 NhomDi = PhaDi < 0.5f ? 0 : 1;
	const FVector VanToc = Truoc * TocThuc;

	for (int32 K = 0; K < Chan.Num(); ++K)
	{
		FChanNhen& C = Chan[K];
		if (C.bNhac)
		{
			C.Pha += Dt / ThoiNhac;
			if (C.Pha >= 1.f)
			{
				C.bNhac = false;
				C.BanChan = C.DenDiem;
				C.PhapTuyen = C.DenPT;
				if (bChoDoiMat && GocGiua(C.DenPT, PhapMatMoi) < 30.f) C.bChamMatMoi = true;
				VanTocNhun -= 70.f;   // chạm đất: thân nhún xuống
			}
			continue;
		}
		const bool bTruoc = K < 2;
		FVector LyTuong = GocTheGioi(C) + XoayThan.RotateVector(C.HuongRa) * VuonRa - LenThan * CaoThan * 0.6f;
		FVector Diem, N;
		bool bCanBuoc = false;
		if (bChoDoiMat && bTruoc && !C.bChamMatMoi)
		{
			// Chân dò: điểm lý tưởng chiếu lên mặt mới, vươn thêm 14 cm dọc mặt mới, nhấc cao, không đợi pha
			const FVector TrenMat = LyTuong - PhapMatMoi * FVector::DotProduct(LyTuong - DiemMatMoi, PhapMatMoi);
			const FVector Doc = ChieuLenMat(-PhapNeo, PhapMatMoi);
			Diem = TrenMat + Doc * 14.f;
			N = PhapMatMoi;
			FHitResult H;
			if (Do(Diem + N * 20.f, Diem - N * 30.f, H)) { Diem = H.ImpactPoint; N = H.ImpactNormal; }
			bCanBuoc = true;
		}
		else
		{
			// Mục tiêu đặt chân = chỗ lý tưởng + vận tốc × (thời gian nhấc + nửa thời gian bám)
			const FVector Muc = LyTuong + VanToc * (ThoiNhac + 0.5f * (1.f / TanSo - ThoiNhac));
			const float Lech = FVector::Dist(C.BanChan, LyTuong);
			const float VuonXa = FVector::Dist(C.BanChan, GocTheGioi(C)) / (DaiDui + DaiCang);
			bCanBuoc = (C.Nhom == NhomDi && Lech > NguongBuoc) || VuonXa > 0.98f || (bChoDoiMat && GocGiua(C.PhapTuyen, PhapMatMoi) > 30.f && Chan[0].bChamMatMoi);
			if (bCanBuoc && !TimCho(C, Muc, Diem, N)) { Diem = GocTheGioi(C) - LenThan * CaoThan; N = LenThan; }
		}
		// Chỉ nhấc khi các chân kề (cùng hàng bên kia, trước sau cùng bên) đang bám
		if (bCanBuoc)
		{
			const int32 Ke[3] = { K ^ 1, K - 2, K + 2 };
			bool bKeBam = true;
			for (int32 J : Ke) if (Chan.IsValidIndex(J) && Chan[J].bNhac) bKeBam = false;
			if (bKeBam || (bChoDoiMat && bTruoc))
			{
				C.bNhac = true;
				C.Pha = 0.f;
				C.TuDiem = C.BanChan; C.TuPT = C.PhapTuyen;
				C.DenDiem = Diem; C.DenPT = N;
			}
		}
	}
}

void ASinhVat::DatThan(float Dt)
{
	// Mặt phẳng bàn chân theo phương pháp Newell (chân xếp vòng quanh thân), trọng tâm bàn chân
	const int32 Vong[8] = { 0, 2, 4, 6, 7, 5, 3, 1 };
	FVector Newell = FVector::ZeroVector, TrongTam = FVector::ZeroVector;
	for (int32 I = 0; I < 8; ++I)
	{
		const FVector& A = Chan[Vong[I]].BanChan;
		const FVector& B = Chan[Vong[(I + 1) % 8]].BanChan;
		Newell.X += (A.Y - B.Y) * (A.Z + B.Z);
		Newell.Y += (A.Z - B.Z) * (A.X + B.X);
		Newell.Z += (A.X - B.X) * (A.Y + B.Y);
		TrongTam += A;
	}
	TrongTam /= 8.f;
	Newell = Newell.GetSafeNormal();
	if (FVector::DotProduct(Newell, PhapNeo) < 0.f) Newell = -Newell;
	// Pháp tuyến các chân đang bám, mặt càng gần mặt neo càng nặng ký (tránh hai mặt tấm mỏng triệt tiêu nhau)
	FVector PTChan = FVector::ZeroVector;
	for (const FChanNhen& C : Chan)
	{
		const FVector& N = C.bNhac ? C.TuPT : C.PhapTuyen;
		PTChan += N * (0.3f + FMath::Max(0.f, FVector::DotProduct(N, PhapNeo)));
	}
	PTChan = PTChan.GetSafeNormal();
	const FVector LenMuon = (PhapNeo * 0.2f + PTChan * 0.15f + (Newell.IsNearlyZero() ? PhapNeo : Newell) * 0.65f).GetSafeNormal();
	LenThan = FMath::VInterpNormalRotationTo(LenThan, LenMuon, Dt, 220.f);

	// Vị trí: neo + pháp tuyến × độ cao  pha 35:65 với  trọng tâm bàn chân + pháp tuyến bàn chân × độ cao; lò xo gần tắt dần tới hạn
	const FVector Muc = (Neo + PhapNeo * CaoThan) * 0.35f + (TrongTam + LenThan * CaoThan) * 0.65f;
	const float K = 160.f, C = 2.f * FMath::Sqrt(K) * 0.95f;
	VanTocThan += ((Muc - ViTriThan) * K - VanTocThan * C) * Dt;
	ViTriThan += VanTocThan * Dt;
	// Nhún: lò xo hơi thiếu tắt dần
	VanTocNhun += (-NhunCao * 260.f - VanTocNhun * 16.f) * Dt;
	NhunCao += VanTocNhun * Dt;

	const FVector TruocThan = ChieuLenMat(Truoc, LenThan);
	const FQuat Muon = FRotationMatrix::MakeFromXZ(TruocThan.IsNearlyZero() ? Truoc : TruocThan, LenThan).ToQuat();
	XoayThan = FQuat::Slerp(XoayThan, Muon, FMath::Clamp(Dt * 10.f, 0.f, 1.f));
	Than->SetWorldLocationAndRotation(ViTriThan + LenThan * NhunCao * 0.15f, XoayThan);
}

void ASinhVat::VeChan()
{
	// Mỗi chân: IK 2 đoạn (đùi, cẳng), đầu gối nhô lên trên và ra ngoài
	for (FChanNhen& C : Chan)
	{
		FVector Ban = C.BanChan;
		if (C.bNhac)
		{
			const float T = Muot(C.Pha);
			const FVector Len = (C.TuPT + C.DenPT + LenThan).GetSafeNormal();
			Ban = FMath::Lerp(C.TuDiem, C.DenDiem, T) + Len * FMath::Sin(T * PI) * NhacCao * (C.DenPT != C.TuPT ? 1.6f : 1.f);
		}
		const FVector Goc = GocTheGioi(C);
		FVector D = Ban - Goc;
		const float Xa = FMath::Clamp(D.Size(), 5.f, DaiDui + DaiCang - 0.5f);
		const FVector Huong = D.GetSafeNormal();
		const FVector Ra = XoayThan.RotateVector(C.HuongRa);
		FVector Gap = (LenThan * 0.75f + Ra * 0.35f);
		Gap = (Gap - Huong * FVector::DotProduct(Gap, Huong)).GetSafeNormal();
		const float CosA = (DaiDui * DaiDui + Xa * Xa - DaiCang * DaiCang) / (2.f * DaiDui * Xa);
		const float A = FMath::Acos(FMath::Clamp(CosA, -1.f, 1.f));
		const FVector Goi = Goc + Huong * FMath::Cos(A) * DaiDui + Gap * FMath::Sin(A) * DaiDui;
		const FVector Cuoi = Goc + Huong * Xa;
		auto Doan = [](UStaticMeshComponent* M, const FVector& P0, const FVector& P1, float BanKinh)
		{
			const FVector V = P1 - P0;
			M->SetWorldLocationAndRotation((P0 + P1) * 0.5f, FRotationMatrix::MakeFromZ(V.GetSafeNormal()).ToQuat());
			M->SetWorldScale3D(FVector(BanKinh / 50.f, BanKinh / 50.f, V.Size() / 100.f));
		};
		Doan(C.Dui, Goc, Goi, 2.6f * TiLe);
		Doan(C.Cang, Goi, Cuoi, 1.6f * TiLe);
		C.Khop->SetWorldLocation(Goi);
		C.Khop->SetWorldScale3D(FVector(0.05f * TiLe));
	}
}

void ASinhVat::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!bHien) return;
	ThoiGian += Dt;
	Dt = FMath::Min(Dt, 0.05f);
	DiChuyenNeo(Dt);
	if (!bHien) return;
	BuocChan(Dt);
	DatThan(Dt);
	VeChan();
}
