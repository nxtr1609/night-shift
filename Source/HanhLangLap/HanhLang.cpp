#include "HanhLang.h"

#include "BanDo.h"
#include "Components/DecalComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"
#include "Cua.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureCube.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "VatLieu.h"

using namespace BanDo;

namespace
{
	// Dựng lưới hộp có UV theo kích thước thật (texture lặp theo cm)
	struct FXay
	{
		TArray<FVector> V;
		TArray<int32> T;
		TArray<FVector> N;
		TArray<FVector2D> UV;
		TArray<FProcMeshTangent> Tg;
		float SU = 100.f, SV = 100.f;
		bool bTuong = false;   // mặt đứng: V tính từ trần xuống (texture tường hai màu)

		FVector2D TinhUV(const FVector& P, const FVector& Nm) const
		{
			const float VDung = bTuong ? (Cao - P.Z) / SV : -P.Z / SV;
			if (FMath::Abs(Nm.X) > 0.5f) return FVector2D(P.Y / SU, VDung);
			if (FMath::Abs(Nm.Y) > 0.5f) return FVector2D(P.X / SU, VDung);
			return FVector2D(P.X / SU, P.Y / SV);
		}

		void Mat(const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& Nm)
		{
			const int32 G = V.Num();
			for (const FVector& P : { A, B, C, D })
			{
				V.Add(P);
				N.Add(Nm);
				UV.Add(TinhUV(P, Nm));
				const FVector TX = FMath::Abs(Nm.Z) > 0.5f ? FVector(1, 0, 0) : (FMath::Abs(Nm.X) > 0.5f ? FVector(0, 1, 0) : FVector(1, 0, 0));
				Tg.Add(FProcMeshTangent(TX, false));
			}
			// UE: mặt trước khi các đỉnh xếp theo chiều kim đồng hồ nhìn từ phía pháp tuyến
			const bool bThuan = FVector::DotProduct(FVector::CrossProduct(B - A, C - A), Nm) < 0.f;
			if (bThuan) T.Append({ G, G + 1, G + 2, G, G + 2, G + 3 });
			else T.Append({ G, G + 2, G + 1, G, G + 3, G + 2 });
		}

		void Hop(const FVector& Mn, const FVector& Mx)
		{
			const FVector P[8] = {
				{ Mn.X, Mn.Y, Mn.Z }, { Mx.X, Mn.Y, Mn.Z }, { Mx.X, Mx.Y, Mn.Z }, { Mn.X, Mx.Y, Mn.Z },
				{ Mn.X, Mn.Y, Mx.Z }, { Mx.X, Mn.Y, Mx.Z }, { Mx.X, Mx.Y, Mx.Z }, { Mn.X, Mx.Y, Mx.Z } };
			Mat(P[4], P[5], P[6], P[7], FVector(0, 0, 1));
			Mat(P[0], P[3], P[2], P[1], FVector(0, 0, -1));
			Mat(P[0], P[1], P[5], P[4], FVector(0, -1, 0));
			Mat(P[2], P[3], P[7], P[6], FVector(0, 1, 0));
			Mat(P[1], P[2], P[6], P[5], FVector(1, 0, 0));
			Mat(P[3], P[0], P[4], P[7], FVector(-1, 0, 0));
		}

		// Hộp theo khung biến đổi T (buồng cầu thang xoay 90°)
		void HopT(const FTransform& Tf, const FVector& Mn, const FVector& Mx)
		{
			const FVector A = Tf.TransformPosition(Mn), B = Tf.TransformPosition(Mx);
			Hop(A.ComponentMin(B), A.ComponentMax(B));
		}

		// Tường dọc trục: chạy theo trục X (bTheoX) hoặc Y từ Dau tới Cuoi, dày từ Mat0 tới Mat1 (trục còn lại),
		// chừa các lỗ (tâm dọc tường, nửa rộng, đáy, đỉnh).
		void Tuong(bool bTheoX, float Dau, float Cuoi, float Mat0, float Mat1, const TArray<FVector4>& Lo, float Day = 0.f, float Dinh = Cao)
		{
			auto H = [&](float A0, float A1, float Z0, float Z1)
			{
				if (A1 - A0 < 0.5f || Z1 - Z0 < 0.5f) return;
				if (bTheoX) Hop(FVector(A0, Mat0, Z0), FVector(A1, Mat1, Z1));
				else Hop(FVector(Mat0, A0, Z0), FVector(Mat1, A1, Z1));
			};
			TArray<FVector4> DS = Lo;
			DS.Sort([](const FVector4& A, const FVector4& B) { return A.X < B.X; });
			float Dang = Dau;
			for (const FVector4& L : DS)
			{
				const float A0 = L.X - L.Y, A1 = L.X + L.Y;
				H(Dang, A0, Day, Dinh);
				H(A0, A1, Day, L.Z);
				H(A0, A1, L.W, Dinh);
				Dang = A1;
			}
			H(Dang, Cuoi, Day, Dinh);
		}

		void Xuat(UProceduralMeshComponent* L, UMaterialInterface* M)
		{
			L->CreateMeshSection(L->GetNumSections(), V, T, N, UV, TArray<FColor>(), Tg, true);
			L->SetMaterial(L->GetNumSections() - 1, M);
			V.Reset(); T.Reset(); N.Reset(); UV.Reset(); Tg.Reset();
		}
	};

	UProceduralMeshComponent* TaoLuoi(AActor* Chu, const TCHAR* Ten)
	{
		UProceduralMeshComponent* L = NewObject<UProceduralMeshComponent>(Chu, Ten);
		L->SetupAttachment(Chu->GetRootComponent());
		L->bUseComplexAsSimpleCollision = true;
		L->RegisterComponent();
		return L;
	}

	// Lỗ cửa trong tường: (tâm, nửa rộng, đáy, đỉnh)
	FVector4 LoCua(float Tam) { return FVector4(Tam, 58.f, 0.f, 222.f); }
	FVector4 LoCuaSo(float Tam) { return FVector4(Tam, 80.f, 95.f, 235.f); }

	const FLinearColor MauSonCua(0.85f, 0.95f, 1.f);
}

AHanhLang::AHanhLang()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Goc"));
}

UStaticMeshComponent* AHanhLang::Khoi(const FVector& Tam, const FVector& CoCm, UMaterialInterface* M, const FRotator& Xoay, USceneComponent* Cha)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetStaticMesh(VatLieu::Khoi());
	C->SetupAttachment(Cha ? Cha : RootComponent.Get());
	C->SetRelativeLocation(Tam);
	C->SetRelativeRotation(Xoay);
	C->SetRelativeScale3D(CoCm / 100.f);
	C->SetMaterial(0, M);
	C->RegisterComponent();
	return C;
}

UStaticMeshComponent* AHanhLang::Mau(FName Ten, const FString& TenMau, const FVector& ViTri, float Yaw, float TiLe, bool bHien)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this, Ten);
	C->SetStaticMesh(VatLieu::MauTinh(TenMau));
	C->SetupAttachment(RootComponent);
	C->SetRelativeLocation(ViTri);
	C->SetRelativeRotation(FRotator(0.f, Yaw, 0.f));
	C->SetRelativeScale3D(FVector(TiLe));
	C->RegisterComponent();
	C->SetVisibility(bHien);
	C->SetCollisionEnabled(bHien ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	FVatDung VD;
	VD.C = C;
	VD.Goc = C->GetRelativeTransform();
	VD.bHienGoc = bHien;
	VatDung.Add(Ten, VD);
	return C;
}

UDecalComponent* AHanhLang::Vet(FName Ten, const FString& Anh, const FVector& ViTri, const FRotator& Huong, const FVector& Co, bool bHien)
{
	UDecalComponent* D = NewObject<UDecalComponent>(this, Ten);
	D->SetupAttachment(RootComponent);
	D->SetDecalMaterial(VatLieu::Decal(this, Anh));
	D->DecalSize = Co;
	D->SetRelativeLocation(ViTri);
	D->SetRelativeRotation(Huong);
	D->RegisterComponent();
	D->SetVisibility(bHien);
	FVatDung VD;
	VD.C = D;
	VD.Goc = D->GetRelativeTransform();
	VD.bHienGoc = bHien;
	VatDung.Add(Ten, VD);
	return D;
}

