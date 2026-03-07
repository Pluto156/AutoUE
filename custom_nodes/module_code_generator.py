import os
import json
import re
import shutil
from typing import Dict, Any, List

from core.BaseLLMNode import BaseLLMNode
from core.BaseLLMNode import GraphState

# ============================================================
# Output directory configuration
# ============================================================
MODULE_OUTPUT_DIR = os.path.join(
    os.getenv("UE_Plugins"),
    "LLMEditor",
    "Source",
    "LLMEditor",
    "Private",
    "LLMGenerate",
    "Module"
)

# ============================================================
# Utility: clear a directory
# ============================================================
def clear_directory(directory: str):
    """Clear all files and subdirectories inside the specified directory."""
    if os.path.exists(directory):
        for filename in os.listdir(directory):
            file_path = os.path.join(directory, filename)
            try:
                if os.path.isdir(file_path):
                    shutil.rmtree(file_path)  # Remove directory and its contents
                else:
                    os.remove(file_path)  # Remove file
            except Exception as e:
                print(f"[ERROR] Failed to delete file or directory {file_path}: {e}")
    else:
        print(f"[WARNING] Directory {directory} does not exist, skipping cleanup.")

# ============================================================
# Post-processing: parse JSON and write .h / .cpp files
#    Also fixes common include name mistakes
#    (e.g. AInventoryModule.h -> InventoryModule.h)
# ============================================================
def write_module_files(state, output: str):
    """Parse the model JSON output and write each module into its own folder."""
    print("\n[DEBUG] Executing ModuleCodeGenerator.post_action ...")

    try:
        cleaned = output.strip()
        cleaned = re.sub(r"^```(?:json)?", "", cleaned.strip())
        cleaned = re.sub(r"```$", "", cleaned.strip())
        data = json.loads(cleaned)
    except Exception as e:
        print(f"[ERROR] Failed to parse model output as JSON: {e}")
        print("[DEBUG] Raw output:\n", output)
        return

    modules = data.get("modules", [])
    if not modules:
        print("[WARN] No module data found.")
        return

    # Clear old output
    clear_directory(MODULE_OUTPUT_DIR)
    os.makedirs(MODULE_OUTPUT_DIR, exist_ok=True)

    for module in modules:
        name = module.get("module_name", "").strip()
        header_code = module.get("header_code", "")
        source_code = module.get("source_code", "")

        if not name:
            print("[WARN] Skipping an unnamed module.")
            continue

        # Fix common include mistakes:
        # If the model mistakenly outputs A{Name}.h or A{Name}.generated.h,
        # fix them to {Name}.h / {Name}.generated.h.
        # Also handles cases where an 'A' prefix appears in other include contexts.
        def fix_includes(text, module_name):
            if not isinstance(text, str):
                return text
            # Direct replacements for common mistakes
            text = text.replace(f'#include "A{module_name}.h"', f'#include "{module_name}.h"')
            text = text.replace(
                f'#include "A{module_name}.generated.h"',
                f'#include "{module_name}.generated.h"'
            )
            # Generic A<Something>.h -> <Something>.h (applied cautiously)
            text = re.sub(r'#include\s+"A([A-Za-z0-9_]+\.h)"', r'#include "\1"', text)
            text = re.sub(
                r'#include\s+"A([A-Za-z0-9_]+\.generated\.h)"',
                r'#include "\1"',
                text
            )
            # Also fix cases like AInventoryModule.h -> InventoryModule.h
            text = re.sub(
                r'#include\s+"A([A-Za-z0-9_]+Module\.h)"',
                r'#include "\1"',
                text
            )
            return text

        header_code = fix_includes(header_code, name)
        source_code = fix_includes(source_code, name)

        # Create module directory
        module_dir = os.path.join(MODULE_OUTPUT_DIR, name)
        os.makedirs(module_dir, exist_ok=True)

        header_path = os.path.join(module_dir, f"{name}.h")
        source_path = os.path.join(module_dir, f"{name}.cpp")

        try:
            with open(header_path, "w", encoding="utf-8") as f:
                f.write(header_code.strip() + "\n")
            print(f"[OK] Header generated: {header_path}")

            with open(source_path, "w", encoding="utf-8") as f:
                f.write(source_code.strip() + "\n")
            print(f"[OK] Source generated: {source_path}")

        except Exception as e:
            print(f"[ERROR] Failed to write files for module {name}: {e}")

    print("[DEBUG] All module files generated successfully.")


