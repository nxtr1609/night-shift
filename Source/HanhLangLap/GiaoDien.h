// Giao diện vẽ bằng Canvas: menu chính, tạm dừng, chấm ngắm, gợi ý, phụ đề, màn đen / trắng, màn kết thúc.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GiaoDien.generated.h"

UCLASS()
class AGiaoDien : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
	// Tên nút tại điểm (rỗng nếu không trúng nút nào)
	FString NutTai(const FVector2D& P) const;

private:
	struct FNut { FBox2D Khung; FString Ten; };
	TArray<FNut> Nut;
	void Chu(const FString& S, float X, float Y, float Co, const FLinearColor& Mau, bool bGiua = true, bool bBong = true);
	void VeNut(const FString& Ten, const FString& Chu, float Y);
	FVector2D Chuot() const;
};
