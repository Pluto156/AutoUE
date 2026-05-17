#pragma once

#include "CoreMinimal.h"
#include "IModuleBase.h"
#include "BattleInterface.h"
#include "BattleStatComponent.h"

/**
 * 简单战斗模块
 * 提供 ApplyDamageTo 功能，供其他模块或系统调用
 */
class FBattleModule : public IModuleBase
{
public:
    virtual void Initialize() override
    {
        UE_LOG(LogTemp, Log, TEXT("[BattleModule] Initialized"));
    }

    virtual void Shutdown() override
    {
        UE_LOG(LogTemp, Log, TEXT("[BattleModule] Shutdown"));
    }

    static FName StaticModuleName() { return TEXT("BattleModule"); }

public:
    /**
     * 应用伤害逻辑
     * @param Attacker  攻击者（实现 IBattleInterface）
     * @param Defender  防御者（实现 IBattleInterface）
     * @param Damage    伤害值
     */
    void ApplyDamageTo(TScriptInterface<IBattleInterface> Attacker, TScriptInterface<IBattleInterface> Defender, float Damage)
    {
        if (!Defender)
        {
            UE_LOG(LogTemp, Warning, TEXT("[BattleModule] Invalid defender."));
            return;
        }

        UBattleStatComponent* DefenderStats = Defender->GetBattleStatComponent();
        if (!DefenderStats)
        {
            UE_LOG(LogTemp, Warning, TEXT("[BattleModule] Defender has no BattleStatComponent."));
            return;
        }

        UE_LOG(LogTemp, Log, TEXT("[BattleModule] %s attacks %s for %.1f damage."),
            Attacker ? *Attacker.GetObject()->GetName() : TEXT("Unknown"),
            *Defender.GetObject()->GetName(),
            Damage);

        DefenderStats->ApplyDamage(Damage, Attacker ? Attacker.GetObject() : nullptr);
    }
};
