// Dựng toàn bộ bối cảnh bằng code: hành lang chữ L khoa hồi sức tầng 4, hai buồng cầu thang, phòng bệnh,
// sân trong mưa đêm, đèn huỳnh quang, đồ đạc (mẫu SimReady). Đạo diễn (DaoDien) đổi trạng thái các thứ này mỗi vòng.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HanhLang.generated.h"

class ACua;
class UProceduralMeshComponent;
class UStaticMeshComponent;
class URectLightComponent;
class UPointLightComponent;
class UDirectionalLightComponent;
class USkyLightComponent;
class UDecalComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class APostProcessVolume;

// Một đèn trần: đèn thật + tấm phát sáng; Muc = độ sáng đạo diễn đặt (0..1), NhapNhay = mức chập chờn
struct FDen
{
	URectLightComponent* Den = nullptr;
	UPointLightComponent* Bong = nullptr;   // bóng đèn trần buồng cầu thang
	UPointLightComponent* Phu = nullptr;    // đèn phụ hắt lên trần (Lumen không dội sáng từ lưới procedural)
	float CuongDoPhu = 0.f;
	UMaterialInstanceDynamic* MatSang = nullptr;
	float CuongDo = 0.f;
	float DoSangMat = 6.f;
	float Muc = 1.f;
	float NhapNhay = 0.f;
	float Hat = 0.f;            // pha ngẫu nhiên riêng
	float TatNgan = 0.f;        // >0: tắt hẳn trong khoảng này (chớp tắt có chủ ý)
	FLinearColor Mau = FLinearColor::White;
	FString Nhom;               // A, Goc, B, Buong
};

// Đồ vật có vị trí gốc để đặt lại mỗi vòng
struct FVatDung
{
	USceneComponent* C = nullptr;
	FTransform Goc;
	bool bHienGoc = true;
	bool bDay = false;   // đẩy được: C là hộp va chạm mô phỏng vật lý, lưới gắn bên trong
	FVector LechTam = FVector::ZeroVector;   // tâm hộp so với gốc lưới
};

UCLASS()
class AHanhLang : public AActor
{
	GENERATED_BODY()

public:
	AHanhLang();
	virtual void Tick(float Dt) override;

	void Dung();

	// Đặt lại mọi đồ vật / vết / đèn về trạng thái gốc
	void DatLai();
	void Dat(FName Ten, const FVector& ViTri, float Yaw);
	void Dat(FName Ten, const FTransform& T);
	void Hien(FName Ten, bool bHien);
	USceneComponent* Vat(FName Ten) const;
	// Hộp vật lý của đồ kéo được (nullptr nếu C không phải)
	bool KeoDuoc(const UPrimitiveComponent* C) const;
	// Đồng hồ treo tường: giờ (số thực, 3.5 = 3:30)
	void DatGio(float Gio);
	// Chớp sét: ánh trời sáng lóe trong vài khung
	void ChopSet(float Manh = 1.f);
	void DatMua(bool bMua);
	void DatBinhMinh(bool bSang);
	void DatBienLoiRa(bool bLoiRa);
	// Ánh sáng môi trường (skylight) — thay cho ánh dội mà Lumen không tính được
	void DatMoiTruong(float CuongDo);
	// Đồ vật người chơi đẩy đi được (mô phỏng vật lý, khóa nghiêng để không đổ)
	void DayDuoc(FName Ten, float KhoiLuong);
	// Kính: Co = độ ướt (0..1), Chay = tốc độ vệt nước trượt
	void DatMuaKinh(float Co, float Chay);
	// Dấu tay trên kính: texture dùng chung (tay_kinh = mờ, dau_tay = máu)
	void DatAnhTayKinh(const FString& Anh);
	int32 SoTayKinh = 0;

	TArray<FDen> Den;
	TMap<FName, FVatDung> VatDung;
	TMap<FName, ACua*> Cua;   // Dau (buồng đầu → A), Cuoi (B → buồng cuối), GiaDau/GiaCuoi (cửa giả), P401..P405
	UPROPERTY() TArray<AActor*> ActorCon;

	UPROPERTY() APostProcessVolume* HauKy;
	UPROPERTY() UDirectionalLightComponent* TroiDem;
	UPROPERTY() USkyLightComponent* AnhTroi;
	UPROPERTY() UDirectionalLightComponent* MatTroi;   // nắng sáng (kết thúc)
	UPROPERTY() UStaticMeshComponent* ManTV;
	UPROPERTY() UMaterialInstanceDynamic* MatTV;
	UPROPERTY() UMaterialInstanceDynamic* MatBienThoat;
	UPROPERTY() UMaterialInstanceDynamic* MatTroiNen;
	UPROPERTY() UMaterialInstanceDynamic* MatKinh;
	UPROPERTY() UMaterialInstanceDynamic* MatMuaKinh;
	UPROPERTY() UMaterialInstanceDynamic* MatTayKinh;
	UPROPERTY() UInstancedStaticMeshComponent* Mua;
	UPROPERTY() USceneComponent* KimGio;
	UPROPERTY() USceneComponent* KimPhut;
	UPROPERTY() UPointLightComponent* DenDienThoai;
	UPROPERTY() UMaterialInstanceDynamic* MatDenDT;

	float Set = 0.f;   // độ sáng chớp sét hiện tại

private:
	UStaticMeshComponent* Khoi(const FVector& Tam, const FVector& CoCm, UMaterialInterface* M, const FRotator& Xoay = FRotator::ZeroRotator, USceneComponent* Cha = nullptr);
	UStaticMeshComponent* Mau(FName Ten, const FString& TenMau, const FVector& ViTri, float Yaw, float TiLe = 1.f, bool bHien = true);
	UDecalComponent* Vet(FName Ten, const FString& Anh, const FVector& ViTri, const FRotator& Huong, const FVector& Co, bool bHien = false);
	ACua* TaoCua(FName Ten, const FVector& ViTri, float Yaw, const FString& Bien, bool bKinh, bool bKhoa, const FLinearColor& Mau);
	void DenTran(const FVector& ViTri, float Yaw, const FString& Nhom);
	void DungTuong();
	void DungBuong(const FTransform& T, const FString& Ten);
	void DungPhongBenh(float X, float Sau, bool bCoDo);
	void DungSanTrong();
	void DungDoDat();
	void DungDoThem();
	void DungAnhSang();
	void DatViTri(FVatDung& V, const FTransform& T, bool bHien);

	UPROPERTY() UProceduralMeshComponent* LuoiSan;
	UPROPERTY() UProceduralMeshComponent* LuoiTuong;
	UPROPERTY() UProceduralMeshComponent* LuoiTran;
	UPROPERTY() UProceduralMeshComponent* LuoiBeTong;
	UPROPERTY() UProceduralMeshComponent* LuoiKhac;

	TArray<FVector> ViTriMua;
	float ThoiGian = 0.f;
};
