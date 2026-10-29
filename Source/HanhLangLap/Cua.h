// Cửa bản lề: gốc actor ở giữa chân khung cửa, trục X cục bộ = phía cửa mở ra.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Cua.generated.h"

class UStaticMeshComponent;

UCLASS()
class ACua : public AActor
{
	GENERATED_BODY()

public:
	ACua();
	virtual void Tick(float Dt) override;

	// Dựng cánh cửa; TenBien = texture biển gắn mặt trước (rỗng = không có), bCoKinh = ô kính nhỏ
	void Dung(const FString& TenBien, bool bCoKinh, const FLinearColor& MauSon);

	void Mo(bool bAmThanh = true);
	void Dong(bool bDap);
	// Đặt ngay trạng thái (không hoạt ảnh, không tiếng)
	void DatGoc(float Goc);
	bool DaDong() const { return FMath::IsNearlyZero(Goc, 0.5f) && FMath::IsNearlyZero(GocDich, 0.5f); }
	bool DangChuyen() const { return !FMath::IsNearlyEqual(Goc, GocDich, 0.5f); }
	// Rung tay nắm (cửa khóa)
	void Rung();

	bool bKhoa = false;
	float GocMo = -100.f;   // góc khi mở hẳn (độ, quanh bản lề)
	float TocDo = 120.f;    // độ/giây
	FString Ten;

	UPROPERTY() USceneComponent* BanLe;
	UPROPERTY() UStaticMeshComponent* Canh;

private:
	float Goc = 0.f;
	float GocDich = 0.f;
	float ThoiRung = 0.f;
	bool bDangDap = false;
};
