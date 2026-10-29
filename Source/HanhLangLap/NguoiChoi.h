// Người chơi góc nhìn thứ nhất: đi chậm, giữ Shift chạy, lắc đầu và tiếng bước chân theo nhịp đi.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NguoiChoi.generated.h"

class UCameraComponent;

UCLASS()
class ANguoiChoi : public ACharacter
{
	GENERATED_BODY()

public:
	ANguoiChoi();
	virtual void Tick(float Dt) override;

	void DiChuyen(const FVector2D& Truc, bool bChay);
	void Nhin(const FVector2D& Chuot);
	// Dịch chuyển giữ nguyên vận tốc và hướng nhìn tương đối (vòng lặp)
	void DichChuyen(const FTransform& Doi);
	FVector ViTriMat() const;
	FVector HuongNhin() const;
	// Khóa điều khiển (đoạn phim, bị bắt)
	bool bKhoa = false;
	float DoRung = 0.f;   // rung camera (0..1), giảm dần

	UPROPERTY() UCameraComponent* Camera;
	UPROPERTY() class UPhysicsHandleComponent* Tay;
	// Nắm đồ vật trước mặt (chuột trái giữ), thả ra khi nhả chuột
	bool Nam(UPrimitiveComponent* Vat, const FVector& Diem);
	void Tha();
	bool DangNam() const;

	static constexpr float TocDi = 140.f;
	static constexpr float TocChay = 290.f;

private:
	float PhaBuoc = 0.f;
	float CaoMat = 162.f;
	float KhoangNam = 150.f;
	FRotator XoayNam = FRotator::ZeroRotator;
};
