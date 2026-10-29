#include "DaoDien.h"

#include "AmThanh.h"
#include "BanDo.h"
#include "BongNguoi.h"
#include "Components/AudioComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Cua.h"
#include "Engine/PostProcessVolume.h"
#include "EngineUtils.h"
#include "HanhLang.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NguoiChoi.h"
#include "SinhVat.h"

using namespace BanDo;

namespace
{
	const FVector ViTriDau(-235.f, 0.f, 92.f);
	const FVector ViTriDienThoai(2010.f, -85.f, 110.f);
	const FVector ViTriPhong403(1450.f, -300.f, 120.f);
	const FVector ViTriDongHo(2036.f, 60.f, 205.f);
	float DoToiDich = 0.f, DoTrangDich = 0.f, ThoiBiBat = -1.f;
}

ADaoDien::ADaoDien()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Goc"));
}

ADaoDien* ADaoDien::Lay(const UObject* Ngu)
{
	UWorld* W = Ngu ? Ngu->GetWorld() : nullptr;
	if (!W) return nullptr;
	for (TActorIterator<ADaoDien> It(W); It; ++It) return *It;
	return nullptr;
}

ANguoiChoi* ADaoDien::NC() const
{
	return Cast<ANguoiChoi>(UGameplayStatics::GetPlayerPawn(this, 0));
}