ACua* AHanhLang::TaoCua(FName Ten, const FVector& ViTri, float Yaw, const FString& Bien, bool bKinh, bool bKhoa, const FLinearColor& MauSon)
{
	ACua* C = GetWorld()->SpawnActor<ACua>(ViTri, FRotator(0.f, Yaw, 0.f));
	C->Dung(Bien, bKinh, MauSon);
	C->bKhoa = bKhoa;
	C->Ten = Ten.ToString();
	Cua.Add(Ten, C);
	ActorCon.Add(C);
	return C;
}

void AHanhLang::DenTran(const FVector& ViTri, float Yaw, const FString& Nhom)
{
	FDen D;
	D.Nhom = Nhom;
	D.Hat = FMath::FRand() * 100.f;
	D.Mau = FLinearColor(0.88f, 0.97f, 1.f);
	D.CuongDo = 1700.f;
	// Hộp đèn: mẫu SimReady nếu có, cộng tấm tản sáng phát sáng
	if (UStaticMesh* M = VatLieu::MauTinh(TEXT("lamp_ceiling_1m_rectangle_a01")))
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetStaticMesh(M);
		C->SetupAttachment(RootComponent);
		C->SetRelativeLocation(FVector(ViTri.X, ViTri.Y, Cao - 30.f));
		C->SetRelativeRotation(FRotator(0.f, Yaw, 0.f));
		C->SetCastShadow(false);
		C->RegisterComponent();
	}
	else
	{
		Khoi(FVector(ViTri.X, ViTri.Y, Cao - 15.f), FVector(100.f, 40.f, 30.f), VatLieu::Mau(this, FLinearColor(0.75f, 0.75f, 0.72f), 0.5f), FRotator(0.f, Yaw, 0.f));
	}
	D.DoSangMat = 80.f;
	D.MatSang = VatLieu::Sang(this, D.Mau, D.DoSangMat);
	UStaticMeshComponent* Tam = Khoi(FVector(ViTri.X, ViTri.Y, Cao - 30.5f), FVector(92.f, 30.f, 1.f), D.MatSang, FRotator(0.f, Yaw, 0.f));
	Tam->SetCastShadow(false);
	Tam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	URectLightComponent* L = NewObject<URectLightComponent>(this);
	L->SetupAttachment(RootComponent);
	L->SetRelativeLocation(FVector(ViTri.X, ViTri.Y, Cao - 32.f));
	L->SetRelativeRotation(FRotator(-90.f, Yaw, 0.f));
	L->SetIntensityUnits(ELightUnits::Lumens);
	L->SetIntensity(D.CuongDo);
	L->SetLightColor(D.Mau);
	L->SetSourceWidth(92.f);
	L->SetSourceHeight(30.f);
	L->SetAttenuationRadius(900.f);
	L->SetBarnDoorAngle(88.f);
	L->RegisterComponent();
	D.Den = L;
	// Đèn phụ không bóng đổ: hắt sáng trần và tường quanh hộp đèn
	UPointLightComponent* Phu = NewObject<UPointLightComponent>(this);
	Phu->SetupAttachment(RootComponent);
	Phu->SetRelativeLocation(FVector(ViTri.X, ViTri.Y, Cao - 20.f));
	Phu->SetIntensityUnits(ELightUnits::Lumens);
	Phu->SetCastShadows(false);
	Phu->SetAttenuationRadius(420.f);
	Phu->SetLightColor(D.Mau);
	Phu->RegisterComponent();
	D.Phu = Phu;
	D.CuongDoPhu = 260.f;
	Den.Add(D);
}

void AHanhLang::Dung()
{
	LuoiSan = TaoLuoi(this, TEXT("LuoiSan"));
	LuoiTuong = TaoLuoi(this, TEXT("LuoiTuong"));
	LuoiTran = TaoLuoi(this, TEXT("LuoiTran"));
	LuoiBeTong = TaoLuoi(this, TEXT("LuoiBeTong"));
	LuoiKhac = TaoLuoi(this, TEXT("LuoiKhac"));
	DungTuong();
	DungBuong(BuongDau(), TEXT("Dau"));
	DungBuong(BuongCuoi(), TEXT("Cuoi"));
	DungPhongBenh(400.f, 300.f, false);
	DungPhongBenh(1000.f, 300.f, false);
	DungPhongBenh(1450.f, 300.f, true);
	DungSanTrong();
	DungDoDat();
	DungAnhSang();
	DatLai();
}