class ModuleCodeGeneratorNode(BaseLLMNode):

    def call_model(self, full_input: str, state: GraphState):

        try:
            analyzer_raw = state.llm_outputs.get("ModuleAnalyzer", "")
            cleaned = re.sub(r"^```(?:json)?|```$", "", analyzer_raw.strip())
            analyzer_json = json.loads(cleaned)
            core_modules = analyzer_json.get("core_modules", [])
        except Exception as e:
            print(f"[ERROR] Failed to parse ModuleAnalyzer output: {e}")
            return "{}"

        print(f"[DEBUG] Detected {len(core_modules)} modules")

        final_modules: List[Dict[str, Any]] = []

        # Store generated dependency module code
        generated_module_code: Dict[str, Dict[str, str]] = {}

        # ====================================================
        # Generate modules in dependency-safe order
        # ====================================================
        for idx, module in enumerate(core_modules):
            module_name = module.get("module_name", "Unknown")
            dependencies = module.get("dependencies", [])

            print(f"[LLM] Generating {idx + 1}/{len(core_modules)}: {module_name}")

            # ------------------------------------------------
            # Inject dependency module code (if any)
            # ------------------------------------------------
            dependency_context = ""
            if dependencies:
                dependency_context += (
                    "\n\n[Dependency Module Code Reference]\n"
                    "The following modules are already implemented and may be used.\n"
                    "DO NOT reimplement them. ONLY call their public APIs.\n"
                )

                for dep in dependencies:
                    dep_code = generated_module_code.get(dep)
                    if not dep_code:
                        continue

                    dependency_context += f"\n--- {dep}.h ---\n"
                    dependency_context += dep_code["header_code"]
                    dependency_context += f"\n--- {dep}.cpp ---\n"
                    dependency_context += dep_code["source_code"]

            # ------------------------------------------------
            # Single-module input
            # ------------------------------------------------
            single_input = json.dumps(
                {
                    "game_concept": analyzer_json.get("game_concept", ""),
                    "core_modules": [module]
                },
                ensure_ascii=False,
                indent=2
            )

            response = self.invoke_model_with_token_tracking([
                {"role": "system", "content": self.prompt},
                {"role": "system", "content": dependency_context},
                {"role": "user", "content": single_input}
            ],state)

            result_text = re.sub(r"^```(?:json)?|```$", "", response.content.strip())

            try:
                data = json.loads(result_text)
                mods = data.get("modules", [])
                if mods:
                    mod = mods[0]
                    final_modules.append(mod)

                    # Cache generated code for dependency injection
                    generated_module_code[module_name] = {
                        "header_code": mod.get("header_code", ""),
                        "source_code": mod.get("source_code", "")
                    }

            except Exception as e:
                print(f"[ERROR] Failed to parse output for {module_name}: {e}")
                print(result_text)

        return json.dumps({"modules": final_modules}, indent=2, ensure_ascii=False)

