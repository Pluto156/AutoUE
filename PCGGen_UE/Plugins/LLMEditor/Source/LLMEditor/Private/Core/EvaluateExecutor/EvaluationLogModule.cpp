#include "EvaluationLogModule.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "ModuleRegistry.h"
#include "ModuleMacro.h"

void FEvaluationLogModule::EnsureLogDirectory() const
{
    const FString DirPath =
        FPaths::ProjectContentDir() / TEXT("MyPCG/eval");

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

    if (!PlatformFile.DirectoryExists(*DirPath))
    {
        PlatformFile.CreateDirectoryTree(*DirPath);
    }
}

void FEvaluationLogModule::ClearOldLogFile() const
{
    const FString FilePath = GetLogFilePath();

    IPlatformFile& PlatformFile =
        FPlatformFileManager::Get().GetPlatformFile();

    if (PlatformFile.FileExists(*FilePath))
    {
        if (PlatformFile.DeleteFile(*FilePath))
        {
            UE_LOG(LogTemp, Log,
                TEXT("[EvaluationLogModule] Old evaluation log deleted."));
        }
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[EvaluationLogModule] Failed to delete old evaluation log."));
        }
    }
}


FString FEvaluationLogModule::GetLogFilePath() const
{
    return FPaths::ProjectContentDir()
        / TEXT("MyPCG/eval/EvaluationLog.txt");
}

void FEvaluationLogModule::WriteEvalLog(const FString& Line)
{
    if (Line.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[EvaluationLogModule] Empty log line."));
        return;
    }

    EnsureLogDirectory();

    const FString LogLine = Line + LINE_TERMINATOR;
    const FString FilePath = GetLogFilePath();

    if (!FFileHelper::SaveStringToFile(
        LogLine,
        *FilePath,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
        &IFileManager::Get(),
        FILEWRITE_Append))
    {
        UE_LOG(LogTemp, Error, TEXT("[EvaluationLogModule] Failed to write log."));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[EvaluationLogModule] Log written: %s"), *Line);
}


/** 注册模块 */
REGISTER_MODULE(FEvaluationLogModule)
