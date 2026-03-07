# ============================================================
# File: interactive_object_code_generator.py
# Description:
#   Automatically generates .h / .cpp files for each interactive object
#   based on outputs from ModuleCodeGenerator + InteractiveObjectAnalyzer.
# ============================================================

import os
import json
import re
import shutil
from typing import Dict, Any, List

from core.BaseLLMNode import BaseLLMNode
from core.BaseLLMNode import GraphState

# ============================================================
# Prompt (Revised: always use the correct Manager access pattern and accept two input sources)
#    Additional rule:
#    Every model produced by InteractiveObjectAnalyzer MUST generate a corresponding
#    interactive object class. Even if a model has no interaction flow, a class must
#    still be generated with an empty HandleInteraction implementation
#    (at minimum calling Super).
# ============================================================
INTERACTIVE_OBJECT_CODE_GENERATOR_PROMPT = """
You are an Unreal Engine 5 C++ programmer.

Your ONLY input consists of two parts (already concatenated by the system):

1) ModuleCodeGenerator output (JSON), format:
{
  "modules": [
    {
      "module_name": "InventoryModule",
      "header_code": "...",
      "source_code": "..."
    }
  ]
}

2) InteractiveObjectAnalyzer output (JSON), format:
{
  "interactive_objects": [
    {
      "model_name": "Chest",
      "interaction_summary": "The player opens the chest and adds items to the inventory",
      "linked_modules": ["InventoryModule"],
      "interaction_flow": "TriggerInteract() → HandleInteraction() → FInventoryModule::AddItem()",
      "requires_battle_system": false,
      "is_movable_ai_unit": false
    }
  ]
}

------------------------------------------------------------
CRITICAL MODULE API VALIDATION RULES (VERY IMPORTANT)
------------------------------------------------------------
You MUST strictly follow these rules when calling module functions:
1. You are ONLY allowed to call functions that ACTUALLY EXIST in the provided
   ModuleCodeGenerator output.
2. A function is considered "existing" ONLY IF:
   - It is declared in the module's header_code
   - OR it is implemented as a public method in the source_code
3. You MUST NOT:
   - Invent function names
   - Guess function signatures
   - Call functions mentioned in interaction_flow if they do NOT exist in module code

IMPORTANT RULES (MUST BE STRICTLY FOLLOWED):

1. You MUST generate one interactive object class for EVERY entry in
   InteractiveObjectAnalyzer.interactive_objects.
   Even if a model has no interaction flow (linked_modules is empty or
   interaction_flow is "no interaction"), you MUST still generate the class,
   and HandleInteraction must at least call:
   Super::HandleInteraction(InteractingCharacter);

2. **ALL generated interactive object classes MUST be exported from the LLMEditor module**.
   Therefore, EVERY generated class declaration MUST include the `LLMEDITOR_API` macro.

   Correct format (MUST use exactly this pattern):

UCLASS()
class LLMEDITOR_API AMyObjectInteractiveObject : public AInteractiveObjectBase
{
    GENERATED_BODY()
    ...
};

   FORBIDDEN:
   - Missing LLMEDITOR_API
   - Using any other *_API macro
   - Attaching the macro to the wrong identifier

3. **File naming and includes MUST strictly follow these rules**:
- Class name: `A<ModelName>InteractiveObject`
- Header file: `<ModelName>InteractiveObject.h`
  and MUST include:
  `#include "<ModelName>InteractiveObject.generated.h"`
- Source file: `<ModelName>InteractiveObject.cpp`
  and MUST include:
  `#include "<ModelName>InteractiveObject.h"`
  `#include "CustomModules.h"`

4. **Subclass responsibilities and restrictions**:
- Must inherit from `AInteractiveObjectBase`
- MUST NOT:
  - Inherit from `IBattleInterface` (handled by base class)
  - Create `BattleStats`
  - Bind death events (handled by base class)
- Subclass is ONLY responsible for:
  - Setting `bIsPartOfBattleSystem`
  - Overriding `HandleInteraction` and implementing interaction logic (calling modules)
  - Optionally overriding `BeginPlay` and calling `Super::BeginPlay()` when needed

5. **Manager access and module calls MUST use this exact pattern**:
 ```cpp
 auto& Manager = FMyModuleManager::Instance();
 auto* Inventory = Manager.GetModule<FInventoryModule>();
 if (Inventory)
 {
     Inventory->AddItem(GetName());
 }
 ```
IMPORTANT RULES:
The variable name Manager MUST be used.
Manager MUST be declared ONLY ONCE per function.
If Manager already exists in the current function, you MUST reuse it.
You MUST NOT redeclare or shadow Manager.
If multiple modules are used, they MUST all use the same Manager instance.
FORBIDDEN:
Declaring auto& Manager = ... more than once in a function
Using different manager variables or calling Instance() repeatedly

6. **Only modules provided by ModuleCodeGenerator may be used**:
- You will receive a `modules` array; ONLY these modules are allowed
- Module class names must be inferred as F<ClassName>
- All module calls MUST be null-checked

7. **Output format requirements**:
- Final output MUST be strict JSON, for example:
  ```
  {
    "objects": [
      {
        "class_name": "AChestInteractiveObject",
        "header_code": "Complete .h file code",
        "cpp_code": "Complete .cpp file code"
      }
    ]
  }
  ```
- JSON strings must contain ONLY code text
- NO natural language outside JSON
- C++ comments inside code are allowed

8. **Header/source style example (MUST FOLLOW)**:

The ONLY parts that may change in the header are:
`#include "CustomInteractiveObject.generated.h"`
and the class name itself.
All other includes MUST remain exactly the same.

CustomInteractiveObject.h
----------------------------------------
#pragma once
#include "InteractiveObjectBase.h"
#include "BattleInterface.h"
#include "BattleStatComponent.h"
#include "HealthBarUIComponent.h"
#include "CustomInteractiveObject.generated.h"

UCLASS()
class LLMEDITOR_API ACustomInteractiveObject : public AInteractiveObjectBase
{
 GENERATED_BODY()
public:
 virtual void HandleInteraction(AInteractiveCharacter* InteractingCharacter) override;
public:
 ACustomInteractiveObject();
protected:
 /** Bind death events */
 virtual void BeginPlay() override;
};

----------------------------------------

CustomInteractiveObject.cpp
----------------------------------------
#include "CustomInteractiveObject.h"
#include "CustomModules.h"
#include "MyModuleManager.h"
#include "EvaluationLogModule.h"

ACustomInteractiveObject::ACustomInteractiveObject()
{
}

void ACustomInteractiveObject::BeginPlay()
{
 Super::BeginPlay();
}

void ACustomInteractiveObject::HandleInteraction(AInteractiveCharacter* InteractingCharacter)
{
 // -----------------------------
 // 1. Execute base interaction logic (includes basic logging)
 // -----------------------------
 Super::HandleInteraction(InteractingCharacter);

 // -----------------------------
 // 2. Record subclass-specific interaction logic
 // -----------------------------
 auto& Manager = FMyModuleManager::Instance();
 auto* EvalLog = Manager.GetModule<FEvaluationLogModule>();

 const FString ObjName = GetName();
 const FString CharName = InteractingCharacter
     ? InteractingCharacter->GetName()
     : TEXT("Unknown");

 if (EvalLog)
 {
     EvalLog->WriteEvalLog(
         FString::Printf(
             TEXT("[CustomInteractiveObject] %s executed custom interaction logic with %s."),
             *ObjName,
             *CharName));
 }

 // -----------------------------
 // 3. Subclass custom logic (example)
 // -----------------------------
 // auto* Inventory = Manager.GetModule<FInventoryModule>();
 // if (Inventory)
 // {
 //     Inventory->AddItem(GetName());
 // }
}

Additional constraints:
- If linked_modules contains multiple modules, choose a reasonable call order
based on interaction_flow.
- At minimum, include a Manager/GetModule example for the FIRST linked module,
if it exists in ModuleCodeGenerator.modules.
- If interaction_flow specifies a concrete method
(e.g. FInventoryModule::AddItemFromContainer),
call that method (assume callable) and null-check the module first.

REMEMBER:
ALL generated interactive object classes MUST include LLMEDITOR_API.
Missing the export macro is considered a critical error.
"""

