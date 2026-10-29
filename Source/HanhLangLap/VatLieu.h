// Nạp vật liệu, texture, mẫu và âm thanh dùng chung.
#pragma once

#include "CoreMinimal.h"

class UMaterialInstanceDynamic;
class UStaticMesh;
class USkeletalMesh;
class UTexture2D;
class USoundBase;
class UAnimSequence;

namespace VatLieu
{
	UMaterialInstanceDynamic* Mau(UObject* Chu, const FLinearColor& C, float DoNham = 0.7f, float KimLoai = 0.f);
	// Texture /Game/Anh/T_<TenAnh> nhân màu; PhatSang > 0 thì texture tự sáng (biển, mặt tiền)
	UMaterialInstanceDynamic* BeMat(UObject* Chu, const FString& TenAnh, const FLinearColor& Nhan = FLinearColor::White, float DoNham = 0.7f, float PhatSang = 0.f);
	UMaterialInstanceDynamic* Sang(UObject* Chu, const FLinearColor& C, float DoSang);
	UMaterialInstanceDynamic* PhuMo(UObject* Chu, const FLinearColor& C, float DoMo);
	UMaterialInstanceDynamic* Decal(UObject* Chu, const FString& TenAnh, float DoMo = 1.f);
	UMaterialInstanceDynamic* NhieuTV(UObject* Chu);
	// Kính cửa sổ (Fresnel + vệt mưa) và vết in trên kính (dấu tay)
	UMaterialInstanceDynamic* Kinh(UObject* Chu);
	UMaterialInstanceDynamic* InKinh(UObject* Chu, const FString& TenAnh, float DoMo = 1.f);

	UTexture2D* Anh(const FString& Ten);
	UStaticMesh* Khoi();
	UStaticMesh* Tru();
	UStaticMesh* Cau();
	// Lưới đầu tiên trong /Game/Mau/<Ten> (mẫu SimReady đã chuyển sang GLB, đơn vị cm)
	UStaticMesh* MauTinh(const FString& Ten);
	USoundBase* AmThanh(const FString& Ten);
	USkeletalMesh* Manny();
	UAnimSequence* HoatAnh(const TCHAR* Duong);
}
