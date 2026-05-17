#pragma once
#include "CoreMinimal.h"
#include "ModuleRegistry.h"

class FMyModuleManager
{
public:
	static FMyModuleManager& Instance()
	{
		static FMyModuleManager Singleton;
		return Singleton;
	}

	template<typename T>
	T* GetModule()
	{
		FName Name = T::StaticModuleName();
		if (!LoadedModules.Contains(Name))
		{
			IModuleBase* Module = FModuleRegistry::Get().CreateModule(Name);
			if (Module)
			{
				Module->Initialize();
				LoadedModules.Add(Name, Module);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[ModuleManager] Failed to create module: %s"), *Name.ToString());
				return nullptr;
			}
		}
		return static_cast<T*>(LoadedModules[Name]); // ✅ 用 static_cast 替换 Cast
	}

	void ShutdownAll()
	{
		for (auto& Pair : LoadedModules)
		{
			if (Pair.Value)
			{
				Pair.Value->Shutdown();
				delete Pair.Value;
			}
		}
		LoadedModules.Empty();
	}

private:
	FMyModuleManager() {}
	TMap<FName, IModuleBase*> LoadedModules;
};