# ============================================================
# Module code generation prompt
#    (Strengthened: explicitly forbids using 'A' prefix in include filenames)
# ============================================================
MODULE_CODE_GENERATOR_PROMPT = """
You are a senior Unreal Engine 5 C++ module development engineer.

Your task is to generate the corresponding C++ implementation files (.h / .cpp)
for each module based on the JSON output from the previous node (ModuleAnalyzer).

All generated module code MUST strictly follow the format below.
You must NOT add or remove blank lines, and must NOT change function order
or indentation style.

IMPORTANT EXPORT RULE (CRITICAL):
All generated module classes are part of the LLMEditor plugin public API.
Therefore, EVERY module class MUST be exported using the `LLMEDITOR_API` macro.

Correct example:
class LLMEDITOR_API FInventoryModule : public IModuleBase

Missing the export macro is considered a critical error.

-------------------------------------------------------------
[Code Template Example] — MUST strictly follow this format
-------------------------------------------------------------

CustomModule.h:
#pragma once
#include "CoreMinimal.h"
#include "IModuleBase.h"

/**
 * Simple inventory module
 * Manages item name -> count
 */
class LLMEDITOR_API FCustomModule : public IModuleBase
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
    /** Add item interface: increment count and print current inventory */
    void AddItem(const FString& ItemName);

    /** Print current inventory contents */
    void PrintInventory() const;

private:
    /** Stored inventory items */
    TMap<FString, int32> Items;
};

-------------------------------------------------------------

CustomModule.cpp:
#include "CustomModule.h"
#include "ModuleRegistry.h"
#include "ModuleMacro.h"
#include "MyModuleManager.h"
#include "EvaluationLogModule.h"
#include "CustomModules.h"
void FCustomModule::AddItem(const FString& ItemName)
{
    auto& Manager = FMyModuleManager::Instance();
    auto* EvalLog = Manager.GetModule<FEvaluationLogModule>();

    if (ItemName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[CustomModule] Invalid item name."));

        if (EvalLog)
        {
            EvalLog->WriteEvalLog(
                TEXT("[CustomModule] AddItem failed: invalid item name."));
        }
        return;
    }

    int32& Count = Items.FindOrAdd(ItemName);
    Count++;

    UE_LOG(LogTemp, Log,
        TEXT("[CustomModule] Added item: %s (x%d)"),
        *ItemName, Count);

    if (EvalLog)
    {
        EvalLog->WriteEvalLog(
            FString::Printf(
                TEXT("[CustomModule] Item added: %s, new count = %d."),
                *ItemName,
                Count));
    }

    PrintInventory();
}

void FCustomModule::PrintInventory() const
{
    auto& Manager = FMyModuleManager::Instance();
    auto* EvalLog = Manager.GetModule<FEvaluationLogModule>();

    if (Items.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[CustomModule] Inventory is empty."));

        if (EvalLog)
        {
            EvalLog->WriteEvalLog(
                TEXT("[CustomModule] Inventory is empty."));
        }
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("=== Current Inventory ==="));

    if (EvalLog)
    {
        EvalLog->WriteEvalLog(TEXT("[CustomModule] Inventory snapshot begin."));
    }

    for (const auto& Pair : Items)
    {
        UE_LOG(LogTemp, Log,
            TEXT(" - %s : %d"),
            *Pair.Key,
            Pair.Value);

        if (EvalLog)
        {
            EvalLog->WriteEvalLog(
                FString::Printf(
                    TEXT("[CustomModule] Inventory entry: %s = %d."),
                    *Pair.Key,
                    Pair.Value));
        }
    }

    UE_LOG(LogTemp, Log, TEXT("=========================="));

    if (EvalLog)
    {
        EvalLog->WriteEvalLog(TEXT("[CustomModule] Inventory snapshot end."));
    }
}

/** Register module */
REGISTER_MODULE(FCustomModule)

-------------------------------------------------------------
[Important Naming / Include Rules]
-------------------------------------------------------------
1. The .cpp file MUST include ONLY the following headers
   and MUST NOT include any other module headers:

#include "<ModuleName>.h"
#include "ModuleRegistry.h"
#include "ModuleMacro.h"
#include "MyModuleManager.h"
#include "EvaluationLogModule.h"
#include "CustomModules.h"

2. You MUST NOT include:
   - Any other module headers
   - Any dependency module headers
   - Any forward includes to other modules

3. All cross-module access MUST rely on:
   #include "CustomModules.h"
4. Module class names MUST use the `F` prefix (e.g. FInventoryModule).
   The `A` prefix is strictly forbidden.
5. All module classes MUST include `LLMEDITOR_API`.
6. Header and source filenames MUST match the module name:
   InventoryModule.h / InventoryModule.cpp
7. All #include directives MUST reference the exact filename.
   Example:
     #include "InventoryModule.h"  ✅
     #include "AInventoryModule.h" ❌
8. Formatting, indentation, comments, and blank lines MUST match
   the example exactly. Only names and business logic may differ.
9. Output MUST be strict JSON only, with no extra text:
{
  "modules": [
    {
      "module_name": "InventoryModule",
      "header_code": "Full .h file content",
      "source_code": "Full .cpp file content"
    }
  ]
}
"""

def create_module_code_generator() -> ModuleCodeGeneratorNode:
    node = ModuleCodeGeneratorNode(
        name="ModuleCodeGenerator",
        prompt=MODULE_CODE_GENERATOR_PROMPT,
        post_action=write_module_files,
        enable_feedback=False
    )
    return node

# ============================================================
# Node instance
# ============================================================
module_code_generator = ModuleCodeGeneratorNode(
    name="ModuleCodeGenerator",
    prompt=MODULE_CODE_GENERATOR_PROMPT,
    post_action=write_module_files,
    enable_feedback=False
)
