#pragma once
#include "CoreMinimal.h"
#include "IModuleBase.h"
/**
 * 简单背包模块
 * 管理物品名称 -> 数量
 */
class FCustomModule : public IModuleBase
{
public:
    virtual void Initialize() override
    {
        UE_LOG(LogTemp, Log, TEXT("[CustomModule] Initialized"));
    }

    virtual void Shutdown() override
    {
        UE_LOG(LogTemp, Log, TEXT("[CustomModule] Shutdown"));
        Items.Empty();
    }
    static FName StaticModuleName() { return TEXT("CustomModule"); }
public:
    /** 添加物品接口：计数 +1 并打印当前物品列表 */
    void AddItem(const FString& ItemName);

    /** 打印当前背包内容 */
    void PrintInventory() const;
private:
    /** 存储背包物品 */
    TMap<FString, int32> Items;
};