void AHanhLang::DungTuong()
{
	FXay X;
	// Sàn + trần hành lang (A, góc, B)
	X.SU = X.SV = 120.f;
	// (sàn / trần dừng trước tâm tường buồng cầu thang để không chồng mặt ở ngưỡng cửa)
	X.Hop(FVector(10.f, -NuaRong - 20.f, -10.f), FVector(2060.f, NuaRong + 20.f, 0.f));
	X.Hop(FVector(1780.f, NuaRong + 20.f, -10.f), FVector(2060.f, CuoiB - 10.f, 0.f));
	X.Xuat(LuoiSan, VatLieu::BeMat(this, TEXT("san"), FLinearColor::White, 0.35f));
	X.Hop(FVector(10.f, -NuaRong - 20.f, Cao), FVector(2060.f, NuaRong + 20.f, Cao + 10.f));
	X.Hop(FVector(1780.f, NuaRong + 20.f, Cao), FVector(2060.f, CuoiB - 10.f, Cao + 10.f));
	X.Xuat(LuoiTran, VatLieu::BeMat(this, TEXT("tran"), FLinearColor::White, 0.9f));

	// Tường: texture rộng 200 cm, cao 280 cm
	X.SU = 200.f; X.SV = Cao; X.bTuong = true;
	// A bên trái (Y-) kéo tới hết góc: cửa phòng 401, 402, 403
	X.Tuong(true, 0.f, 2060.f, -NuaRong - 20.f, -NuaRong, { LoCua(400.f), LoCua(1000.f), LoCua(1450.f) });
	// A bên phải (Y+): cửa sổ nhìn ra sân trong
	X.Tuong(true, 0.f, 1780.f, NuaRong, NuaRong + 20.f, { LoCuaSo(300.f), LoCuaSo(700.f), LoCuaSo(1100.f), LoCuaSo(1500.f) });
	// Tường cuối góc + B bên trái (X+ khi đi theo +Y): cửa phòng 405
	X.Tuong(false, -NuaRong - 20.f, CuoiB - 10.f, 2040.f, 2060.f, { LoCua(500.f) });
	// B bên phải (X-): cửa sổ nhìn ra sân trong
	X.Tuong(false, NuaRong, CuoiB - 10.f, 1780.f, 1800.f, { LoCuaSo(350.f), LoCuaSo(700.f), LoCuaSo(1000.f) });
	X.Xuat(LuoiTuong, VatLieu::BeMat(this, TEXT("tuong"), FLinearColor::White, 0.6f));

	// Khung cửa sổ, bậu, kính
	UMaterialInterface* MKhung = VatLieu::Mau(this, FLinearColor(0.35f, 0.37f, 0.38f), 0.35f, 0.8f);
	MatKinh = VatLieu::Kinh(this);
	UMaterialInterface* MKinh = MatKinh;
	MatMuaKinh = VatLieu::InKinh(this, TEXT("giot_kinh"), 0.f);
	auto CuaSo = [&](const FVector& Tam, bool bTheoX)
	{
		const FRotator R(0.f, bTheoX ? 0.f : 90.f, 0.f);
		const FVector Ngang = bTheoX ? FVector(1, 0, 0) : FVector(0, 1, 0);
		Khoi(Tam + FVector(0, 0, 95.f - 165.f + 3.f), R.RotateVector(FVector(166.f, 34.f, 6.f)).GetAbs(), MKhung);
		Khoi(Tam + FVector(0, 0, 235.f - 165.f - 3.f), R.RotateVector(FVector(166.f, 24.f, 6.f)).GetAbs(), MKhung);
		for (float S : { -77.f, 0.f, 77.f })
			Khoi(Tam + Ngang * S, R.RotateVector(FVector(5.f, 8.f, 140.f)).GetAbs(), MKhung);
		UStaticMeshComponent* K = Khoi(Tam, R.RotateVector(FVector(160.f, 1.f, 140.f)).GetAbs(), MKinh);
		K->SetCastShadow(false);
		// Lớp giọt mưa sát mặt ngoài kính (ngoài trời: +Y với đoạn A, -X với đoạn B)
		UStaticMeshComponent* G = Khoi(Tam + (bTheoX ? FVector(0.f, 1.2f, 0.f) : FVector(-1.2f, 0.f, 0.f)), R.RotateVector(FVector(160.f, 0.1f, 140.f)).GetAbs(), MatMuaKinh);
		G->SetCastShadow(false);
		G->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	};
	for (float S : { 300.f, 700.f, 1100.f, 1500.f }) CuaSo(FVector(S, NuaRong + 10.f, 165.f), true);
	for (float S : { 350.f, 700.f, 1000.f }) CuaSo(FVector(1790.f, S, 165.f), false);

	// Dấu tay áp lên mặt ngoài kính (ẩn sẵn; đạo diễn cho hiện dần)
	MatTayKinh = VatLieu::InKinh(this, TEXT("tay_kinh"));
	auto TayKinh = [&](const FVector& ViTri, bool bTheoX, float Xoay, float Co)
	{
		const FName Ten(*FString::Printf(TEXT("TayKinh%d"), SoTayKinh++));
		UStaticMeshComponent* C = Khoi(ViTri, bTheoX ? FVector(Co, 0.1f, Co) : FVector(0.1f, Co, Co), MatTayKinh, bTheoX ? FRotator(0.f, 0.f, Xoay) : FRotator(Xoay, 0.f, 0.f));
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetVisibility(false);
		FVatDung V; V.C = C; V.Goc = C->GetRelativeTransform(); V.bHienGoc = false;
		VatDung.Add(Ten, V);
	};
	for (float S : { 300.f, 700.f, 1100.f, 1500.f })
	{
		TayKinh(FVector(S - 35.f, NuaRong + 11.5f, 175.f), true, 12.f, 46.f);
		TayKinh(FVector(S + 30.f, NuaRong + 11.5f, 140.f), true, -20.f, 42.f);
	}
	TayKinh(FVector(1788.5f, 680.f, 170.f), false, 8.f, 46.f);
	TayKinh(FVector(1788.5f, 735.f, 128.f), false, -15.f, 42.f);

	// Tay vịn dọc tường + khung cửa
	UMaterialInterface* MVin = VatLieu::Mau(this, FLinearColor(0.55f, 0.42f, 0.3f), 0.45f);
	auto TayVin = [&](bool bTheoX, float Dau, float Cuoi, float Mat, const TArray<float>& Cat)
	{
		float Dang = Dau;
		TArray<float> C = Cat; C.Add(Cuoi + 1000.f);
		for (float K : C)
		{
			const float A1 = FMath::Min(K - 65.f, Cuoi);
			if (A1 - Dang > 20.f)
			{
				const float Giua = (Dang + A1) * 0.5f, Dai = A1 - Dang;
				Khoi(bTheoX ? FVector(Giua, Mat, 92.f) : FVector(Mat, Giua, 92.f), bTheoX ? FVector(Dai, 6.f, 5.f) : FVector(6.f, Dai, 5.f), MVin);
			}
			Dang = K + 65.f;
		}
	};
	TayVin(true, 20.f, 2030.f, -NuaRong + 3.f, { 400.f, 1000.f, 1450.f });
	TayVin(false, -100.f, CuoiB - 20.f, 2037.f, { 500.f });

	UMaterialInterface* MVienCua = VatLieu::Mau(this, FLinearColor(0.42f, 0.47f, 0.5f), 0.5f, 0.3f);
	auto VienCua = [&](const FVector& Tam, float Yaw)
	{
		const FRotator R(0.f, Yaw, 0.f);
		for (float S : { -58.f, 58.f })
			Khoi(Tam + R.RotateVector(FVector(0, S, 111.f)), R.RotateVector(FVector(26.f, 8.f, 222.f)).GetAbs(), MVienCua);
		Khoi(Tam + R.RotateVector(FVector(0, 0, 226.f)), R.RotateVector(FVector(26.f, 124.f, 8.f)).GetAbs(), MVienCua);
	};
	// Cửa phòng bệnh (mở vào trong phòng = hướng -Y), 401 và 405 luôn khóa
	TaoCua(TEXT("P401"), FVector(400.f, -NuaRong - 10.f, 0.f), -90.f, TEXT("bien_401"), true, true, MauSonCua);
	TaoCua(TEXT("P402"), FVector(1000.f, -NuaRong - 10.f, 0.f), -90.f, TEXT("bien_402"), true, true, MauSonCua);
	TaoCua(TEXT("P403"), FVector(1450.f, -NuaRong - 10.f, 0.f), -90.f, TEXT("bien_403"), true, true, MauSonCua);
	TaoCua(TEXT("P405"), FVector(2050.f, 500.f, 0.f), 0.f, TEXT("bien_405"), true, true, MauSonCua);
	for (float S : { 400.f, 1000.f, 1450.f }) VienCua(FVector(S, -NuaRong - 10.f, 0.f), 90.f);
	VienCua(FVector(2050.f, 500.f, 0.f), 0.f);

	// Biển khoa trên tường cuối đoạn A + biển thoát hiểm trên cửa buồng cuối
	Khoi(FVector(2038.f, -40.f, 248.f), FVector(1.f, 150.f, 24.f), VatLieu::BeMat(this, TEXT("bien_khoa"), FLinearColor::White, 0.5f, 2.f));
	MatBienThoat = VatLieu::BeMat(this, TEXT("bien_loi_thoat"), FLinearColor::White, 0.5f, 60.f);
	Khoi(FVector(GocX, CuoiB - 12.f, 250.f), FVector(48.f, 4.f, 18.f), MatBienThoat);
}

