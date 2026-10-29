// "Người đeo mặt nạ": hình người cao gầy tối màu (mannequin của engine) đeo mặt nạ trắng.
// Đạo diễn ra lệnh: đứng im, đi tới điểm, rượt theo người chơi dọc hành lang, biến mất.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BongNguoi.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;
class UAnimSequence;

UENUM()
enum class EBong : uint8 { An, Dung, Di, Duoi, Bo, CoGiat };

UCLASS()
class ABongNguoi : public AActor
{
	GENERATED_BODY()

public:
	ABongNguoi();
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

	void An();
	// Đứng im tại chỗ, quay mặt theo Yaw (bDongBang: tư thế cứng đờ, không thở)
	void Dung(const FVector& ViTri, float Yaw, bool bDongBang = true);
	// Đi thẳng tới Dich với tốc độ cm/s rồi đứng (hoặc biến mất khi tới nếu bAnKhiToi)
	void Di(const FVector& Tu, const FVector& Dich, float Toc, bool bAnKhiToi, bool bChay = false);
	// Rượt theo dọc đường giữa hành lang, bắt đầu từ quãng đường S
	void Duoi(float S, float Toc);
	// Bò ngược lên mặt kính bên ngoài cửa sổ (đi thẳng từ Tu tới Toi, tứ chi cử động), tới nơi thì biến mất
	void Bo(const FVector& Tu, const FVector& Toi, float Toc, float Yaw);
	// Đứng co giật tại chỗ (giật tư thế, giật đầu, lắc người)
	void CoGiat(const FVector& ViTri, float Yaw);
	// Chỉ hiện trong ảnh chụp cảnh (không dùng) — để trống
	bool DangHien() const { return CheDo != EBong::An; }
	float QuangDuong = 0.f;   // khi rượt
	EBong CheDo = EBong::An;

	UPROPERTY() USkeletalMeshComponent* Than;
	UPROPERTY() UStaticMeshComponent* MatNa;
	UPROPERTY() class UPointLightComponent* AnhChieu;

private:
	void ChoiHoatAnh(UAnimSequence* A, float TocDoPhat);
	void DatMatNa();
	void AnChieu() {}
	static constexpr float GocMa = -90.f;   // mẫu nhìn theo +Y → xoay để nhìn theo +X của actor
	float HuongMatNa = 0.f;
	bool bCoMatNa = true;
	// Mẫu bóng ma (Ghost Girl Animated) — có thì dùng thay mannequin đeo mặt nạ
	bool bMa = false;
	float DaiMa = 1.f;   // độ dài hoạt ảnh (giây)
	UPROPERTY() UAnimSequence* AnhMa;
	UPROPERTY() UAnimSequence* AnhDung;
	UPROPERTY() UAnimSequence* AnhDi;
	UPROPERTY() UAnimSequence* AnhChay;
	FVector Dich = FVector::ZeroVector;
	float Toc = 0.f;
	bool bAnKhiToi = false;
	FVector ViTriGoc = FVector::ZeroVector;
	float YawGoc = 0.f;
	float ChoGiat = 0.f;
};
