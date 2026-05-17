#pragma once
#include "CoreMinimal.h"

class IModuleBase
{
public:
	virtual ~IModuleBase() {}
	virtual void Initialize() {}
	virtual void Shutdown() {}

	/** 每个模块必须实现一个唯一的名字 */
	static FName StaticModuleName() { return NAME_None; }
};