INTERACTIVE_OBJECTS_BASE_DIR = os.path.join(
    os.getenv("UE_Plugins"),
    "LLMEditor",
    "Source",
    "LLMEditor",
    "Private",
    "LLMGenerate",
    "InteractiveObjects"
)


def clear_directory(directory: str):
 if os.path.exists(directory):
     for filename in os.listdir(directory):
         file_path = os.path.join(directory, filename)
         try:
             if os.path.isdir(file_path):
                 shutil.rmtree(file_path)
             else:
                 os.remove(file_path)
         except Exception as e:
             print(f"[ERROR] Failed to delete file or directory {file_path}: {e}")
 else:
     print(f"[WARNING] Directory {directory} does not exist, skipping cleanup.")


def regenerate_custom_modules_header():
 base_module_dir = os.path.join(
    os.getenv("UE_Plugins"),
    "LLMEditor",
    "Source",
    "LLMEditor",
    "Private",
    "LLMGenerate"
)
 header_path = os.path.join(base_module_dir, "CustomModules.h")

 include_lines = []
 module_root = os.path.join(base_module_dir, "Module")

 print(f"[DEBUG] base_module_dir: {base_module_dir}")
 print(f"[DEBUG] module_root: {module_root}")

 if not os.path.exists(module_root):
     print(f"[WARNING] Module root directory does not exist: {module_root}")
     entries = []
 else:
     entries = os.listdir(module_root)

 for entry in entries:
     entry_path = os.path.join(module_root, entry)
     is_dir = os.path.isdir(entry_path)

     print(f"[DEBUG] Checking entry: {entry}, is_dir: {is_dir}")

     candidates = [
         os.path.join(module_root, entry, f"{entry}Module.h"),
         os.path.join(module_root, entry, f"{entry}.h"),
         os.path.join(module_root, f"{entry}Module", f"{entry}Module.h"),
         os.path.join(module_root, f"{entry}Module", f"{entry}.h"),
         os.path.join(module_root, entry, f"{entry}Module.generated.h"),
     ]

     for candidate in candidates:
         if os.path.exists(candidate):
             rel_folder = os.path.relpath(os.path.dirname(candidate), base_module_dir).replace("\\", "/")
             header_file_name = os.path.basename(candidate)
             include_lines.append(f'#include "LLMGenerate/{rel_folder}/{header_file_name}"')
             break

 content_lines = [
     "#pragma once",
     '#include "InteractiveCharacter.h"',
     '#include "MyModuleManager.h"',
 ]
 content_lines.extend(include_lines)

 final_text = "\n".join(content_lines) + "\n"

 os.makedirs(base_module_dir, exist_ok=True)
 with open(header_path, "w", encoding="utf-8") as f:
     f.write(final_text)

 print(f"[SUCCESS] CustomModules.h regenerated at: {header_path}")


