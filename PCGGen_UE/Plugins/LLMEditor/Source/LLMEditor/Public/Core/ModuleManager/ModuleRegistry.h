#pragma once
#include "CoreMinimal.h"
#include "IModuleBase.h"

typedef IModuleBase* (*FModuleFactoryFunc)();

class FModuleRegistry
{
public:
	static FModuleRegistry& Get()
	{
		static FModuleRegistry Instance;
		return Instance;
	}

	void RegisterModule(FName Name, FModuleFactoryFunc Factory)
	{
		if (!Factories.Contains(Name))
		{
			Factories.Add(Name, Factory);
		}
	}

	IModuleBase* CreateModule(FName Name)
	{
		if (Factories.Contains(Name))
		{
			return Factories[Name]();
		}
		return nullptr;
	}

private:
	TMap<FName, FModuleFactoryFunc> Factories;
};
