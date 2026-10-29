#include "AmThanh.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "VatLieu.h"

namespace
{
	USoundAttenuation* SuyGiam(float BanKinh)
	{
		static TMap<int32, TStrongObjectPtr<USoundAttenuation>> Nho;
		const int32 Khoa = FMath::RoundToInt(BanKinh);
		if (TStrongObjectPtr<USoundAttenuation>* Co = Nho.Find(Khoa)) return Co->Get();
		USoundAttenuation* S = NewObject<USoundAttenuation>();
		S->Attenuation.bAttenuate = true;
		S->Attenuation.bSpatialize = true;
		S->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
		S->Attenuation.AttenuationShapeExtents = FVector(120.f, 0.f, 0.f);
		S->Attenuation.FalloffDistance = BanKinh;
		S->Attenuation.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
		Nho.Add(Khoa, TStrongObjectPtr<USoundAttenuation>(S));
		return S;
	}
}

void AmThanh::Phat2D(const UObject* Ngu, const FString& Ten, float Am, float Cao)
{
	if (USoundBase* S = VatLieu::AmThanh(Ten)) UGameplayStatics::PlaySound2D(Ngu, S, Am, Cao);
}

void AmThanh::Phat3D(const UObject* Ngu, const FString& Ten, const FVector& ViTri, float Am, float Cao)
{
	if (USoundBase* S = VatLieu::AmThanh(Ten)) UGameplayStatics::PlaySoundAtLocation(Ngu, S, ViTri, Am, Cao, 0.f, SuyGiam(1500.f));
}

UAudioComponent* AmThanh::TaoVong(AActor* Chu, const FString& Ten, USceneComponent* Gan, float BanKinh)
{
	UAudioComponent* A = NewObject<UAudioComponent>(Chu);
	A->SetSound(VatLieu::AmThanh(Ten));
	A->bAutoActivate = false;
	A->bAllowSpatialization = Gan != nullptr;
	A->bIsUISound = false;
	if (Gan)
	{
		A->SetupAttachment(Gan);
		A->AttenuationSettings = SuyGiam(BanKinh);
	}
	else
	{
		A->SetupAttachment(Chu->GetRootComponent());
	}
	A->RegisterComponent();
	return A;
}

void AmThanh::DatAm(UAudioComponent* A, float Am, float GiayChuyen)
{
	if (!A || !A->Sound) return;
	if (Am <= 0.001f)
	{
		if (A->IsPlaying()) A->FadeOut(FMath::Max(GiayChuyen, 0.05f), 0.f);
		return;
	}
	if (!A->IsPlaying()) A->FadeIn(FMath::Max(GiayChuyen, 0.05f), Am);
	else A->AdjustVolume(FMath::Max(GiayChuyen, 0.05f), Am);
}
