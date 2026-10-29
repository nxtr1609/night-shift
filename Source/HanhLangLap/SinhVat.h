// Sinh vật 8 chân bò sàn → tường → trần, hoàn toàn bằng code (không khung hoạt ảnh, không học máy, chỉ tia dò).
// Theo ghi chú kỹ thuật người dùng đưa: điểm neo bám mặt + vận chuyển song song hướng đi khi đổi mặt,
// "chân trước chạm mặt mới trước", dáng đi 2 nhóm 4 chân xen kẽ, thân theo pháp tuyến mặt đỡ + mặt phẳng bàn chân, IK 2 đoạn.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SinhVat.generated.h"

class UStaticMeshComponent;
class UAudioComponent;
class UMaterialInterface;

USTRUCT()
struct FChanNhen
{
	GENERATED_BODY()
	FVector GocThan = FVector::ZeroVector;   // gốc chân trong hệ thân (X trước, Y phải, Z lên)
	FVector HuongRa = FVector::ZeroVector;   // hướng duỗi ra (hệ thân)
	FVector BanChan = FVector::ZeroVector;   // vị trí bàn chân (thế giới)
	FVector PhapTuyen = FVector::UpVector;   // pháp tuyến mặt bàn chân đang bám
	FVector TuDiem = FVector::ZeroVector, DenDiem = FVector::ZeroVector;
	FVector TuPT = FVector::UpVector, DenPT = FVector::UpVector;
	float Pha = 0.f;                         // 0..1 khi đang nhấc
	bool bNhac = false;
	bool bChamMatMoi = false;                // chân trước đã chạm mặt mới (khi đổi mặt)
	int32 Nhom = 0;
	UPROPERTY() UStaticMeshComponent* Dui = nullptr;
	UPROPERTY() UStaticMeshComponent* Cang = nullptr;
	UPROPERTY() UStaticMeshComponent* Khop = nullptr;
};

UCLASS()
class ASinhVat : public AActor
{
	GENERATED_BODY()

public:
	ASinhVat();
	virtual void Tick(float Dt) override;

	// Bắt đầu bò từ điểm trên một mặt (pháp tuyến N), lần lượt tới các đích (đích cuối tới nơi thì biến mất)
	void Bat(const FVector& Diem, const FVector& N, const FVector& Huong, const TArray<FVector>& CacDich, float Toc = 230.f);
	void An();
	bool DangHien() const { return bHien; }

	// Cho tự kiểm: các mặt đã bò qua (pháp tuyến lúc đổi mặt)
	TArray<FVector> MatDaQua;
	FVector Neo = FVector::ZeroVector;   // điểm neo trên mặt
	FVector PhapNeo = FVector::UpVector;
	UPROPERTY() UAudioComponent* TiengBo;
	UPROPERTY() class UPointLightComponent* AnhChieu;

private:
	bool Do(const FVector& A, const FVector& B, FHitResult& H) const;
	void DiChuyenNeo(float Dt);
	void BuocChan(float Dt);
	void DatThan(float Dt);
	void VeChan();
	bool TimCho(const FChanNhen& C, const FVector& Muc, FVector& Diem, FVector& N) const;
	FVector GocTheGioi(const FChanNhen& C) const;
	UStaticMeshComponent* Khoi(UStaticMesh* M, UMaterialInterface* Mat);

	TArray<FChanNhen> Chan;
	TArray<FVector> Dich;
	int32 DichSo = 0;
	FVector Truoc = FVector::ForwardVector;   // hướng đi (nằm trên mặt neo)
	float Toc = 230.f;
	float TocThuc = 0.f;
	bool bHien = false;
	// Đổi mặt: đã thấy mặt mới nhưng chờ chân trước chạm vào
	bool bChoDoiMat = false;
	FVector DiemMatMoi = FVector::ZeroVector, PhapMatMoi = FVector::UpVector;
	float ThoiCho = 0.f;
	// Thân
	FVector ViTriThan = FVector::ZeroVector, VanTocThan = FVector::ZeroVector;
	FVector LenThan = FVector::UpVector;
	FQuat XoayThan = FQuat::Identity;
	float NhunCao = 0.f, VanTocNhun = 0.f;
	float PhaDi = 0.f;
	float ThoiGian = 0.f;
	UPROPERTY() USceneComponent* Than;
	UPROPERTY() TArray<UStaticMeshComponent*> PhanThan;
	UPROPERTY() UMaterialInterface* MatDa;
};