void ADaoDien::Luu(int32 V)
{
	FFileHelper::SaveStringToFile(FString::FromInt(V), *FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CaTrucDem.txt")));
}

FVector ADaoDien::ViTriDT() { return ViTriDienThoai; }

int32 ADaoDien::Doc()
{
	FString S;
	if (!FFileHelper::LoadFileToString(S, *FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CaTrucDem.txt")))) return 0;
	return FCString::Atoi(*S);
}

void ADaoDien::BeginPlay()
{
	Super::BeginPlay();
	HL = GetWorld()->SpawnActor<AHanhLang>();
	HL->Dung();
	Bong = GetWorld()->SpawnActor<ABongNguoi>();
	Nhen = GetWorld()->SpawnActor<ASinhVat>();
	Nhen->An();

	GocDongHo = NewObject<USceneComponent>(this);
	GocDongHo->SetupAttachment(RootComponent);
	GocDongHo->SetWorldLocation(ViTriDongHo);
	GocDongHo->RegisterComponent();
	GocDienThoai = NewObject<USceneComponent>(this);
	GocDienThoai->SetupAttachment(RootComponent);
	GocDienThoai->SetWorldLocation(ViTriDienThoai);
	GocDienThoai->RegisterComponent();
	GocPhong403 = NewObject<USceneComponent>(this);
	GocPhong403->SetupAttachment(RootComponent);
	GocPhong403->SetWorldLocation(ViTriPhong403);
	GocPhong403->RegisterComponent();

	AmDen = AmThanh::TaoVong(this, TEXT("nen_den_huynh_quang"), nullptr);
	AmMua = AmThanh::TaoVong(this, TEXT("nen_mua"), nullptr);
	AmU1 = AmThanh::TaoVong(this, TEXT("nen_u_am_1"), nullptr);
	AmU2 = AmThanh::TaoVong(this, TEXT("nen_u_am_2"), nullptr);
	AmTim = AmThanh::TaoVong(this, TEXT("nhip_tim"), nullptr);
	AmDuoi = AmThanh::TaoVong(this, TEXT("duoi_bat"), nullptr);
	AmBinhMinh = AmThanh::TaoVong(this, TEXT("binh_minh"), nullptr);
	AmDongHo = AmThanh::TaoVong(this, TEXT("dong_ho"), GocDongHo, 900.f);
	AmChuong = AmThanh::TaoVong(this, TEXT("dien_thoai_reo"), GocDienThoai, 3500.f);
	AmMayThang = AmThanh::TaoVong(this, TEXT("may_tim_thang"), GocPhong403, 1500.f);

	SeedLanChoi = FMath::Rand();
	// Nền menu: đứng đầu hành lang, vòng 1
	ApDung(1);
	TrangThai = ETrangThai::Menu;
	DoToi = DoToiDich = 0.f;
}

void ADaoDien::BatDau(int32 V)
{
	Vong = FMath::Clamp(V, 1, VongCuoiKinhDi + 1);
	TrangThai = ETrangThai::Choi;
	UGameplayStatics::SetGamePaused(this, false);
	if (ANguoiChoi* P = NC())
	{
		P->SetActorLocation(ViTriDau, false, nullptr, ETeleportType::TeleportPhysics);
		if (P->Controller) P->Controller->SetControlRotation(FRotator(0.f, 0.f, 0.f));
		P->bKhoa = false;
	}
	ApDung(Vong);
	DoToi = 1.f;
	DoToiDich = 0.f;
	if (Vong == 1)
	{
		Hen(1.2f, [this]() { NoiPhuDe(TEXT("03:00. Night shift. Intensive Care, 4th floor."), 4.f); });
		Hen(5.5f, [this]() { NoiPhuDe(TEXT("Check the corridor, then take the stairs at the end of the ward."), 4.f); });
	}
}

void ADaoDien::VeMenu()
{
	TrangThai = ETrangThai::Menu;
	UGameplayStatics::SetGamePaused(this, false);
	bDangDuoi = false;
	if (ANguoiChoi* P = NC())
	{
		P->SetActorLocation(FVector(80.f, 0.f, 92.f), false, nullptr, ETeleportType::TeleportPhysics);
		if (P->Controller) P->Controller->SetControlRotation(FRotator(-3.f, 0.f, 0.f));
	}
	ApDung(1);
	AmThanh::DatAm(AmDuoi, 0.f, 0.5f);
	DoTrang = DoTrangDich = 0.f;
	DoToi = 1.f;
	DoToiDich = 0.f;
}

void ADaoDien::TamDung(bool bDung)
{
	if (TrangThai != ETrangThai::Choi && TrangThai != ETrangThai::TamDung) return;
	TrangThai = bDung ? ETrangThai::TamDung : ETrangThai::Choi;
	UGameplayStatics::SetGamePaused(this, bDung);
}

void ADaoDien::KhiQua(float S, TFunction<void()> Lam)
{
	FSuKien E;
	E.S = S;
	E.Lam = MoveTemp(Lam);
	SuKien.Add(MoveTemp(E));
}

void ADaoDien::Hen(float Giay, TFunction<void()> Lam)
{
	FSuKien E;
	E.S = -10000.f;
	E.Tre = ThoiVong + Giay;
	E.Lam = MoveTemp(Lam);
	SuKien.Add(MoveTemp(E));
}

void ADaoDien::NoiPhuDe(const FString& Chu, float Giay)
{
	PhuDe = Chu;
	ThoiPhuDe = Giay;
}

void ADaoDien::DatNen(float Den, float Mua, float UAm1, float UAm2, float Tim)
{
	AmThanh::DatAm(AmDen, Den, 1.5f);
	AmThanh::DatAm(AmMua, Mua, 1.5f);
	AmThanh::DatAm(AmU1, UAm1, 3.f);
	AmThanh::DatAm(AmU2, UAm2, 3.f);
	AmThanh::DatAm(AmTim, Tim, 2.f);
}

void ADaoDien::DenNhom(const FString& Nhom, float Muc, float NhapNhay, float TatNgan)
{
	for (FDen& D : HL->Den)
	{
		const bool bHop = Nhom == TEXT("*") ? (D.Nhom == TEXT("A") || D.Nhom == TEXT("Goc") || D.Nhom == TEXT("B")) : D.Nhom == Nhom;
		if (!bHop) continue;
		if (Muc >= 0.f) D.Muc = Muc;
		D.NhapNhay = NhapNhay;
		D.TatNgan = FMath::Max(D.TatNgan, TatNgan);
	}
}

void ADaoDien::MauDenNhom(const FString& Nhom, const FLinearColor& C)
{
	for (FDen& D : HL->Den)
	{
		const bool bHop = Nhom == TEXT("*") ? (D.Nhom == TEXT("A") || D.Nhom == TEXT("Goc") || D.Nhom == TEXT("B")) : D.Nhom == Nhom;
		if (!bHop) continue;
		if (D.Den) D.Den->SetLightColor(C);
		if (D.MatSang) D.MatSang->SetVectorParameterValue(TEXT("MauSac"), C);
	}
}

void ADaoDien::DatHauKy()
{
	FPostProcessSettings& P = HL->HauKy->Settings;
	const float M = FMath::Clamp((Vong - 1) / float(VongCuoiKinhDi - 1), 0.f, 1.f);
	const bool bSang = Vong > VongCuoiKinhDi;
	P.bOverride_AutoExposureMethod = true;
	P.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
	P.bOverride_AutoExposureMinBrightness = P.bOverride_AutoExposureMaxBrightness = true;
	P.AutoExposureMinBrightness = P.AutoExposureMaxBrightness = bSang ? 5.2f : 8.8f - 1.6f * M;
	P.bOverride_AutoExposureBias = true;
	P.AutoExposureBias = 0.f;
	P.bOverride_VignetteIntensity = true;
	P.VignetteIntensity = bSang ? 0.3f : 0.45f + 0.5f * M;
	P.bOverride_FilmGrainIntensity = true;
	P.FilmGrainIntensity = bSang ? 0.1f : 0.18f + 0.35f * M;
	P.bOverride_SceneFringeIntensity = true;
	P.SceneFringeIntensity = bSang ? 0.f : FMath::Max(0.f, M - 0.5f) * 2.5f;
	P.bOverride_ColorSaturation = true;
	const float Bao = bSang ? 1.05f : 0.85f - 0.3f * M;
	P.ColorSaturation = FVector4(Bao, Bao, Bao, 1.f);
	P.bOverride_ColorGain = true;
	P.ColorGain = bSang ? FVector4(1.f, 1.f, 1.f, 1.f) : FVector4(1.f + 0.12f * M, 1.f - 0.05f * M, 1.f - 0.08f * M, 1.f);
	P.bOverride_LensFlareIntensity = true;
	P.LensFlareIntensity = 0.f;
	P.bOverride_BloomIntensity = true;
	P.BloomIntensity = 0.6f;
	P.bOverride_MotionBlurAmount = true;
	P.MotionBlurAmount = 0.f;
}

bool ADaoDien::NhinVao(const FVector& P, float Cos) const
{
	const ANguoiChoi* N = NC();
	if (!N) return false;
	return FVector::DotProduct(N->HuongNhin(), (P - N->ViTriMat()).GetSafeNormal()) > Cos;
}

UPrimitiveComponent* ADaoDien::VatDangNhin(float Tam) const
{
	const ANguoiChoi* N = NC();
	if (!N) return nullptr;
	FHitResult H;
	FCollisionQueryParams Q;
	Q.AddIgnoredActor(N);
	const FVector A = N->ViTriMat();
	if (GetWorld()->LineTraceSingleByChannel(H, A, A + N->HuongNhin() * Tam, ECC_Visibility, Q)) return H.GetComponent();
	return nullptr;
}

ACua* ADaoDien::CuaDangNhin() const
{
	UPrimitiveComponent* C = VatDangNhin();
	return C ? Cast<ACua>(C->GetOwner()) : nullptr;
}

void ADaoDien::TuongTac()
{
	if (TrangThai != ETrangThai::Choi) return;
	ANguoiChoi* N = NC();
	if (!N || N->bKhoa) return;
	// Nghe điện thoại (rộng tay: đứng gần + nhìn về phía máy)
	if (bDienThoaiReo && FVector::Dist(N->ViTriMat(), ViTriDienThoai) < 260.f && NhinVao(ViTriDienThoai, 0.6f))
	{
		bDienThoaiReo = false;
		bDaNghe = true;
		AmThanh::DatAm(AmChuong, 0.f, 0.05f);
		// Giọng nữ hoảng sợ trong máy (Piper TTS, xem nguon/_scripts/tao_giong_noi.py); phụ đề khớp mốc thời gian của file
		AmThanh::Phat2D(this, TEXT("dt_giong"), 1.f);
		AmThanh::Phat3D(this, TEXT("nhieu_dien_thoai"), ViTriDienThoai, 0.35f);
		Hen(0.7f, [this]() { NoiPhuDe(TEXT("\"Hello? Hello?! Oh God... someone's there.\""), 3.f); });
		Hen(3.8f, [this]() { NoiPhuDe(TEXT("\"I'm in room four-oh-three. I can't get out... she's in here with me.\""), 4.4f); });
		Hen(8.4f, [this]() { NoiPhuDe(TEXT("\"Don't come in. Whatever you do... don't look back.\""), 3.f); });
		Hen(11.2f, [this]() { NoiPhuDe(TEXT("(The line goes dead.)"), 2.f); AmThanh::Phat3D(this, TEXT("cua_dong"), ViTriDienThoai, 0.5f, 1.4f); });
		return;
	}
	ACua* C = CuaDangNhin();
	if (!C) return;
	if (C->Ten == TEXT("Cuoi") && TrongBuongCuoi(N->GetActorLocation()))
	{
		C->Rung();
		NoiPhuDe(TEXT("The door has latched shut."), 2.f);
		return;
	}
	if (C->bKhoa)
	{
		C->Rung();
		NoiPhuDe(C->Ten.StartsWith(TEXT("Gia")) ? TEXT("It's locked from the other side.") : TEXT("The ward door is locked."), 2.f);
		return;
	}
	if (C->DaDong()) C->Mo();
}

void ADaoDien::NamVat(bool bGiu)
{
	ANguoiChoi* N = NC();
	if (!N) return;
	if (!bGiu || TrangThai != ETrangThai::Choi || N->bKhoa) { if (N->DangNam()) N->Tha(); return; }
	if (N->DangNam()) return;
	FHitResult H;
	FCollisionQueryParams Q;
	Q.AddIgnoredActor(N);
	const FVector A = N->ViTriMat();
	if (GetWorld()->LineTraceSingleByChannel(H, A, A + N->HuongNhin() * 260.f, ECC_Visibility, Q) && HL->KeoDuoc(H.GetComponent()))
		N->Nam(H.GetComponent(), H.ImpactPoint);
}

void ADaoDien::ApDung(int32 V)
{
	if (ANguoiChoi* N0 = NC()) if (N0->DangNam()) N0->Tha();
	SuKien.Reset();
	ThoiVong = 0.f;
	HL->DatLai();
	Bong->An();
	bTitPhong = bGiotNuoc = bQuayKim = bBongSauLung = bBongNgoaiCuaSo = bAnCangKhiQua = bBongTienLai = false;
	bLiec = bDoaSet = bCoGiatBenKia = bChoBo = false;
	SoCuHu = 0;
	if (Nhen) Nhen->An();
	bLuatNhinLai = V >= 8 && V <= 11;
	SoLanNhinLai = 0;
	bChoQuayDi = false;
	ThoiNhinLai = ThoiThayCoGiat = 0.f;
	bTienGiat = false;
	bDienThoaiReo = bDaNghe = bDangDuoi = false;
	NhinBong = 0.f;
	AmThanh::DatAm(AmChuong, 0.f, 0.1f);
	AmThanh::DatAm(AmMayThang, 0.f, 0.1f);
	AmThanh::DatAm(AmDuoi, 0.f, 0.5f);
	AmThanh::DatAm(AmBinhMinh, 0.f, 0.5f);
	AmThanh::DatAm(AmDongHo, 0.5f, 0.5f);
	MauDenNhom(TEXT("*"), FLinearColor(0.88f, 0.97f, 1.f));
	HL->DatBinhMinh(false);
	HL->DatBienLoiRa(false);
	ChoSet = FMath::FRandRange(6.f, 12.f);

	const float M = FMath::Clamp((V - 1) / float(VongCuoiKinhDi - 1), 0.f, 1.f);
	DatNen(V >= 12 ? 0.f : (V >= 10 ? 0.2f : 0.35f), V >= 12 ? 0.25f : 0.5f,
		V >= 3 ? FMath::Lerp(0.2f, 0.45f, M) : 0.f, V >= 8 ? FMath::Lerp(0.15f, 0.55f, (V - 8) / 4.f) : 0.f, V >= 10 ? 0.45f + 0.1f * (V - 10) : 0.f);
	ThoiTiet(V > VongCuoiKinhDi || V <= 3 ? 0 : (V <= 5 ? 1 : 2));
	DatHauKy();
	HL->DatMoiTruong(V > VongCuoiKinhDi ? 60.f : FMath::Lerp(1.2f, 0.25f, M));
	DiThuongPhu(V);

	ANguoiChoi* N = NC();
	switch (V)
	{
	case 1:
		break;
	case 2:
		// Gần như không có gì: một chiếc ghế lệch, biển sàn ướt đổi chỗ, tiếng bước chân xa sau lưng
		HL->Dat(TEXT("Ghe2"), FVector(730.f, -NuaRong + 55.f, 0.f), 115.f);
		HL->Dat(TEXT("BienSanUot"), FVector(1300.f, -45.f, 0.f), -40.f);
		KhiQua(900.f, [this]()
		{
			if (ANguoiChoi* P = NC())
				for (int32 K = 0; K < 3; ++K)
					Hen(K * 0.55f, [this, P]() { AmThanh::Phat3D(this, TEXT("buoc_chan"), P->GetActorLocation() - FVector(700.f, 0.f, 80.f), 0.9f, 0.9f); });
		});
		break;
	case 3:
		// Cột truyền dịch ra giữa hành lang, nhỏ giọt; cửa 402 hé; đèn góc chập
		HL->Dat(TEXT("CotTruyen"), FVector(1050.f, 5.f, 0.f), 30.f);
		bGiotNuoc = true;   // (tiếng nhỏ giọt ở chỗ cột cũ, vẫn nghe được)
		HL->Cua[TEXT("P402")]->DatGoc(-28.f);
		KhiQua(830.f, [this]() { CuaDap(TEXT("P402")); });
		KhiQua(1550.f, [this]() { DenNhom(TEXT("Goc"), 1.f, 0.7f); AmThanh::Phat3D(this, TEXT("den_chap"), FVector(GocX, 0.f, 260.f), 0.8f); Hen(2.f, [this]() { DenNhom(TEXT("Goc"), 1.f, 0.f); }); });
		break;
	case 4:
		// Xe lăn giữa hành lang quay về phía người chơi; thùng rác đổ
		HL->Dat(TEXT("XeLan"), FVector(950.f, 0.f, 0.f), 90.f);
		HL->Dat(TEXT("ThungRac"), FTransform(FRotator(0.f, 10.f, 90.f), FVector(500.f, -92.f, 18.f)));
		KhiQua(700.f, [this]() { AmThanh::Phat3D(this, TEXT("xe_lan"), FVector(950.f, 0.f, 50.f), 0.8f); });
		bLiec = true; LiecS0 = 700.f; LiecS1 = 1800.f;
		KhiQua(1650.f, [this]() { DenNhom(TEXT("A"), 1.f, 0.85f); AmThanh::Phat2D(this, TEXT("den_chap"), 0.6f); Hen(1.6f, [this]() { DenNhom(TEXT("A"), 1.f, 0.f); }); });
		break;
	case 5:
		// Rèm chớp kéo xuống hết; điện thoại quầy trực reo; máy theo dõi tim trong phòng 403 kêu
		for (int32 K = 0; K < 4; ++K) HL->Hien(FName(*FString::Printf(TEXT("Rem%d"), K)), true);
		bTitPhong = true;
		KhiQua(450.f, [this]() { bDienThoaiReo = true; AmThanh::DatAm(AmChuong, 1.f, 0.05f); });
		KhiQua(GocX + 250.f, [this]()
		{
			if (bDienThoaiReo) { bDienThoaiReo = false; AmThanh::DatAm(AmChuong, 0.f, 0.05f); NoiPhuDe(TEXT("The ringing stops."), 2.f); }
		});
		break;
	case 6:
		// Đèn vàng yếu; "người" đứng ngoài cửa sổ trên gờ tường, sét lóe; tiếng gõ cửa phòng 405
		DenNhom(TEXT("*"), 0.7f);
		MauDenNhom(TEXT("*"), FLinearColor(1.f, 0.9f, 0.72f));
		Bong->Dung(FVector(1100.f, NuaRong + 70.f, 0.f), -90.f);
		bBongNgoaiCuaSo = true;
		KhiQua(820.f, [this]() { HL->ChopSet(1.f); });
		KhiQua(GocX + 400.f, [this]() { AmThanh::Phat3D(this, TEXT("go_cua"), FVector(2080.f, 500.f, 120.f), 1.f); });
		break;
	case 7:
		// Cáng có xác phủ khăn giữa đoạn B, biến mất khi quay lưng; xe lăn đã ở ngay cửa đầu; TV nhiễu
		HL->Hien(TEXT("GiuongCang"), true);
		HL->Hien(TEXT("XacPhuKhan"), true);
		bAnCangKhiQua = true;
		HL->Dat(TEXT("XeLan"), FVector(260.f, 0.f, 0.f), 180.f);
		HL->Hien(TEXT("ManTV"), true);
		bTitPhong = true;
		HL->Cua[TEXT("P401")]->DatGoc(-30.f);
		KhiQua(300.f, [this]() { CuaDap(TEXT("P401")); });
		{
			const float CuaSoA[] = { 300.f, 700.f, 1100.f, 1500.f };
			for (int32 K = 0; K < 4; ++K)
			{
				KhiQua(CuaSoA[K] - 260.f, [this, K]() { HienTayKinh(K * 2, true); Hen(0.45f, [this, K]() { HienTayKinh(K * 2 + 1, false); }); });
			}
		}
		bLiec = true; LiecS0 = GocX + 350.f; LiecS1 = GocX + 1100.f;
		break;
	case 8:
		// Bóng người cuối đoạn B dưới đèn; đèn tắt một nhịp là mất; ghế quay mặt vào tường; chữ máu
		HL->Dat(TEXT("Ghe1"), FVector(640.f, -NuaRong + 40.f, 0.f), -90.f);
		HL->Dat(TEXT("Ghe2"), FVector(720.f, -NuaRong + 40.f, 0.f), -90.f);
		HL->Dat(TEXT("Ghe3"), FVector(800.f, -NuaRong + 40.f, 0.f), -90.f);
		HL->Hien(TEXT("ChuMau"), true);
		// Bên kia sân trong: nó đứng co giật trong đoạn B, nhìn thấy qua hai lớp cửa sổ khi còn ở đoạn A
		Bong->CoGiat(FVector(1885.f, 745.f, 0.f), 200.f);
		bCoGiatBenKia = true;
		// Rẽ sang đoạn B thì chẳng có ai. Nhìn dọc hành lang: sấm chớp, nó đứng cuối hành lang, mất điện chớp nhoáng
		bDoaSet = true; ViTriDoaSet = FVector(GocX, 1110.f, 0.f); SDoaSet = GocX + 150.f;
		KhiQua(GocX + 700.f, [this]() { AmThanh::Phat3D(this, TEXT("cua_dap"), FVector(2080.f, 500.f, 120.f), 1.2f); HL->Cua[TEXT("P405")]->Rung(); });
		break;
	case 9:
		// Đèn B đỏ chập chờn; kim đồng hồ quay ngược; dấu tay máu; bóng người cắt ngang góc rẽ; tiếng thì thầm
		DenNhom(TEXT("B"), 0.6f, 0.35f);
		MauDenNhom(TEXT("B"), FLinearColor(1.f, 0.35f, 0.3f));
		bQuayKim = true;
		for (const TCHAR* T : { TEXT("DauTay1"), TEXT("DauTay2"), TEXT("DauTay3"), TEXT("ChuMau") }) HL->Hien(T, true);
		HL->DatAnhTayKinh(TEXT("dau_tay"));
		for (int32 K = 0; K < HL->SoTayKinh; ++K) HienTayKinh(K, false);
		KhiQua(1350.f, [this]()
		{
			if (ANguoiChoi* P = NC())
			{
				const FVector Ma(GocX + 30.f, 160.f, 0.f);
				Bong->Dung(Ma, (P->GetActorLocation() - Ma).Rotation().Yaw, true);
			}
			DenNhom(TEXT("Goc"), -1.f, 0.85f);
			AmThanh::Phat3D(this, TEXT("den_chap"), FVector(GocX, 0.f, 260.f), 0.9f);
			Hen(1.0f, [this]()
			{
				AmThanh::Phat2D(this, TEXT("giat_nhe"), 0.8f);
				Bong->Di(Bong->GetActorLocation(), FVector(GocX - 30.f, -100.f, 0.f), 620.f, true, true);
				++SoCuHu;
			});
			Hen(1.6f, [this]() { DenNhom(TEXT("Goc"), -1.f, 0.f, 0.3f); });
		});
		KhiQua(1750.f, [this]() { AmThanh::Phat3D(this, TEXT("thi_tham"), FVector(GocX, 0.f, 180.f), 0.9f); });
		break;
	case 10:
		// Gần như tối hẳn; nhịp tim; tiếng thở sau gáy; nó đứng sau lưng — quay lại nhìn là đèn tắt phụt
		{
			int32 K = 0;
			for (FDen& D : HL->Den)
				if (D.Nhom == TEXT("A")) { D.Muc = (K == 1 || K == 4) ? 0.6f : 0.08f; D.NhapNhay = (K == 1 || K == 4) ? 0.45f : 0.f; ++K; }
			DenNhom(TEXT("Goc"), 0.3f, 0.3f);
			DenNhom(TEXT("B"), 0.25f, 0.2f);
		}
		bQuayKim = true;
		for (const TCHAR* T : { TEXT("DauTay1"), TEXT("DauTay2"), TEXT("ChuMau"), TEXT("VetCao") }) HL->Hien(T, true);
		bChoBo = true; ViTriBo = 700.f;
		KhiQua(550.f, [this]() { if (ANguoiChoi* P = NC()) AmThanh::Phat3D(this, TEXT("tho"), P->GetActorLocation() - P->HuongNhin().GetSafeNormal2D() * 90.f + FVector(0, 0, 70.f), 1.f); });
		break;
	case 11:
		// Vết máu dẫn vào phòng 403 đang mở, máy tim kêu một tiếng dài; các cửa giật mạnh khi đi qua; nó đi về phía người chơi
		for (const TCHAR* T : { TEXT("VetMau1"), TEXT("VetMau2"), TEXT("DauTay3"), TEXT("VetCao"), TEXT("ChuMau") }) HL->Hien(T, true);
		HL->Cua[TEXT("P403")]->DatGoc(-75.f);
		DenNhom(TEXT("Phong1450"), 1.f, 0.25f);
		AmThanh::DatAm(AmMayThang, 0.8f, 1.f);
		DenNhom(TEXT("*"), 0.45f, 0.25f);
		MauDenNhom(TEXT("*"), FLinearColor(1.f, 0.75f, 0.6f));
		KhiQua(330.f, [this]() { HL->Cua[TEXT("P401")]->Rung(); });
		KhiQua(930.f, [this]() { HL->Cua[TEXT("P402")]->Rung(); });
		Bong->Dung(FVector(GocX - 60.f, 0.f, 0.f), 180.f, true);
		bTienGiat = true;
		SBongTien = GocX - 60.f;
		ChoTienGiat = 0.5f;
		break;
	case 12:
		// Mất điện: chỉ còn đèn khẩn cấp đỏ; tới góc rẽ thì nó lao tới từ phía sau — chạy tới cửa cầu thang
		DenNhom(TEXT("A"), 0.05f);
		DenNhom(TEXT("Goc"), 0.16f, 0.2f);
		DenNhom(TEXT("B"), 0.12f, 0.15f);
		HL->DatMoiTruong(0.05f);
		MauDenNhom(TEXT("*"), FLinearColor(1.f, 0.12f, 0.08f));
		for (const TCHAR* T : { TEXT("DauTay1"), TEXT("DauTay2"), TEXT("DauTay3"), TEXT("ChuMau"), TEXT("VetMau1"), TEXT("VetMau2"), TEXT("VetCao") }) HL->Hien(T, true);
		HL->Cua[TEXT("P403")]->DatGoc(-75.f);
		DenNhom(TEXT("Phong1450"), 1.f, 0.6f);
		KhiQua(GocX - 40.f, [this]()
		{
			AmThanh::Phat2D(this, TEXT("giat_minh"), 1.f);
			AmThanh::Phat2D(this, TEXT("sam_set"), 1.f);
			HL->ChopSet(1.2f);
			DenNhom(TEXT("*"), -1.f, 0.f, 0.25f);
			NoiPhuDe(TEXT("RUN!"), 2.5f);
			Bong->Duoi(FMath::Max(QuangDuong - 1150.f, 250.f), 345.f);
			bDangDuoi = true;
			AmThanh::DatAm(AmDuoi, 1.f, 0.3f);
			AmThanh::DatAm(AmU2, 0.f, 1.f);
			if (ANguoiChoi* P = NC()) P->DoRung = 1.f;
		});
		break;
	default:
		// Vòng 13: bình minh. Không còn gì cả.
		HL->DatMua(false);
		HL->DatBinhMinh(true);
		HL->DatBienLoiRa(true);
		DenNhom(TEXT("*"), 0.35f);
		MauDenNhom(TEXT("*"), FLinearColor(1.f, 0.95f, 0.85f));
		DenNhom(TEXT("Buong"), 0.6f);
		DatNen(0.f, 0.f, 0.f, 0.f, 0.f);
		AmThanh::DatAm(AmBinhMinh, 0.6f, 3.f);
		AmThanh::DatAm(AmDongHo, 0.f, 0.5f);
		HL->DatGio(6.0f);
		Hen(2.f, [this]() { NoiPhuDe(TEXT("06:00. Dawn at last."), 4.f); });
		break;
	}
	(void)N;
}

void ADaoDien::ThoiTiet(int32 Muc)
{
	MucMua = Muc;
	HL->DatMua(Muc > 0);
	HL->DatMuaKinh(Muc == 0 ? 0.f : (Muc == 1 ? 0.55f : 1.f), Muc == 0 ? 0.f : (Muc == 1 ? 0.06f : 0.14f));
	AmThanh::DatAm(AmMua, Muc == 0 ? 0.f : (Muc == 1 ? 0.3f : 0.55f), 2.f);
	ChoSet = FMath::FRandRange(4.f, 9.f);
}

void ADaoDien::TatDen(float Giay)
{
	for (FDen& D : HL->Den)
		if (D.Nhom == TEXT("A") || D.Nhom == TEXT("Goc") || D.Nhom == TEXT("B")) D.TatNgan = FMath::Max(D.TatNgan, Giay);
}

void ADaoDien::CuaDap(const TCHAR* Ten)
{
	ACua* C = HL->Cua[Ten];
	if (C->DaDong()) return;
	C->Dong(true);
	++SoCuHu;
	if (ANguoiChoi* P = NC()) P->DoRung = 0.55f;
}

void ADaoDien::HienTayKinh(int32 K, bool bTieng)
{
	const FName Ten(*FString::Printf(TEXT("TayKinh%d"), K));
	HL->Hien(Ten, true);
	if (bTieng)
		if (USceneComponent* C = HL->Vat(Ten)) AmThanh::Phat3D(this, TEXT("go_kinh"), C->GetComponentLocation(), 0.9f, FMath::FRandRange(0.9f, 1.1f));
}

void ADaoDien::DiThuongPhu(int32 V)
{
	// Mỗi vòng 2–9 thêm một thay đổi nhỏ ngẫu nhiên, để không vòng nào giống hệt lần chơi trước
	if (V < 2 || V > 9) return;
	FRandomStream R(SeedLanChoi + V * 7919);
	switch (R.RandRange(0, 6))
	{
	case 0: HL->Dat(TEXT("ThungRac"), FVector(700.f, NuaRong - 25.f, 0.f), 70.f); break;
	case 1: HL->Dat(TEXT("Ghe3"), FVector(800.f, -NuaRong + 40.f, 0.f), 200.f); break;
	case 2: KhiQua(GocX + 650.f, [this]() { AmThanh::Phat3D(this, TEXT("khay_roi"), FVector(1900.f, 1150.f, 60.f), 1.f); }); break;
	case 3: KhiQua(R.FRandRange(300.f, 1300.f), [this]() { AmThanh::Phat3D(this, TEXT("ro_ri"), FVector(GocX, 900.f, 150.f), 0.7f); }); break;
	case 4: KhiQua(380.f, [this]() { HL->Cua[TEXT("P401")]->Rung(); }); break;
	case 5: HL->Dat(TEXT("XeVeSinh"), FVector(900.f, NuaRong - 40.f, 0.f), 70.f); break;
	default: HL->Dat(TEXT("XeMayTinh"), FVector(1842.f, 830.f, 0.f), 30.f); break;
	}
}

void ADaoDien::ChuyenVong()
{
	ANguoiChoi* N = NC();
	if (!N) return;
	ACua* Cuoi = HL->Cua[TEXT("Cuoi")];
	if (Vong > VongCuoiKinhDi)
	{
		KetThuc();
		return;
	}
	N->DichChuyen(DoiVong());
	Cuoi->DatGoc(0.f);
	HL->Cua[TEXT("Dau")]->DatGoc(0.f);
	++Vong;
	++SoLanChuyenVong;
	if (!bTuKiem) Luu(Vong);
	ApDung(Vong);
}

void ADaoDien::BiBat()
{
	ANguoiChoi* N = NC();
	if (!N || ThoiBiBat >= 0.f) return;
	++SoLanBiBat;
	bDangDuoi = false;
	N->bKhoa = true;
	N->DoRung = 1.f;
	// Mặt nạ áp sát trước mặt
	const FVector H = N->HuongNhin().GetSafeNormal2D();
	Bong->Dung(N->GetActorLocation() + H * 75.f - FVector(0, 0, 92.f), (-H).Rotation().Yaw, false);
	AmThanh::Phat2D(this, TEXT("giat_minh"), 1.3f, 0.9f);
	AmThanh::DatAm(AmDuoi, 0.f, 0.3f);
	ThoiBiBat = 0.f;
}

void ADaoDien::KetThuc()
{
	TrangThai = ETrangThai::KetThuc;
	ThoiKetThuc = 0.f;
	DoTrangDich = 1.f;
	AmThanh::DatAm(AmBinhMinh, 0.f, 3.f);
	AmThanh::Phat2D(this, TEXT("ket_thuc"), 0.8f);
	if (ANguoiChoi* N = NC()) N->bKhoa = true;
	if (!bTuKiem) Luu(0);
}

void ADaoDien::Tick(float Dt)
{
	Super::Tick(Dt);
	ThoiGian += Dt;
	DoToi = FMath::FInterpConstantTo(DoToi, DoToiDich, Dt, 1.2f);
	DoTrang = FMath::FInterpConstantTo(DoTrang, DoTrangDich, Dt, 0.35f);
	if (ThoiPhuDe > 0.f) ThoiPhuDe -= Dt;
	if (TrangThai == ETrangThai::KetThuc) ThoiKetThuc += Dt;
	if (TrangThai != ETrangThai::Choi) { GoiY.Reset(); return; }
	ThoiVong += Dt;

	ANguoiChoi* N = NC();
	if (!N) return;
	const FVector P = N->GetActorLocation();
	QuangDuong = BanDo::QuangDuong(P);

	// Bị bắt: màn đen rồi làm lại vòng cuối
	if (ThoiBiBat >= 0.f)
	{
		ThoiBiBat += Dt;
		if (ThoiBiBat > 0.5f) DoToiDich = 1.f;
		if (ThoiBiBat > 2.6f)
		{
			ThoiBiBat = -1.f;
			N->SetActorLocation(ViTriDau, false, nullptr, ETeleportType::TeleportPhysics);
			if (N->Controller) N->Controller->SetControlRotation(FRotator::ZeroRotator);
			N->bKhoa = false;
			HL->Cua[TEXT("Dau")]->DatGoc(0.f);
			HL->Cua[TEXT("Cuoi")]->DatGoc(0.f);
			ApDung(Vong);
			DoToiDich = 0.f;
			NoiPhuDe(TEXT("...This door again."), 3.f);
		}
		return;
	}

	// Sự kiện theo quãng đường / hẹn giờ
	for (int32 K = 0; K < SuKien.Num(); ++K)
	{
		FSuKien& E = SuKien[K];
		if (E.bXong) continue;
		const bool bToi = E.S > -9000.f ? (QuangDuong >= E.S) : (ThoiVong >= E.Tre);
		if (!bToi) continue;
		E.bXong = true;
		TFunction<void()> Lam = E.Lam;   // Lam có thể thêm sự kiện mới (mảng đổi chỗ)
		Lam();
	}

	// Sét
	if (MucMua >= 1 && (ChoSet -= Dt) <= 0.f)
	{
		// Bão: chớp rồi sấm vọng lại; mưa nhẹ: chỉ sấm xa, không chớp
		if (MucMua >= 2)
		{
			HL->ChopSet(FMath::FRandRange(0.5f, 1.f));
			Hen(FMath::FRandRange(0.3f, 1.5f), [this]() { AmThanh::Phat2D(this, TEXT("sam_xa"), FMath::FRandRange(0.6f, 1.f), FMath::FRandRange(0.85f, 1.1f)); });
			ChoSet = FMath::FRandRange(7.f, 15.f);
		}
		else
		{
			AmThanh::Phat2D(this, TEXT("sam_xa"), 0.35f, 0.8f);
			ChoSet = FMath::FRandRange(18.f, 30.f);
		}
	}

	// Liếc ra sau: thấy nó trong tích tắc rồi biến mất
	if (bLiec && QuangDuong >= LiecS0 && QuangDuong <= LiecS1 && !Bong->DangHien())
	{
		const FVector Huong = HuongDuong(QuangDuong);
		const float SB = QuangDuong > GocX ? FMath::Max(QuangDuong - 650.f, GocX + 40.f) : FMath::Max(QuangDuong - 650.f, 40.f);
		if (QuangDuong - SB > 350.f && FVector::DotProduct(N->HuongNhin().GetSafeNormal2D(), Huong) < -0.6f)
		{
			const FVector PB = DiemDuong(SB);
			Bong->Dung(PB, (P - PB).Rotation().Yaw, true);
			AmThanh::Phat2D(this, TEXT("giat_nhe"), 0.55f);
			Hen(0.16f, [this]() { Bong->An(); });
			bLiec = false;
			++SoCuHu;
		}
	}
	// Sấm chớp + bóng người + mất điện chớp nhoáng, tất cả trong nửa giây
	if (bDoaSet && !bCoGiatBenKia && QuangDuong >= SDoaSet && NhinVao(ViTriDoaSet + FVector(0, 0, 150.f), 0.8f))
	{
		bDoaSet = false;
		++SoCuHu;
		Bong->Dung(ViTriDoaSet, (P - ViTriDoaSet).Rotation().Yaw, true);
		HL->ChopSet(1.3f);
		AmThanh::Phat2D(this, TEXT("sam_set"), 1.1f);
		Hen(0.18f, [this]() { TatDen(0.35f); Bong->An(); AmThanh::Phat2D(this, TEXT("cup_dien"), 0.5f); });
		Hen(0.55f, [this]() { AmThanh::Phat2D(this, TEXT("dien_lai"), 0.45f); });
	}
	// Nó bò ngoài kính: ánh chớp hắt lên người (không thì chỉ là bóng đen trên nền đen)
	Bong->AnhChieu->SetIntensity(Bong->CheDo == EBong::Bo ? 120.f + HL->Set * 2500.f : 0.f);
	// Bóng co giật bên kia sân: tới góc rẽ thì biến mất (rẽ sang đoạn B sẽ không thấy ai)
	if (bCoGiatBenKia)
	{
		if ((ChoCoGiat -= Dt) <= 0.f) { AmThanh::Phat3D(this, TEXT("co_giat"), Bong->GetActorLocation() + FVector(0, 0, 150.f), 0.8f, FMath::FRandRange(0.8f, 1.1f)); ChoCoGiat = FMath::FRandRange(1.8f, 3.f); }
		if (NhinVao(Bong->GetActorLocation() + FVector(0, 0, 140.f), 0.93f)) ThoiThayCoGiat += Dt;
		if (QuangDuong > 1720.f || ThoiThayCoGiat > 1.1f)
		{
			if (ThoiThayCoGiat > 1.1f) AmThanh::Phat3D(this, TEXT("giat_nhe"), Bong->GetActorLocation() + FVector(0, 0, 150.f), 0.6f);
			Bong->An(); bCoGiatBenKia = false; ++SoCuHu;
		}
	}
	// Sinh vật 8 chân: từ sàn bò lên tường rồi qua trần, ngang trên đầu người chơi, ra sau lưng thì mất
	if (bChoBo && P.X > 60.f && P.X < ViTriBo - 180.f && NhinVao(FVector(ViTriBo, NuaRong, 120.f), 0.85f))
	{
		bChoBo = false;
		++SoCuHu;
		float Xt = FMath::Min(P.X + 400.f, 1650.f);
		for (float Cs : { 300.f, 700.f, 1100.f, 1500.f })
			if (FMath::Abs(Xt - Cs) < 110.f) Xt = Cs + (Xt > Cs ? 120.f : -120.f);   // leo chỗ tường đặc, tránh ô cửa sổ
		TArray<FVector> Dichs = { FVector(Xt, NuaRong - 2.f, 210.f), FVector(P.X + 60.f, 0.f, Cao - 2.f), FVector(P.X - 420.f, 0.f, Cao - 2.f) };
		Nhen->Bat(FVector(Xt + 80.f, NuaRong - 60.f, 1.f), FVector::UpVector, FVector(0.f, 1.f, 0.f), Dichs, 240.f);
		TatDen(0.2f);
		AmThanh::Phat2D(this, TEXT("giat_nhe"), 0.7f);
	}
	if (Nhen && Nhen->DangHien()) Nhen->AnhChieu->SetIntensity(180.f + HL->Set * 2000.f);
	{
		const bool bSangDT = bDienThoaiReo && FMath::Frac(ThoiGian * 2.2f) < 0.5f;
		HL->DenDienThoai->SetIntensity(bSangDT ? 600.f : 0.f);
		HL->MatDenDT->SetScalarParameterValue(TEXT("DoSang"), bSangDT ? 400.f : (bDienThoaiReo ? 5.f : 0.f));
	}
	if (bTitPhong && (ChoTit -= Dt) <= 0.f) { AmThanh::Phat3D(this, TEXT("may_tim_bip"), ViTriPhong403, 0.5f); ChoTit = 1.05f; }
	if (bGiotNuoc && (ChoGiot -= Dt) <= 0.f) { AmThanh::Phat3D(this, TEXT("giot_nuoc"), FVector(1150.f, 10.f, 60.f), 0.7f, FMath::FRandRange(0.9f, 1.1f)); ChoGiot = 1.7f; }
	if (bQuayKim) HL->DatGio(3.f - ThoiVong * 0.9f);

	// Người ngoài cửa sổ: tới gần hoặc nhìn lâu thì sét lóe và biến mất
	if (bBongNgoaiCuaSo && Bong->DangHien())
	{
		if (NhinVao(Bong->GetActorLocation() + FVector(0, 0, 160.f), 0.93f)) NhinBong += Dt;
		if (FMath::Abs(P.X - Bong->GetActorLocation().X) < 230.f || NhinBong > 2.2f)
		{
			HL->ChopSet(1.f);
			Bong->An();
			bBongNgoaiCuaSo = false;
			NhinBong = 0.f;
		}
	}
	// Nó đứng sau lưng: quay lại nhìn quá 0,3 s là đèn tắt phụt, nó biến mất
	if (bBongSauLung && Bong->DangHien())
	{
		if (NhinVao(Bong->GetActorLocation() + FVector(0, 0, 150.f), 0.85f)) NhinBong += Dt;
		if (NhinBong > 0.3f)
		{
			DenNhom(TEXT("*"), -1.f, 0.45f, 0.7f);
			AmThanh::Phat2D(this, TEXT("giat_minh"), 0.8f);
			N->DoRung = 0.6f;
			Hen(0.25f, [this]() { Bong->An(); });
			bBongSauLung = false;
		}
	}
	// Cáng biến mất khi người chơi đi qua và không nhìn
	if (bAnCangKhiQua && QuangDuong > GocX + 860.f && !NhinVao(FVector(1856.f, 668.f, 90.f), 0.2f))
	{
		HL->Hien(TEXT("GiuongCang"), false);
		HL->Hien(TEXT("XacPhuKhan"), false);
		AmThanh::Phat3D(this, TEXT("cang_xe_day"), FVector(1856.f, 668.f, 60.f), 0.9f);
		bAnCangKhiQua = false;
	}
	// Vòng 11: mỗi lần đèn tắt phụt, nó đứng gần hơn; sát mặt thì hét rồi biến mất
	if (bTienGiat && QuangDuong > 700.f && (ChoTienGiat -= Dt) <= 0.f)
	{
		ChoTienGiat = 1.1f;
		TatDen(0.22f);
		AmThanh::Phat2D(this, TEXT("den_chap"), 0.5f);
		Hen(0.07f, [this]()
		{
			ANguoiChoi* P2 = NC();
			if (!P2 || !bTienGiat) return;
			SBongTien = FMath::Max(SBongTien - 300.f, QuangDuong + 120.f);
			const FVector Ma = DiemDuong(SBongTien);
			Bong->Dung(Ma, (P2->GetActorLocation() - Ma).Rotation().Yaw, true);
			if (SBongTien - QuangDuong <= 220.f)
			{
				bTienGiat = false;
				++SoCuHu;
				Hen(0.5f, [this]()
				{
					ANguoiChoi* P3 = NC();
					if (!P3) return;
					TatDen(0.25f);
					const FVector H2 = P3->HuongNhin().GetSafeNormal2D();
					Bong->Dung(P3->GetActorLocation() + H2 * 85.f - FVector(0, 0, 92.f), (-H2).Rotation().Yaw, true);
					AmThanh::Phat2D(this, TEXT("giat_minh"), 1.1f);
					P3->DoRung = 0.8f;
					Hen(0.45f, [this]() { TatDen(0.5f); Bong->An(); });
				});
			}
		});
	}
	// Luật "đừng nhìn lại" (vòng 8–11): mỗi lần quay lại nhìn, nó hiện ra trong tích tắc, lần sau gần hơn; lần thứ 3 thì bị bắt
	if (bLuatNhinLai && !Bong->DangHien() && ThoiBiBat < 0.f && QuangDuong > 120.f && QuangDuong < TongDuong - 60.f)
	{
		const float Doi = FVector::DotProduct(N->HuongNhin().GetSafeNormal2D(), HuongDuong(QuangDuong));
		if (Doi > 0.2f) { bChoQuayDi = false; ThoiNhinLai = 0.f; }
		else if (Doi < -0.6f && !bChoQuayDi && (ThoiNhinLai += Dt) > 0.25f)
		{
			bChoQuayDi = true;
			++SoLanNhinLai;
			++SoCuHu;
			if (SoLanNhinLai >= 3) BiBat();
			else
			{
				const float Xa = SoLanNhinLai == 1 ? 520.f : 240.f;
				const float SB = QuangDuong > GocX ? FMath::Max(QuangDuong - Xa, GocX + 30.f) : FMath::Max(QuangDuong - Xa, 40.f);
				const FVector Ma = DiemDuong(SB);
				Bong->Dung(Ma, (P - Ma).Rotation().Yaw, true);
				AmThanh::Phat2D(this, SoLanNhinLai == 1 ? TEXT("giat_nhe") : TEXT("giat_minh"), 0.85f);
				N->DoRung = SoLanNhinLai * 0.3f;
				Hen(0.2f, [this]() { TatDen(0.15f); Bong->An(); });
				NoiPhuDe(SoLanNhinLai == 1 ? TEXT("Don't look back.") : TEXT("Don't. Look. Back."), 2.5f);
			}
		}
	}
	// Nó tiến lại: còn cách 3,8 m thì tắt đèn, biến mất
	if (bBongTienLai && Bong->DangHien() && FVector::Dist2D(Bong->GetActorLocation(), P) < 380.f)
	{
		DenNhom(TEXT("*"), -1.f, 0.25f, 0.9f);
		AmThanh::Phat3D(this, TEXT("tho"), P + FVector(0, 0, 60.f), 1.f);
		Hen(0.3f, [this]() { Bong->An(); });
		bBongTienLai = false;
	}

	// Rượt đuổi
	ACua* Cuoi = HL->Cua[TEXT("Cuoi")];
	if (bDangDuoi)
	{
		if (!TrongBuongCuoi(P) && Bong->QuangDuong >= QuangDuong - 70.f) BiBat();
		else if (Cuoi->DaDong() && FVector::Dist2D(P, Cuoi->GetActorLocation()) < 190.f) Cuoi->Mo();
	}

	// Vào buồng cuối: cửa tự đóng sau lưng, đóng hẳn thì sang vòng mới
	if (TrongBuongCuoi(P))
	{
		if (!Cuoi->DaDong() && !Cuoi->DangChuyen() && P.Y > CuoiB + 70.f)
		{
			Cuoi->Dong(Vong >= 8);
			if (bDangDuoi) { bDangDuoi = false; AmThanh::DatAm(AmDuoi, 0.f, 0.4f); Hen(0.4f, [this]() { Bong->An(); }); }
		}
		if (Cuoi->DaDong() && !bDangChuyen)
		{
			bDangChuyen = true;
			ChuyenVong();
			bDangChuyen = false;
		}
	}

	// Gợi ý tương tác
	GoiY.Reset();
	if (bDienThoaiReo && FVector::Dist(N->ViTriMat(), ViTriDienThoai) < 260.f && NhinVao(ViTriDienThoai, 0.6f)) GoiY = TEXT("[E] Answer");
	else if (ACua* C = CuaDangNhin()) { if (C->DaDong()) GoiY = TEXT("[E] Open"); }
	else if (!N->DangNam())
	{
		FHitResult H;
		FCollisionQueryParams Q;
		Q.AddIgnoredActor(N);
		if (GetWorld()->LineTraceSingleByChannel(H, N->ViTriMat(), N->ViTriMat() + N->HuongNhin() * 260.f, ECC_Visibility, Q) && HL->KeoDuoc(H.GetComponent()))
			GoiY = TEXT("[Hold Left Mouse] Drag");
	}
}
