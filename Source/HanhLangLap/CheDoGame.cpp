#include "CheDoGame.h"

#include "DaoDien.h"
#include "DieuKhien.h"
#include "GiaoDien.h"
#include "Misc/CommandLine.h"
#include "NguoiChoi.h"
#include "TuKiem.h"

ACheDoGame::ACheDoGame()
{
	DefaultPawnClass = ANguoiChoi::StaticClass();
	PlayerControllerClass = ADieuKhien::StaticClass();
	HUDClass = AGiaoDien::StaticClass();
}

void ACheDoGame::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	ADaoDien* DD = GetWorld()->SpawnActor<ADaoDien>();
	// -autotest=<thư mục> [-che_do=choi|anh] [-vong=N]: máy tự chơi / chụp ảnh từng vòng, ghi KetQua.txt rồi thoát
	FString ThuMuc;
	if (FParse::Value(FCommandLine::Get(), TEXT("autotest="), ThuMuc))
	{
		DD->bTuKiem = true;
		ATuKiem* TK = GetWorld()->SpawnActor<ATuKiem>();
		TK->ThuMuc = ThuMuc;
		FParse::Value(FCommandLine::Get(), TEXT("che_do="), TK->CheDo);
		FParse::Value(FCommandLine::Get(), TEXT("vong="), TK->VongDau);
	}
}

void ACheDoGame::RestartPlayer(AController* NewPlayer)
{
	RestartPlayerAtTransform(NewPlayer, FTransform(FRotator::ZeroRotator, FVector(80.f, 0.f, 92.f)));
}