void AHanhLang::DungBuong(const FTransform& T, const FString& Ten)
{
	// Buồng cầu thang 300 x 300 bê tông, cửa vào (-X cục bộ) và cửa ra (+X cục bộ)
	FXay X;
	X.SU = X.SV = 300.f;
	const float N = NuaBuong;
	X.HopT(T, FVector(-N - 10.f, -N - 10.f, -10.f), FVector(N + 10.f, N + 10.f, 0.f));
	X.HopT(T, FVector(-N - 10.f, -N - 10.f, Cao), FVector(N + 10.f, N + 10.f, Cao + 10.f));
	// Hai tường có cửa (dày 20, tâm ở ±150)
	for (float Mx : { -N, N })
	{
		X.HopT(T, FVector(Mx - 10.f, -N - 10.f, 0.f), FVector(Mx + 10.f, -58.f, Cao));
		X.HopT(T, FVector(Mx - 10.f, 58.f, 0.f), FVector(Mx + 10.f, N + 10.f, Cao));
		X.HopT(T, FVector(Mx - 10.f, -58.f, 222.f), FVector(Mx + 10.f, 58.f, Cao));
	}
	X.HopT(T, FVector(-N - 10.f, -N - 10.f, 0.f), FVector(N + 10.f, -N + 10.f, Cao));
	X.HopT(T, FVector(-N - 10.f, N - 10.f, 0.f), FVector(N + 10.f, N + 10.f, Cao));
	// Bậc cầu thang đi xuống bị chặn (chỉ là mấy bậc dẫn vào bóng tối) + bệ ghế
	for (int32 K = 0; K < 4; ++K)
		X.HopT(T, FVector(-60.f + K * 28.f, N - 70.f, 0.f), FVector(-32.f + K * 28.f, N - 10.f, 60.f - K * 15.f));
	X.HopT(T, FVector(-N + 10.f, -N + 10.f, 0.f), FVector(-N + 70.f, -N + 120.f, 45.f));
	X.Xuat(LuoiBeTong, VatLieu::BeMat(this, TEXT("be_tong"), FLinearColor(0.85f, 0.85f, 0.85f), 0.85f));

	UMaterialInterface* MVienCua = VatLieu::Mau(this, FLinearColor(0.42f, 0.47f, 0.5f), 0.5f, 0.3f);
	const FRotator R = T.Rotator();
	for (float Mx : { -N, N })
	{
		for (float S : { -58.f, 58.f })
			Khoi(T.TransformPosition(FVector(Mx, S, 111.f)), R.RotateVector(FVector(26.f, 8.f, 222.f)).GetAbs(), MVienCua);
		Khoi(T.TransformPosition(FVector(Mx, 0.f, 226.f)), R.RotateVector(FVector(26.f, 124.f, 8.f)).GetAbs(), MVienCua);
	}
	// Biển "cầu thang bộ" + ống nước trên trần
	Khoi(T.TransformPosition(FVector(30.f, N - 11.f, 190.f)), R.RotateVector(FVector(48.f, 1.f, 15.f)).GetAbs(), VatLieu::BeMat(this, TEXT("bien_cau_thang"), FLinearColor::White, 0.5f, 1.f));
	Khoi(T.TransformPosition(FVector(0.f, -N + 25.f, Cao - 18.f)), R.RotateVector(FVector(300.f, 10.f, 10.f)).GetAbs(), VatLieu::Mau(this, FLinearColor(0.3f, 0.25f, 0.2f), 0.5f, 0.8f));

	// Cửa: buồng đầu → cửa ra là cửa thật (Dau), cửa vào là cửa giả; buồng cuối → cửa vào là cửa thật (Cuoi), cửa ra giả
	const bool bDau = Ten == TEXT("Dau");
	ACua* Vao = TaoCua(bDau ? TEXT("GiaDau") : TEXT("Cuoi"), T.TransformPosition(FVector(-N, 0.f, 0.f)), R.Yaw, FString(), true, bDau, MauSonCua);
	ACua* Ra = TaoCua(bDau ? TEXT("Dau") : TEXT("GiaCuoi"), T.TransformPosition(FVector(N, 0.f, 0.f)), R.Yaw, FString(), true, !bDau, MauSonCua);
	(void)Vao; (void)Ra;

	// Bóng đèn trần trần trụi
	FDen D;
	D.Nhom = TEXT("Buong");
	D.Hat = FMath::FRand() * 100.f;
	D.Mau = FLinearColor(1.f, 0.82f, 0.6f);
	D.CuongDo = 500.f;
	D.DoSangMat = 400.f;
	const FVector ViTriBong = T.TransformPosition(FVector(0.f, 0.f, Cao - 45.f));
	Khoi(ViTriBong + FVector(0, 0, 22.f), FVector(1.f, 1.f, 40.f), VatLieu::Mau(this, FLinearColor(0.05f, 0.05f, 0.05f)));
	D.MatSang = VatLieu::Sang(this, D.Mau, D.DoSangMat);
	UStaticMeshComponent* Bong = NewObject<UStaticMeshComponent>(this);
	Bong->SetStaticMesh(VatLieu::Cau());
	Bong->SetupAttachment(RootComponent);
	Bong->SetRelativeLocation(ViTriBong);
	Bong->SetRelativeScale3D(FVector(0.07f, 0.07f, 0.09f));
	Bong->SetMaterial(0, D.MatSang);
	Bong->SetCastShadow(false);
	Bong->RegisterComponent();
	UPointLightComponent* P = NewObject<UPointLightComponent>(this);
	P->SetupAttachment(RootComponent);
	P->SetRelativeLocation(ViTriBong - FVector(0, 0, 8.f));
	P->SetIntensityUnits(ELightUnits::Lumens);
	P->SetIntensity(D.CuongDo);
	P->SetLightColor(D.Mau);
	P->SetAttenuationRadius(600.f);
	P->SetSourceRadius(3.f);
	P->RegisterComponent();
	D.Bong = P;
	Den.Add(D);
}

void AHanhLang::DungPhongBenh(float X0, float Sau, bool bCoDo)
{
	// Phòng bệnh sau cửa (phía Y-): tối, chỉ thấy khi cửa hé
	FXay X;
	const float Y0 = -NuaRong - 20.f, Y1 = Y0 - Sau;
	X.SU = X.SV = 120.f;
	X.Hop(FVector(X0 - 180.f, Y1 - 20.f, -10.f), FVector(X0 + 180.f, Y0, 0.f));
	X.Xuat(LuoiSan, VatLieu::BeMat(this, TEXT("san"), FLinearColor(0.8f, 0.8f, 0.8f), 0.4f));
	X.Hop(FVector(X0 - 180.f, Y1 - 20.f, Cao), FVector(X0 + 180.f, Y0, Cao + 10.f));
	X.Xuat(LuoiTran, VatLieu::BeMat(this, TEXT("tran"), FLinearColor(0.8f, 0.8f, 0.8f), 0.9f));
	X.SU = 200.f; X.SV = Cao; X.bTuong = true;
	X.Tuong(true, X0 - 180.f, X0 + 180.f, Y1 - 20.f, Y1, { LoCuaSo(X0) });
	X.Hop(FVector(X0 - 180.f, Y1 - 20.f, 0.f), FVector(X0 - 160.f, Y0, Cao));
	X.Hop(FVector(X0 + 160.f, Y1 - 20.f, 0.f), FVector(X0 + 180.f, Y0, Cao));
	X.Xuat(LuoiTuong, VatLieu::BeMat(this, TEXT("tuong"), FLinearColor(0.75f, 0.75f, 0.75f), 0.6f));
	Khoi(FVector(X0, Y1 - 10.f, 165.f), FVector(160.f, 2.f, 140.f), VatLieu::PhuMo(this, FLinearColor(0.f, 0.f, 0.f), 0.6f));
	const FName Tien(*FString::Printf(TEXT("Phong%d"), FMath::RoundToInt(X0)));
	if (bCoDo)
	{
		Mau(FName(*(Tien.ToString() + TEXT("_Giuong"))), TEXT("bed_hospital_standard_a01"), FVector(X0 - 40.f, Y1 + 120.f, 0.f), 0.f);
		Mau(FName(*(Tien.ToString() + TEXT("_May"))), TEXT("monitor_patient_a01"), FVector(X0 + 110.f, Y1 + 40.f, 0.f), 180.f);
		Mau(FName(*(Tien.ToString() + TEXT("_Rem"))), TEXT("screen_hospital_b01"), FVector(X0 - 150.f, Y1 + 40.f, 0.f), 0.f, 0.68f);
	}
	// Đèn đầu giường (tắt sẵn; đạo diễn bật khi cần)
	FDen D;
	D.Nhom = Tien.ToString();
	D.Mau = FLinearColor(1.f, 0.35f, 0.3f);
	D.CuongDo = 250.f;
	D.Muc = 0.f;
	UPointLightComponent* P = NewObject<UPointLightComponent>(this);
	P->SetupAttachment(RootComponent);
	P->SetRelativeLocation(FVector(X0, Y1 + 60.f, 200.f));
	P->SetIntensityUnits(ELightUnits::Lumens);
	P->SetIntensity(0.f);
	P->SetLightColor(D.Mau);
	P->SetAttenuationRadius(500.f);
	P->RegisterComponent();
	D.Bong = P;
	Den.Add(D);
}

