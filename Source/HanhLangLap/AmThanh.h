// Phát âm thanh (tạo bằng Stable Audio 3 qua Audio Studio, nhập ở /Game/AmThanh).
#pragma once

#include "CoreMinimal.h"

class UAudioComponent;
class USceneComponent;

namespace AmThanh
{
	void Phat2D(const UObject* Ngu, const FString& Ten, float Am = 1.f, float Cao = 1.f);
	void Phat3D(const UObject* Ngu, const FString& Ten, const FVector& ViTri, float Am = 1.f, float Cao = 1.f);
	// Âm lặp gắn vào actor; Gan = nullptr là âm 2D (nền). Tạo xong ở trạng thái tắt (âm lượng 0).
	UAudioComponent* TaoVong(AActor* Chu, const FString& Ten, USceneComponent* Gan, float BanKinh = 1500.f);
	// Đổi âm lượng mượt (bật nếu đang tắt)
	void DatAm(UAudioComponent* A, float Am, float GiayChuyen = 1.f);
}