def write_interactive_object_files(state, output):
 """
 output: JSON string generated by the LLM (must be strict JSON).
 This function writes generated objects to disk and regenerates CustomModules.h.
 """
 clear_directory(INTERACTIVE_OBJECTS_BASE_DIR)

 cleaned = re.sub(r"^```(?:json)?|```$", "", output.strip())

 try:
     data = json.loads(cleaned)
 except json.JSONDecodeError as e:
     print(f"[ERROR] Failed to parse JSON: {e}")
     print("Raw output:", output)
     return

 objects = data.get("objects", [])
 if not isinstance(objects, list):
     print("[ERROR] 'objects' field is not a list.")
     return

 os.makedirs(INTERACTIVE_OBJECTS_BASE_DIR, exist_ok=True)

 for obj in objects:
     class_name = obj.get("class_name")
     header_code = obj.get("header_code", "")
     cpp_code = obj.get("cpp_code", "")

     if not class_name or not header_code or not cpp_code:
         print(f"[WARNING] Skipping incomplete object entry: {obj}")
         continue

     fixed_name = class_name[1:] if class_name.startswith("A") else class_name
     folder_name = fixed_name.replace("InteractiveObject", "") or fixed_name

     obj_dir = os.path.join(INTERACTIVE_OBJECTS_BASE_DIR, folder_name)
     os.makedirs(obj_dir, exist_ok=True)

     with open(os.path.join(obj_dir, f"{folder_name}InteractiveObject.h"), "w", encoding="utf-8") as f:
         f.write(header_code.strip() + "\n")

     with open(os.path.join(obj_dir, f"{folder_name}InteractiveObject.cpp"), "w", encoding="utf-8") as f:
         f.write(cpp_code.strip() + "\n")

     print(f"[GENERATED] {class_name}")

 regenerate_custom_modules_header()