void AHanhLang::DungSanTrong()
{
	// Gờ tường bên ngoài cửa sổ đoạn A (chỗ "người" có thể đứng), mặt đất xa bên dưới, tòa nhà đối diện
	FXay X;
	X.SU = X.SV = 300.f;
	X.Hop(FVector(-20.f, NuaRong + 20.f, -25.f), FVector(1780.f, NuaRong + 110.f, 0.f));
	X.Hop(FVector(-800.f, -200.f, -1450.f), FVector(2800.f, 2400.f, -1400.f));
	// Mặt ngoài tường hành lang (cho đẹp khi nhìn chéo qua sân)
	X.Hop(FVector(-20.f, NuaRong + 20.f, -1400.f), FVector(1780.f, NuaRong + 40.f, -25.f));
	X.Hop(FVector(1760.f, NuaRong + 20.f, -1400.f), FVector(1780.f, CuoiB + 300.f, 0.f));
	X.Hop(FVector(1760.f, NuaRong + 20.f, Cao + 10.f), FVector(1780.f, CuoiB + 300.f, Cao + 400.f));
	X.Hop(FVector(-20.f, NuaRong + 20.f, Cao + 10.f), FVector(1780.f, NuaRong + 40.f, Cao + 400.f));
	X.Xuat(LuoiKhac, VatLieu::BeMat(this, TEXT("be_tong"), FLinearColor(0.45f, 0.45f, 0.47f), 0.9f));
	// Tòa nhà đối diện: mặt tiền có ô cửa sáng
	UMaterialInterface* MTien = VatLieu::BeMat(this, TEXT("mat_tien"), FLinearColor::White, 0.9f, 12.f);
	UStaticMeshComponent* T1 = Khoi(FVector(700.f, 2100.f, -600.f), FVector(3000.f, 20.f, 1700.f), MTien);
	UStaticMeshComponent* T2 = Khoi(FVector(-700.f, 1000.f, -600.f), FVector(20.f, 2400.f, 1700.f), MTien, FRotator(0.f, 0.f, 0.f));
	T1->SetCastShadow(false); T2->SetCastShadow(false);
	// Nền trời sau tòa nhà (đêm gần đen, sáng sớm xanh nhạt)
	MatTroiNen = VatLieu::Sang(this, FLinearColor(0.02f, 0.025f, 0.04f), 1.f);
	for (const FVector& T : { FVector(700.f, 2600.f, 1000.f), FVector(-1200.f, 1000.f, 1000.f) })
	{
		UStaticMeshComponent* N = Khoi(T, T.Y > 2000.f ? FVector(6000.f, 10.f, 5000.f) : FVector(10.f, 6000.f, 5000.f), MatTroiNen);
		N->SetCastShadow(false);
		N->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	UStaticMeshComponent* Nap = Khoi(FVector(700.f, 1000.f, 2600.f), FVector(6000.f, 6000.f, 10.f), MatTroiNen);
	Nap->SetCastShadow(false);
	Nap->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Mưa: vệt mảnh rơi trong sân
	Mua = NewObject<UInstancedStaticMeshComponent>(this, TEXT("Mua"));
	Mua->SetStaticMesh(VatLieu::Khoi());
	Mua->SetMaterial(0, VatLieu::PhuMo(this, FLinearColor(0.35f, 0.4f, 0.45f), 0.22f));
	Mua->SetupAttachment(RootComponent);
	Mua->SetCastShadow(false);
	Mua->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mua->RegisterComponent();
	FRandomStream R(11);
	for (int32 K = 0; K < 900; ++K)
	{
		const FVector P(R.FRandRange(-300.f, 1760.f), R.FRandRange(NuaRong + 120.f, 1900.f), R.FRandRange(-500.f, 700.f));
		ViTriMua.Add(P);
		Mua->AddInstance(FTransform(FRotator::ZeroRotator, P, FVector(0.003f, 0.003f, 0.45f)));
	}
}

void AHanhLang::DungDoDat()
{
	// --- Đoạn A: bên trái (Y-) là phòng bệnh, bên phải (Y+) cửa sổ ---
	Mau(TEXT("BinhCuuHoa"), TEXT("fireExtinguisher_abc_a03"), FVector(160.f, -NuaRong + 12.f, 0.f), 90.f);
	Mau(TEXT("ThungRac"), TEXT("trashCan_office_gray_a03"), FVector(520.f, -NuaRong + 22.f, 0.f), 0.f);
	Mau(TEXT("Ghe1"), TEXT("chair_hospital_b01"), FVector(640.f, -NuaRong + 40.f, 0.f), 90.f);
	Mau(TEXT("Ghe2"), TEXT("chair_hospital_b01"), FVector(720.f, -NuaRong + 40.f, 0.f), 90.f);
	Mau(TEXT("Ghe3"), TEXT("chair_hospital_b01"), FVector(800.f, -NuaRong + 40.f, 0.f), 90.f);
	Mau(TEXT("CotTruyen"), TEXT("IV_pole_a01"), FVector(1150.f, -NuaRong + 34.f, 0.f), 0.f);
	Mau(TEXT("TuThuoc"), TEXT("cabinet_for_drugs_a01"), FVector(1260.f, -NuaRong + 41.f, 0.f), 90.f);
	Mau(TEXT("BienSanUot"), TEXT("sign_wetFloor_a01"), FVector(1300.f, 50.f, 0.f), 30.f);
	Mau(TEXT("XeVeSinh"), TEXT("trolley_cleaning_a01"), FVector(900.f, NuaRong - 40.f, 0.f), 0.f);
	Mau(TEXT("XeLan"), TEXT("wheelChair_c01"), FVector(1660.f, NuaRong - 45.f, 0.f), -90.f);
	// Rèm lá sập xuống che cửa sổ (ẩn sẵn, vòng 5 mới kéo xuống): dựng bằng các thanh lá mỏng
	UMaterialInterface* MLa = VatLieu::Mau(this, FLinearColor(0.72f, 0.7f, 0.64f), 0.55f);
	int32 K = 0;
	for (float S : { 300.f, 700.f, 1100.f, 1500.f })
	{
		USceneComponent* Rem = NewObject<USceneComponent>(this, FName(*FString::Printf(TEXT("Rem%d"), K)));
		Rem->SetupAttachment(RootComponent);
		Rem->SetRelativeLocation(FVector(S, NuaRong - 4.f, 0.f));
		Rem->RegisterComponent();
		for (int32 L = 0; L < 24; ++L)
		{
			UStaticMeshComponent* La = Khoi(FVector(0.f, 0.f, 236.f - L * 6.f), FVector(166.f, 5.f, 0.6f), MLa, FRotator(0.f, 0.f, 70.f), Rem);
			La->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		Khoi(FVector(0.f, 0.f, 240.f), FVector(170.f, 6.f, 5.f), MLa, FRotator::ZeroRotator, Rem)->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Rem->SetVisibility(false, true);
		FVatDung VR; VR.C = Rem; VR.Goc = Rem->GetRelativeTransform(); VR.bHienGoc = false;
		VatDung.Add(FName(*FString::Printf(TEXT("Rem%d"), K++)), VR);
	}

	// --- Góc rẽ: quầy trực, điện thoại, đồng hồ, TV ---
	UMaterialInterface* MQuay = VatLieu::Mau(this, FLinearColor(0.62f, 0.55f, 0.45f), 0.5f);
	UMaterialInterface* MMatQuay = VatLieu::Mau(this, FLinearColor(0.85f, 0.85f, 0.82f), 0.3f);
	Khoi(FVector(2005.f, -60.f, 50.f), FVector(60.f, 110.f, 100.f), MQuay);
	Khoi(FVector(2003.f, -60.f, 102.f), FVector(66.f, 114.f, 4.f), MMatQuay);
	Mau(TEXT("DienThoai"), TEXT("phone_reception_b01"), FVector(2010.f, -85.f, 104.f), 90.f, 1.6f);
	// Đèn báo cuộc gọi: chấm đỏ trên máy + ánh đỏ hắt lên quầy (nhấp nháy khi reo)
	MatDenDT = VatLieu::Sang(this, FLinearColor(1.f, 0.08f, 0.05f), 0.f);
	UStaticMeshComponent* Led = Khoi(FVector(2004.f, -85.f, 123.f), FVector(4.f, 4.f, 2.f), MatDenDT);
	Led->SetStaticMesh(VatLieu::Cau());
	Led->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Led->SetCastShadow(false);
	DenDienThoai = NewObject<UPointLightComponent>(this, TEXT("DenDienThoai"));
	DenDienThoai->SetupAttachment(RootComponent);
	DenDienThoai->SetRelativeLocation(FVector(1990.f, -85.f, 135.f));
	DenDienThoai->SetIntensityUnits(ELightUnits::Lumens);
	DenDienThoai->SetIntensity(0.f);
	DenDienThoai->SetLightColor(FLinearColor(1.f, 0.12f, 0.08f));
	DenDienThoai->SetAttenuationRadius(320.f);
	DenDienThoai->SetCastShadows(false);
	DenDienThoai->RegisterComponent();
	Mau(TEXT("TV"), TEXT("TV_a01"), FVector(1860.f, -NuaRong + 12.f, 170.f), -90.f);
	// Màn nhiễu đè lên TV (ẩn sẵn)
	MatTV = VatLieu::NhieuTV(this);
	ManTV = Khoi(FVector(1860.f, -NuaRong + 12.5f, 207.f), FVector(112.f, 0.5f, 62.f), MatTV);
	ManTV->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ManTV->SetCastShadow(false);
	FVatDung VTV; VTV.C = ManTV; VTV.Goc = ManTV->GetRelativeTransform(); VTV.bHienGoc = false;
	VatDung.Add(TEXT("ManTV"), VTV);

	// Đồng hồ treo tường (mặt phẳng hướng -X, trên tường cuối đoạn A)
	UStaticMeshComponent* Mat = Khoi(FVector(2038.f, 60.f, 205.f), FVector(1.f, 34.f, 34.f), VatLieu::BeMat(this, TEXT("mat_dong_ho"), FLinearColor::White, 0.4f, 0.5f));
	Mat->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UMaterialInterface* MKim = VatLieu::Mau(this, FLinearColor(0.02f, 0.02f, 0.02f), 0.5f);
	KimGio = NewObject<USceneComponent>(this);
	KimGio->SetupAttachment(RootComponent);
	KimGio->SetRelativeLocation(FVector(2036.f, 60.f, 205.f));
	KimGio->RegisterComponent();
	KimPhut = NewObject<USceneComponent>(this);
	KimPhut->SetupAttachment(RootComponent);
	KimPhut->SetRelativeLocation(FVector(2035.f, 60.f, 205.f));
	KimPhut->RegisterComponent();
	Khoi(FVector(0.f, 0.f, 4.5f), FVector(0.6f, 1.6f, 9.f), MKim, FRotator::ZeroRotator, KimGio)->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Khoi(FVector(0.f, 0.f, 6.5f), FVector(0.6f, 1.f, 13.f), MKim, FRotator::ZeroRotator, KimPhut)->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// --- Đoạn B ---
	Mau(TEXT("GheSofa"), TEXT("sofa_visitor_a01"), FVector(1985.f, 280.f, 0.f), 180.f);
	Mau(TEXT("CotTruyen2"), TEXT("IV_pole_a01"), FVector(1520.f, -400.f, 0.f), 40.f);   // trong phòng 403
	Mau(TEXT("XeDungCu"), TEXT("cart_medical_utility_a01"), FVector(2000.f, 880.f, 0.f), 180.f);
	Mau(TEXT("XeCapCuu"), TEXT("trolley_emergency_a01"), FVector(1840.f, 1000.f, 0.f), 0.f);
	Mau(TEXT("BanDauGiuong"), TEXT("table_bedside_b01"), FVector(2005.f, 1100.f, 0.f), 180.f);
	Mau(TEXT("BienCam"), TEXT("sign_hospital_doNotEnter_a01"), FVector(2036.f, 750.f, 150.f), 180.f);
	// Giường cáng có xác phủ khăn (ẩn sẵn)
	Mau(TEXT("GiuongCang"), TEXT("bed_hospital_standard_a01"), FVector(1856.f, 668.f, 0.f), 90.f, 1.f, false);
	UStaticMeshComponent* Xac = Khoi(FVector(1856.f, 668.f, 76.f), FVector(55.f, 175.f, 26.f), VatLieu::Mau(this, FLinearColor(0.8f, 0.82f, 0.8f), 0.85f));
	Xac->SetStaticMesh(VatLieu::Cau());
	Xac->SetRelativeScale3D(FVector(0.48f, 1.65f, 0.24f));
	FVatDung VX; VX.C = Xac; VX.Goc = Xac->GetRelativeTransform(); VX.bHienGoc = false;
	VatDung.Add(TEXT("XacPhuKhan"), VX);
	Xac->SetVisibility(false);
	Xac->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Mặt nạ rơi trên sàn (vòng cuối), vết máu, chữ trên tường
	Mau(TEXT("MatNaSan"), TEXT("misc_toy_phantomMask_a01"), FVector(GocX - 30.f, 1100.f, 4.f), 30.f, 1.3f, false);
	Dat(TEXT("MatNaSan"), FTransform(FRotator(-85.f, 30.f, 0.f), FVector(GocX - 30.f, 1100.f, 4.f), FVector(1.3f)));
	VatDung[TEXT("MatNaSan")].Goc = VatDung[TEXT("MatNaSan")].C->GetRelativeTransform();
	Vet(TEXT("ChuMau"), TEXT("chu_mau"), FVector(1250.f, NuaRong + 2.f, 160.f), FRotator(0.f, 90.f, 0.f), FVector(20.f, 110.f, 110.f));
	Vet(TEXT("DauTay1"), TEXT("dau_tay"), FVector(700.f, NuaRong + 12.f, 150.f), FRotator(0.f, 90.f, 0.f), FVector(20.f, 22.f, 22.f));
	Vet(TEXT("DauTay2"), TEXT("dau_tay"), FVector(1500.f, NuaRong + 12.f, 140.f), FRotator(0.f, 90.f, 10.f), FVector(20.f, 22.f, 22.f));
	Vet(TEXT("DauTay3"), TEXT("dau_tay"), FVector(1450.f - 40.f, -NuaRong - 2.f, 120.f), FRotator(0.f, -90.f, -15.f), FVector(20.f, 22.f, 22.f));
	Vet(TEXT("VetMau1"), TEXT("vet_mau"), FVector(1300.f, -40.f, 0.f), FRotator(-90.f, 0.f, -60.f), FVector(20.f, 120.f, 60.f));
	Vet(TEXT("VetMau2"), TEXT("vet_mau"), FVector(1430.f, -100.f, 0.f), FRotator(-90.f, 0.f, -85.f), FVector(20.f, 90.f, 50.f));
	Vet(TEXT("VetCao"), TEXT("vet_cao"), FVector(GocX + 118.f, 900.f, 150.f), FRotator(0.f, 0.f, 0.f), FVector(20.f, 60.f, 60.f));
	Vet(TEXT("VungNuoc"), TEXT("vung_nuoc"), FVector(1300.f, 20.f, 0.f), FRotator(-90.f, 0.f, 0.f), FVector(20.f, 90.f, 90.f), true);

	DungDoThem();

	// Đồ đẩy được (kg): ghế, xe, cột truyền, thùng rác, biển báo...
	// (cáng phải gắn xác trước khi bọc hộp vật lý, nên đặt cáng sau cùng trong danh sách)
	const TPair<const TCHAR*, float> Day[] = {
		{ TEXT("Ghe1"), 22.f }, { TEXT("Ghe2"), 22.f }, { TEXT("Ghe3"), 22.f }, { TEXT("XeLan"), 18.f }, { TEXT("ThungRac"), 5.f },
		{ TEXT("BienSanUot"), 3.f }, { TEXT("XeVeSinh"), 25.f }, { TEXT("CotTruyen"), 8.f }, { TEXT("CotTruyen2"), 8.f },
		{ TEXT("XeDungCu"), 30.f }, { TEXT("XeCapCuu"), 60.f }, { TEXT("BanDauGiuong"), 20.f },
		{ TEXT("GheDau"), 8.f }, { TEXT("ThungRacXanh"), 15.f }, { TEXT("XeDungCuNho"), 18.f }, { TEXT("ThungRacLon"), 8.f },
		{ TEXT("BienCham"), 6.f }, { TEXT("GheBenhNhan"), 35.f }, { TEXT("XeMayTinh"), 25.f }, { TEXT("GheBanh"), 25.f },
		{ TEXT("CotTruyen3"), 8.f }, { TEXT("BinhCuuHoa2"), 6.f }, { TEXT("BinhCuuHoa3"), 6.f },
		{ TEXT("GiuongCang"), 70.f }, { TEXT("TuThuoc"), 40.f }, { TEXT("TuKhoa"), 55.f }, { TEXT("TuHoSo"), 40.f },
		{ TEXT("GheSofa"), 40.f }, { TEXT("BinhCuuHoa"), 6.f }, { TEXT("P402_Ghe"), 25.f }, { TEXT("P403_ChauRua"), 12.f },
		{ TEXT("Phong1450_May"), 25.f } };
	// "Xác" phủ khăn nằm trên cáng: gắn vào cáng để kéo cáng thì xác theo cùng
	if (FVatDung* VXac = VatDung.Find(TEXT("XacPhuKhan")))
		if (FVatDung* Cang = VatDung.Find(TEXT("GiuongCang")))
		{
			VXac->C->AttachToComponent(Cang->C, FAttachmentTransformRules::KeepWorldTransform);
			VatDung.Remove(TEXT("XacPhuKhan"));
		}
	for (const auto& D : Day) DayDuoc(D.Key, D.Value);
}

void AHanhLang::DungDoThem()
{
	// Đồ thêm cho đầy không gian (kích thước đo từ mẫu: xem Saved/kich_thuoc_mau.txt)
	// Đoạn A, bên trái (phòng bệnh)
	Mau(TEXT("ThungRacLon"), TEXT("bin_waste_a01"), FVector(255.f, -NuaRong + 12.f, 0.f), 0.f);
	Mau(TEXT("MayNuocRuaTay"), TEXT("dispenser_a01"), FVector(1085.f, -NuaRong, 105.f), 90.f);
	Mau(TEXT("TuKhoa"), TEXT("facilityFurniture_lockerUnit_multiDoor_a01"), FVector(1560.f, -NuaRong, 0.f), 90.f);
	Mau(TEXT("BinhCuuHoa2"), TEXT("fireProtection_extinguisher_portable_a01"), FVector(1710.f, -NuaRong, 0.f), 90.f);
	// Đoạn A, bên phải (dưới cửa sổ)
	Mau(TEXT("GheDau"), TEXT("stool_mobile_padded_vinyl_a01"), FVector(560.f, NuaRong - 40.f, 0.f), 20.f);
	Mau(TEXT("GheBanh"), TEXT("sofa_soft_b01"), FVector(1215.f, NuaRong - 40.f, 0.f), -90.f);
	Mau(TEXT("XeDungCuNho"), TEXT("trolley_instrument_b01"), FVector(1420.f, NuaRong - 34.f, 0.f), 0.f);
	Mau(TEXT("ThungRacXanh"), TEXT("trashCan_wheeled_green_a01"), FVector(480.f, -380.f, 0.f), 30.f);   // trong phòng 401
	// Quầy trực
	Mau(TEXT("DenBan"), TEXT("lamp_table_bedside_a01"), FVector(2012.f, -22.f, 104.f), 200.f);
	// Đoạn B
	Mau(TEXT("GheBenhNhan"), TEXT("chair_patient_mobile_a01"), FVector(330.f, NuaRong - 45.f, 0.f), 90.f);
	Mau(TEXT("TuHoSo"), TEXT("cabinet_file_a01"), FVector(2018.f, 640.f, 0.f), 0.f);
	Mau(TEXT("XeMayTinh"), TEXT("cart_computer_a01"), FVector(1842.f, 830.f, 0.f), 90.f);
	Mau(TEXT("BienCham"), TEXT("sign_floor_movable_a01"), FVector(2008.f, 955.f, 0.f), 90.f);
	Mau(TEXT("CotTruyen3"), TEXT("IV_pole_b01"), FVector(1845.f, 1130.f, 0.f), 0.f);
	Mau(TEXT("BinhCuuHoa3"), TEXT("fireProtection_extinguisher_portable_a01"), FVector(2040.f, 1180.f, 0.f), 180.f);
	// Phòng 402 (sau cửa, chỉ thấy khi cửa hé) và phòng 403
	Mau(TEXT("P402_Ghe"), TEXT("chair_hospital_a01"), FVector(1060.f, -360.f, 0.f), 150.f);
	Mau(TEXT("P402_BonRua"), TEXT("fixture_sink_basin_a01"), FVector(900.f, -403.f, 78.f), 0.f);
	Khoi(FVector(900.f, -403.f, 39.f), FVector(52.f, 74.f, 78.f), VatLieu::Mau(this, FLinearColor(0.75f, 0.74f, 0.7f), 0.5f));
	Mau(TEXT("P403_Khan"), TEXT("bedding_a01"), FVector(1410.f, -320.f, 48.f), 0.f);
	Mau(TEXT("P403_ChauRua"), TEXT("washBasin_a01"), FVector(1590.f, -410.f, 0.f), 180.f);
}

void AHanhLang::DungAnhSang()
{
	for (float S : { 150.f, 450.f, 750.f, 1050.f, 1350.f, 1650.f }) DenTran(FVector(S, 0.f, 0.f), 0.f, TEXT("A"));
	DenTran(FVector(GocX, 0.f, 0.f), 90.f, TEXT("Goc"));
	for (float S : { 320.f, 620.f, 920.f, 1150.f }) DenTran(FVector(GocX, S, 0.f), 90.f, TEXT("B"));

	// Trời đêm (ánh trăng xanh rất yếu) + nắng sáng (kết thúc, tắt sẵn)
	TroiDem = NewObject<UDirectionalLightComponent>(this, TEXT("TroiDem"));
	TroiDem->SetupAttachment(RootComponent);
	TroiDem->SetRelativeRotation(FRotator(-30.f, -45.f, 0.f));
	TroiDem->SetIntensity(0.4f);
	TroiDem->SetLightColor(FLinearColor(0.55f, 0.65f, 1.f));
	TroiDem->RegisterComponent();
	MatTroi = NewObject<UDirectionalLightComponent>(this, TEXT("MatTroi"));
	MatTroi->SetupAttachment(RootComponent);
	MatTroi->SetRelativeRotation(FRotator(-18.f, -100.f, 0.f));
	MatTroi->SetIntensity(0.f);
	MatTroi->SetLightColor(FLinearColor(1.f, 0.85f, 0.65f));
	MatTroi->SetVisibility(false);
	MatTroi->RegisterComponent();
	AnhTroi = NewObject<USkyLightComponent>(this, TEXT("AnhTroi"));
	AnhTroi->SetupAttachment(RootComponent);
	AnhTroi->SourceType = ESkyLightSourceType::SLS_SpecifiedCubemap;
	AnhTroi->SetCubemap(LoadObject<UTextureCube>(nullptr, TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap")));
	AnhTroi->SetIntensity(1.f);
	AnhTroi->SetLightColor(FLinearColor(0.4f, 0.5f, 0.7f));
	AnhTroi->bLowerHemisphereIsBlack = false;
	AnhTroi->RegisterComponent();
	AnhTroi->RecaptureSky();

	HauKy = GetWorld()->SpawnActor<APostProcessVolume>();
	HauKy->bUnbound = true;
	ActorCon.Add(HauKy);
}

void AHanhLang::DatViTri(FVatDung& V, const FTransform& T, bool bHien)
{
	if (!V.bDay)
	{
		V.C->SetRelativeTransform(T);
		return;
	}
	// T là vị trí của lưới; hộp va chạm đặt ở tâm lưới. Đồ đang mô phỏng đã tách khỏi actor (actor ở gốc tọa độ)
	UPrimitiveComponent* P = CastChecked<UPrimitiveComponent>(V.C);
	P->SetSimulatePhysics(false);
	P->SetWorldTransform(FTransform(T.GetRotation(), T.TransformPosition(V.LechTam)), false, nullptr, ETeleportType::ResetPhysics);
	if (bHien)
	{
		P->SetSimulatePhysics(true);
		P->SetPhysicsLinearVelocity(FVector::ZeroVector);
		P->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}
}

void AHanhLang::DayDuoc(FName Ten, float KhoiLuong)
{
	FVatDung* V = VatDung.Find(Ten);
	UStaticMeshComponent* C = V ? Cast<UStaticMeshComponent>(V->C) : nullptr;
	if (!C || !C->GetStaticMesh() || V->bDay) return;
	// Hộp va chạm bọc lưới (lưới SimReady chỉ có va chạm theo lưới, không mô phỏng được)
	const FBox Bao = C->GetStaticMesh()->GetBoundingBox();
	UBoxComponent* H = NewObject<UBoxComponent>(this, FName(*(Ten.ToString() + TEXT("_Hop"))));
	H->SetupAttachment(RootComponent);
	H->SetBoxExtent(Bao.GetExtent() * C->GetRelativeScale3D() * 0.95f);
	H->RegisterComponent();
	V->LechTam = Bao.GetCenter();
	H->SetWorldTransform(FTransform(C->GetComponentQuat(), C->GetComponentTransform().TransformPosition(Bao.GetCenter())));
	C->AttachToComponent(H, FAttachmentTransformRules::KeepWorldTransform);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	V->C = H;
	V->bDay = true;
	H->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	H->SetMassOverrideInKg(NAME_None, KhoiLuong, true);
	H->SetLinearDamping(2.5f);
	H->SetAngularDamping(5.f);
	if (FBodyInstance* B = H->GetBodyInstance())
	{
		B->bLockXRotation = true;
		B->bLockYRotation = true;
		B->SetDOFLock(EDOFMode::SixDOF);
	}
}

void AHanhLang::DatMuaKinh(float Co, float Chay)
{
	// Vệt nước trượt XUỐNG (UV cộng dương làm hình trôi lên) và mờ; giọt nhỏ đứng yên ở lớp riêng
	MatKinh->SetScalarParameterValue(TEXT("MuaCo"), Co * 0.55f);
	MatKinh->SetScalarParameterValue(TEXT("MuaChay"), -Chay * 0.35f);
	MatKinh->SetScalarParameterValue(TEXT("SangGiot"), 3.f);
	MatMuaKinh->SetScalarParameterValue(TEXT("DoMo"), Co * 0.9f);
}

void AHanhLang::DatAnhTayKinh(const FString& Anh)
{
	if (UTexture2D* T = VatLieu::Anh(Anh)) MatTayKinh->SetTextureParameterValue(TEXT("Anh"), T);
}

void AHanhLang::DatLai()
{
	for (auto& KV : VatDung)
	{
		FVatDung& V = KV.Value;
		V.C->SetVisibility(V.bHienGoc, true);
		DatViTri(V, V.Goc, V.bHienGoc);
		if (UPrimitiveComponent* P = Cast<UPrimitiveComponent>(V.C))
			if (P->GetCollisionEnabled() != ECollisionEnabled::NoCollision || V.bHienGoc)
				if (!(KV.Key.ToString().StartsWith(TEXT("TayKinh"))))
					P->SetCollisionEnabled(V.bHienGoc ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
	DatAnhTayKinh(TEXT("tay_kinh"));
	if (VatDung.Contains(TEXT("ManTV"))) ManTV->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	for (FDen& D : Den)
	{
		D.Muc = D.Nhom.StartsWith(TEXT("Phong")) ? 0.f : 1.f;
		D.NhapNhay = 0.f;
		D.TatNgan = 0.f;
	}
	for (auto& KV : Cua)
	{
		KV.Value->DatGoc(0.f);
		KV.Value->bKhoa = KV.Key != TEXT("Dau") && KV.Key != TEXT("Cuoi");
	}
	DatGio(3.0f);
}

void AHanhLang::Dat(FName Ten, const FVector& ViTri, float Yaw)
{
	if (FVatDung* V = VatDung.Find(Ten))
		DatViTri(*V, FTransform(FRotator(0.f, Yaw, 0.f), ViTri, V->C->GetComponentScale()), V->C->GetVisibleFlag());
}

void AHanhLang::Dat(FName Ten, const FTransform& T)
{
	if (FVatDung* V = VatDung.Find(Ten)) DatViTri(*V, T, V->C->GetVisibleFlag());
}

void AHanhLang::Hien(FName Ten, bool bHien)
{
	if (FVatDung* V = VatDung.Find(Ten))
	{
		V->C->SetVisibility(bHien, true);
		if (UPrimitiveComponent* P = Cast<UPrimitiveComponent>(V->C))
			if (P != ManTV && !Ten.ToString().StartsWith(TEXT("TayKinh")))
			{
				P->SetCollisionEnabled(bHien ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
				if (V->bDay) P->SetSimulatePhysics(bHien);
			}
	}
}

bool AHanhLang::KeoDuoc(const UPrimitiveComponent* C) const
{
	for (const auto& KV : VatDung)
		if (KV.Value.bDay && KV.Value.C == C) return true;
	return false;
}

USceneComponent* AHanhLang::Vat(FName Ten) const
{
	const FVatDung* V = VatDung.Find(Ten);
	return V ? V->C : nullptr;
}

void AHanhLang::DatGio(float Gio)
{
	const float Phut = FMath::Fmod(Gio, 1.f) * 60.f;
	// Nhìn từ phía -X: quay theo chiều kim đồng hồ = roll âm quanh trục X
	KimGio->SetRelativeRotation(FRotator(0.f, 0.f, -Gio * 30.f));
	KimPhut->SetRelativeRotation(FRotator(0.f, 0.f, -Phut * 6.f));
}

void AHanhLang::ChopSet(float Manh)
{
	Set = FMath::Max(Set, Manh);
}

void AHanhLang::DatMua(bool bBat)
{
	Mua->SetVisibility(bBat);
}

void AHanhLang::DatBinhMinh(bool bSang)
{
	MatTroi->SetVisibility(bSang);
	TroiDem->SetVisibility(!bSang);
	MatTroi->SetIntensity(bSang ? 80.f : 0.f);
	AnhTroi->SetLightColor(bSang ? FLinearColor(0.95f, 0.9f, 0.85f) : FLinearColor(0.5f, 0.6f, 0.8f));
	MatTroiNen->SetVectorParameterValue(TEXT("MauSac"), bSang ? FLinearColor(0.55f, 0.7f, 0.95f) : FLinearColor(0.02f, 0.025f, 0.04f));
	MatTroiNen->SetScalarParameterValue(TEXT("DoSang"), bSang ? 400.f : 1.f);
}

void AHanhLang::DatMoiTruong(float CuongDo)
{
	AnhTroi->SetIntensity(CuongDo);
	AnhTroi->RecaptureSky();
}

void AHanhLang::DatBienLoiRa(bool bLoiRa)
{
	if (UTexture2D* T = VatLieu::Anh(bLoiRa ? TEXT("bien_loi_ra") : TEXT("bien_loi_thoat"))) MatBienThoat->SetTextureParameterValue(TEXT("Anh"), T);
}

void AHanhLang::Tick(float Dt)
{
	Super::Tick(Dt);
	ThoiGian += Dt;

	// Đèn: độ sáng = gốc x mức đạo diễn đặt x chập chờn
	for (FDen& D : Den)
	{
		float H = D.Muc;
		if (D.NhapNhay > 0.f)
		{
			const float N = FMath::PerlinNoise1D(ThoiGian * 9.f + D.Hat);
			// NhapNhay càng lớn thì càng hay tắt (1 = tắt quá nửa thời gian)
			const bool bTat = N < D.NhapNhay * 0.9f - 0.65f;
			H *= bTat ? 0.04f : (0.75f + 0.25f * FMath::Sin(ThoiGian * 120.f + D.Hat));
		}
		if (D.TatNgan > 0.f)
		{
			D.TatNgan -= Dt;
			H = 0.f;
		}
		if (D.Den) D.Den->SetIntensity(D.CuongDo * H);
		if (D.Bong) D.Bong->SetIntensity(D.CuongDo * H);
		if (D.Phu) D.Phu->SetIntensity(D.CuongDoPhu * H);
		if (D.MatSang) D.MatSang->SetScalarParameterValue(TEXT("DoSang"), D.DoSangMat * H);
	}

	// Sét: ánh trời lóe sáng rồi tắt dần
	if (Set > 0.f)
	{
		Set = FMath::Max(0.f, Set - Dt * 2.5f);
		const float Chop = (FMath::Frac(ThoiGian * 13.f) > 0.4f ? 1.f : 0.5f) * Set;
		TroiDem->SetIntensity(0.4f + 300.f * Chop);
	}
	else if (TroiDem->IsVisible())
	{
		TroiDem->SetIntensity(0.4f);
	}

	// Mưa rơi
	if (Mua && Mua->IsVisible())
	{
		TArray<FTransform> DS;
		DS.Reserve(ViTriMua.Num());
		for (FVector& P : ViTriMua)
		{
			P.Z -= Dt * 950.f;
			P.Y -= Dt * 60.f;
			if (P.Z < -500.f) { P.Z += 1200.f; P.Y += 0.f; }
			if (P.Y < NuaRong + 120.f) P.Y += 1700.f;
			DS.Add(FTransform(FRotator::ZeroRotator, P, FVector(0.003f, 0.003f, 0.45f)));
		}
		Mua->BatchUpdateInstancesTransforms(0, DS, false, true);
	}
}
