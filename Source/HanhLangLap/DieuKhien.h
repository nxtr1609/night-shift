// Điều khiển: WASD đi, chuột nhìn, giữ Shift chạy, E / chuột trái tương tác, Esc tạm dừng. Menu bấm chuột.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DieuKhien.generated.h"

UCLASS()
class ADieuKhien : public APlayerController
{
	GENERATED_BODY()

public:
	ADieuKhien();
	virtual void BeginPlay() override;
	virtual void PlayerTick(float Dt) override;

	// Máy tự chơi điều khiển thay (tắt đọc phím chuột)
	bool bMayChoi = false;
	float DoNhay = 1.f;   // độ nhạy chuột

	void BamNut(const FString& Nut);

private:
	void DatCheDoChuot(bool bHienChuot);
	bool bDangHienChuot = false;
};
