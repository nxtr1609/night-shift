#include "VatLieu.h"

#include "Animation/AnimSequence.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"

namespace
{
	UMaterialInterface* NapVatLieu(const TCHAR* Duong)
	{
		return LoadObject<UMaterialInterface>(nullptr, Duong, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	UMaterialInstanceDynamic* Tao(UObject* Chu, const TCHAR* Duong)
	{
		UMaterialInterface* G = NapVatLieu(Duong);
		if (!G) G = NapVatLieu(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		return UMaterialInstanceDynamic::Create(G, Chu);
	}
}

UMaterialInstanceDynamic* VatLieu::Mau(UObject* Chu, const FLinearColor& C, float DoNham, float KimLoai)
{
	UMaterialInstanceDynamic* M = Tao(Chu, TEXT("/Game/VatLieu/M_Mau.M_Mau"));
	M->SetVectorParameterValue(TEXT("MauSac"), C);
	M->SetScalarParameterValue(TEXT("DoNham"), DoNham);
	M->SetScalarParameterValue(TEXT("KimLoai"), KimLoai);
	return M;
}

UMaterialInstanceDynamic* VatLieu::BeMat(UObject* Chu, const FString& TenAnh, const FLinearColor& Nhan, float DoNham, float PhatSang)
{
	UMaterialInstanceDynamic* M = Tao(Chu, TEXT("/Game/VatLieu/M_BeMat.M_BeMat"));
	if (UTexture2D* T = Anh(TenAnh)) M->SetTextureParameterValue(TEXT("Anh"), T);
	M->SetVectorParameterValue(TEXT("Nhan"), Nhan);
	M->SetScalarParameterValue(TEXT("DoNham"), DoNham);
	M->SetScalarParameterValue(TEXT("PhatSang"), PhatSang);
	return M;
}

UMaterialInstanceDynamic* VatLieu::Sang(UObject* Chu, const FLinearColor& C, float DoSang)
{
	UMaterialInstanceDynamic* M = Tao(Chu, TEXT("/Game/VatLieu/M_Sang.M_Sang"));
	M->SetVectorParameterValue(TEXT("MauSac"), C);
	M->SetScalarParameterValue(TEXT("DoSang"), DoSang);
	return M;
}

UMaterialInstanceDynamic* VatLieu::PhuMo(UObject* Chu, const FLinearColor& C, float DoMo)
{
	UMaterialInstanceDynamic* M = Tao(Chu, TEXT("/Game/VatLieu/M_PhuMo.M_PhuMo"));
	M->SetVectorParameterValue(TEXT("MauSac"), C);
	M->SetScalarParameterValue(TEXT("DoMo"), DoMo);
	return M;
}

UMaterialInstanceDynamic* VatLieu::Decal(UObject* Chu, const FString& TenAnh, float DoMo)
{
	UMaterialInstanceDynamic* M = Tao(Chu, TEXT("/Game/VatLieu/M_Decal.M_Decal"));
	if (UTexture2D* T = Anh(TenAnh)) M->SetTextureParameterValue(TEXT("Anh"), T);
	M->SetScalarParameterValue(TEXT("DoMo"), DoMo);
	return M;
}

UMaterialInstanceDynamic* VatLieu::NhieuTV(UObject* Chu)
{
	return Tao(Chu, TEXT("/Game/VatLieu/M_NhieuTV.M_NhieuTV"));
}

UMaterialInstanceDynamic* VatLieu::Kinh(UObject* Chu)
{
	return Tao(Chu, TEXT("/Game/VatLieu/M_Kinh.M_Kinh"));
}

UMaterialInstanceDynamic* VatLieu::InKinh(UObject* Chu, const FString& TenAnh, float DoMo)
{
	UMaterialInstanceDynamic* M = Tao(Chu, TEXT("/Game/VatLieu/M_InKinh.M_InKinh"));
	if (UTexture2D* T = Anh(TenAnh)) M->SetTextureParameterValue(TEXT("Anh"), T);
	M->SetScalarParameterValue(TEXT("DoMo"), DoMo);
	return M;
}

UTexture2D* VatLieu::Anh(const FString& Ten)
{
	return LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Anh/T_%s.T_%s"), *Ten, *Ten), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

UStaticMesh* VatLieu::Khoi() { return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")); }
UStaticMesh* VatLieu::Tru() { return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")); }
UStaticMesh* VatLieu::Cau() { return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")); }

UStaticMesh* VatLieu::MauTinh(const FString& Ten)
{
	static TMap<FString, UStaticMesh*> Nho;
	if (UStaticMesh** Co = Nho.Find(Ten)) return *Co;
	IAssetRegistry& AR = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	const FString ThuMuc = TEXT("/Game/Mau/") + Ten;
	TArray<FAssetData> DS;
	AR.ScanPathsSynchronous({ ThuMuc }, false);
	AR.GetAssetsByPath(FName(*ThuMuc), DS, true);
	DS.Sort([](const FAssetData& A, const FAssetData& B) { return A.AssetName.LexicalLess(B.AssetName); });
	UStaticMesh* Ket = nullptr;
	for (const FAssetData& D : DS)
		if (D.AssetClassPath == UStaticMesh::StaticClass()->GetClassPathName())
			if ((Ket = Cast<UStaticMesh>(D.GetAsset())) != nullptr) break;
	if (!Ket) UE_LOG(LogTemp, Warning, TEXT("Thieu mau %s"), *Ten);
	Nho.Add(Ten, Ket);
	return Ket;
}

USoundBase* VatLieu::AmThanh(const FString& Ten)
{
	return LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/AmThanh/%s.%s"), *Ten, *Ten), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

USkeletalMesh* VatLieu::Manny()
{
	return LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
}

UAnimSequence* VatLieu::HoatAnh(const TCHAR* Duong)
{
	return LoadObject<UAnimSequence>(nullptr, Duong);
}
