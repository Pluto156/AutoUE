#define REGISTER_MODULE(ModuleClass) \
	static IModuleBase* Create##ModuleClass() { return new ModuleClass(); } \
	struct FAutoRegister##ModuleClass { \
		FAutoRegister##ModuleClass() { \
			FModuleRegistry::Get().RegisterModule(ModuleClass::StaticModuleName(), &Create##ModuleClass); \
		} \
	}; \
	static FAutoRegister##ModuleClass AutoRegister##ModuleClass##Instance;
