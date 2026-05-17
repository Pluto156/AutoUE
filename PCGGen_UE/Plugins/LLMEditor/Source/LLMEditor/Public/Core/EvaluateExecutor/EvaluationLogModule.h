#pragma once

#include "CoreMinimal.h"
#include "IModuleBase.h"

/**
 * 评估日志模块
 * 接收字符串并按行写入 txt 文件
 */
class FEvaluationLogModule : public IModuleBase
{
public:
    virtual void Initialize() override
    {
        UE_LOG(LogTemp, Log, TEXT("[EvaluationLogModule] Initialized"));
        EnsureLogDirectory();
        ClearOldLogFile();
    }

    virtual void Shutdown() override
    {
        UE_LOG(LogTemp, Log, TEXT("[EvaluationLogModule] Shutdown"));
    }

    static FName StaticModuleName()
    {
        return TEXT("EvaluationLogModule");
    }

public:
    /** 
     * 写入一行评估日志
     * 每次调用追加一行
     */
    void WriteEvalLog(const FString& Line);

private:
    /** 确保日志目录存在 */
    void EnsureLogDirectory() const;
    /** 初始化时清空并删除旧日志文件 */
    void ClearOldLogFile() const;

    /** 获取日志文件完整路径 */
    FString GetLogFilePath() const;
};
