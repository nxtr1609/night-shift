// Đạo diễn: đếm vòng, dời người chơi khi đóng cửa buồng cuối, đặt dị thường mỗi vòng (tăng dần từ "gần như không có gì"
// tới kinh dị), sự kiện theo quãng đường đi, âm thanh nền, rượt đuổi vòng cuối và kết thúc.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DaoDien.generated.h"

class AHanhLang;
class ABongNguoi;
class ANguoiChoi;
class ACua;
class UAudioComponent;
class ASinhVat;

UENUM()
enum class ETrangThai : uint8 { Menu, Choi, TamDung, KetThuc };

UCLASS()
class ADaoDien : public AActor
{
	GENERATED_BODY()
	friend class ATuKiem;   // chế độ quay trailer điều khiển thẳng các cú hù

public:
	ADaoDien();
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

	static ADaoDien* Lay(const UObject* Ngu);

	static constexpr int32 VongCuoiKinhDi = 12;   // vòng 13 = bình minh, kết thúc

	// Bắt đầu chơi từ vòng V (1 = chơi mới)
	void BatDau(int32 V);
	void VeMenu();
	void TamDung(bool bDung);
	// Người chơi bấm E / chuột trái
	void TuongTac();
	// Chuột trái: nắm / thả đồ vật kéo được trước mặt
	void NamVat(bool bGiu);

	ETrangThai TrangThai = ETrangThai::Menu;
	int32 Vong = 1;
	float QuangDuong = 0.f;
	bool bTuKiem = false;
	bool bTrailer = false;   // quay trailer: ẩn chấm ngắm và gợi ý

	// Cho giao diện
	FString PhuDe;
	float ThoiPhuDe = 0.f;
	FString GoiY;
	float DoToi = 0.f;     // màn đen 0..1
	float DoTrang = 0.f;   // màn trắng 0..1
	float ThoiKetThuc = 0.f;
	bool bDangDuoi = false;
	int32 SoLanBiBat = 0;
	int32 SoLanChuyenVong = 0;
	int32 SoCuHu = 0;   // số cú hù đã xảy ra trong vòng này (cho tự kiểm)

	// Lưu / đọc vòng đã tới (Saved/CaTrucDem.txt)
	static void Luu(int32 V);
	static int32 Doc();
	static FVector ViTriDT();

	UPROPERTY() AHanhLang* HL;
	UPROPERTY() ABongNguoi* Bong;
	UPROPERTY() ASinhVat* Nhen;
	int32 SoLanNhinLai = 0;   // luật "đừng nhìn lại" (vòng 8–11): lần thứ 3 thì bị bắt

	// Cho tự kiểm
	ACua* CuaDangNhin() const;
	bool bDienThoaiReo = false;

private:
	struct FSuKien
	{
		float S = 0.f;           // khi quãng đường >= S
		float Tre = 0.f;         // hoặc sau bao nhiêu giây (S < -9000 = theo giờ)
		TFunction<void()> Lam;
		bool bXong = false;
	};

	ANguoiChoi* NC() const;
	void ApDung(int32 V);
	void KhiQua(float S, TFunction<void()> Lam);
	void Hen(float Giay, TFunction<void()> Lam);
	void NoiPhuDe(const FString& Chu, float Giay = 3.5f);
	void ChuyenVong();
	void BiBat();
	void KetThuc();
	void DatNen(float Den, float Mua, float UAm1, float UAm2, float Tim);
	void DatHauKy();
	void DenNhom(const FString& Nhom, float Muc, float NhapNhay = 0.f, float TatNgan = 0.f);
	void MauDenNhom(const FString& Nhom, const FLinearColor& C);
	void DiThuongPhu(int32 V);
	bool NhinVao(const FVector& P, float Cos) const;
	UPrimitiveComponent* VatDangNhin(float Tam = 230.f) const;
	// Mưa / sấm theo vòng: 0 khô, 1 mưa nhẹ, 2 bão có sấm chớp
	void ThoiTiet(int32 Muc);
	// Tắt phụt mọi đèn hành lang trong Giay giây (giữ nguyên chập chờn đang có)
	void TatDen(float Giay);
	// Cửa đang hé đóng sầm lại
	void CuaDap(const TCHAR* Ten);
	void HienTayKinh(int32 K, bool bTieng);

	TArray<FSuKien> SuKien;
	float ThoiGian = 0.f;
	float ThoiVong = 0.f;
	float ChoSet = 8.f;
	float ChoTit = 0.f;
	float ChoGiot = 0.f;
	float NhinBong = 0.f;
	bool bTitPhong = false;
	bool bGiotNuoc = false;
	bool bQuayKim = false;
	bool bBongSauLung = false;
	bool bBongNgoaiCuaSo = false;
	bool bDangChuyen = false;
	bool bDaNghe = false;
	bool bAnCangKhiQua = false;
	bool bBongTienLai = false;
	int32 SeedLanChoi = 1;
	int32 MucMua = 0;
	// Liếc ra sau thấy nó trong tích tắc (khi quãng đường trong [LiecS0, LiecS1])
	bool bLiec = false;
	float LiecS0 = 0.f, LiecS1 = 0.f;
	// Sấm chớp + bóng người + mất điện chớp nhoáng (khi đã qua SDoaSet và nhìn về phía ViTriDoaSet)
	bool bDoaSet = false;
	FVector ViTriDoaSet = FVector::ZeroVector;
	float SDoaSet = 0.f;
	// Bóng người co giật bên kia sân (nhìn qua cửa sổ); biến mất khi tới gần
	bool bCoGiatBenKia = false;
	float ChoCoGiat = 0.f;
	// Thứ gì đó bò lên cửa sổ (khi nhìn về phía cửa sổ ở X = ViTriBo)
	bool bChoBo = false;
	float ViTriBo = 700.f;
	bool bLuatNhinLai = false;
	bool bChoQuayDi = false;
	float ThoiNhinLai = 0.f;
	float ThoiThayCoGiat = 0.f;
	// Vòng 11: mỗi lần đèn tắt phụt nó đứng gần hơn
	bool bTienGiat = false;
	float SBongTien = 0.f;
	float ChoTienGiat = 0.f;

	UPROPERTY() UAudioComponent* AmDen;
	UPROPERTY() UAudioComponent* AmMua;
	UPROPERTY() UAudioComponent* AmU1;
	UPROPERTY() UAudioComponent* AmU2;
	UPROPERTY() UAudioComponent* AmTim;
	UPROPERTY() UAudioComponent* AmDuoi;
	UPROPERTY() UAudioComponent* AmBinhMinh;
	UPROPERTY() UAudioComponent* AmDongHo;
	UPROPERTY() UAudioComponent* AmChuong;
	UPROPERTY() UAudioComponent* AmMayThang;
	UPROPERTY() USceneComponent* GocDongHo;
	UPROPERTY() USceneComponent* GocDienThoai;
	UPROPERTY() USceneComponent* GocPhong403;
};
