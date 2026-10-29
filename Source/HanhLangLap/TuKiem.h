// Tự kiểm: -autotest=<thư mục> -che_do=choi (máy tự đi hết 12 vòng + kết thúc) hoặc anh (chụp từng vòng từ chỗ cố định).
// Ghi KetQua.txt + ảnh vào thư mục rồi thoát.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TuKiem.generated.h"

UCLASS()
class ATuKiem : public AActor
{
	GENERATED_BODY()

public:
	ATuKiem();
	virtual void Tick(float Dt) override;

	FString ThuMuc;
	FString CheDo = TEXT("choi");
	int32 VongDau = 1;

private:
	void Kiem(bool bDat, const FString& MoTa);
	void Chup(const FString& Ten, bool bGiaoDien = false);
	void Xong();
	void TickChoi(float Dt);
	void TickAnh(float Dt);
	void TickHu(float Dt);
	void TickDuong(float Dt);
	// Quay trailer: các cảnh dàn dựng, chụp mọi khung (chạy với -benchmark -fps=30 cho nhịp cố định)
	void TickTrailer(float Dt);
	int32 KhungTrailer = 0;
	float ThoiCanh = 0.f;
	// Kiểm tra từ đầu tới cuối hành lang luôn có lối cho người chơi đi qua (không cần đẩy đồ)
	bool KiemLoiDi(FString& NoiChan) const;
	void NhinVe(const FVector& Diem, float Dt, float TocQuay = 8.f);
	FVector LanTranh(const FVector& Tu, const FVector& Dich) const;

	TArray<FString> KetQua;
	int32 SoDat = 0, SoPhep = 0;
	float ThoiGian = 0.f;
	float ThoiVong = 0.f;
	int32 KhungCho = 0;
	bool bBatDau = false;
	bool bXong = false;
	// Chơi
	int32 VongTruoc = 0;
	bool bDaChupA = false, bDaChupB = false, bDaNhinLai = false, bDaNgheMay = false;
	int32 CuHuVong = 0;
	FVector ViTriXeLan = FVector::ZeroVector;
	bool bDaKiemKetThuc = false, bThayBongSauLung = false, bBongBienMat = false, bThayDuoi = false;
	float ChoDung = 0.f;     // dừng lại chụp ảnh
	FString AnhCho;
	FVector ViTriTruoc = FVector::ZeroVector;
	FVector TuongDoiTruoc = FVector::ZeroVector;
	float KhongTien = 0.f;
	float QuangTruoc = -1e9f;
	int32 SoKhung = 0;
	float TongKhungGiay = 0.f;
	// Ảnh
	int32 BuocAnh = 0;
};