class InteractiveObjectCodeGeneratorNode(BaseLLMNode):

 def call_model(self, full_input: str, state: GraphState):
     """
     full_input is already assembled from:
     - ModuleCodeGenerator output
     - InteractiveObjectAnalyzer output
     """

     try:
        raw = state.llm_outputs.get("InteractiveObjectAnalyzer", "") if state else ""
        cleaned = re.sub(r"^```(?:json)?|```$", "", raw.strip())
        analyzer_json = json.loads(cleaned) if cleaned else {}

        interactive_objects = analyzer_json.get("interactive_objects", [])
        print(f"[DEBUG] {len(interactive_objects)} interactive objects found")

     except Exception as e:
         print(f"[ERROR] Failed to parse InteractiveObjectAnalyzer output: {e}")
         return "{}"

     final_objects: List[Dict[str, Any]] = []

     # ====================================================
     # Invoke LLM once per interactive object
     # ====================================================
     for idx, obj in enumerate(interactive_objects):
         print(f"[LLM] Generating interactive object {idx + 1}/{len(interactive_objects)}: {obj.get('model_name')}")

         single_input = (
             full_input.split("InteractiveObjectAnalyzer output：")[0]
             + "InteractiveObjectAnalyzer output：\n"
             + json.dumps({"interactive_objects": [obj]}, ensure_ascii=False, indent=2)
         )

         response = self.invoke_model_with_token_tracking([
             {"role": "system", "content": self.prompt},
             {"role": "user", "content": single_input}
         ],state)

         cleaned = re.sub(r"^```(?:json)?|```$", "", response.content.strip())

         try:
             data = json.loads(cleaned)
             final_objects.extend(data.get("objects", []))
         except Exception as e:
             print(f"[ERROR] Failed to parse LLM output: {e}")
             print(response.content)

     return json.dumps({"objects": final_objects}, indent=2, ensure_ascii=False)


def GetInput(node: BaseLLMNode, state: GraphState, full_input: str):
 """
 Concatenate ModuleCodeGenerator and InteractiveObjectAnalyzer outputs
 into a single prompt input for the LLM.
 """
 module_output = state.llm_outputs.get("ModuleCodeGenerator", "")
 analyzer_output = state.llm_outputs.get("InteractiveObjectAnalyzer", "")

 return (
     "\nModuleCodeGenerator output:\n"
     + module_output
     + "\n\nInteractiveObjectAnalyzer output:\n"
     + analyzer_output
 )


def create_interactive_object_code_generator() -> InteractiveObjectCodeGeneratorNode:
 return InteractiveObjectCodeGeneratorNode(
     name="InteractiveObjectCodeGenerator",
     prompt=INTERACTIVE_OBJECT_CODE_GENERATOR_PROMPT,
     enable_feedback=False,
     extra_prompt_action=GetInput,
     post_action=write_interactive_object_files,
 )


interactive_object_code_generator = InteractiveObjectCodeGeneratorNode(
 name="InteractiveObjectCodeGenerator",
 prompt=INTERACTIVE_OBJECT_CODE_GENERATOR_PROMPT,
 enable_feedback=False,
 extra_prompt_action=GetInput,
 post_action=write_interactive_object_files,
)
