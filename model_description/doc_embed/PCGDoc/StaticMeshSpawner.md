# PCGStaticMeshSpawnerSettings
## Documentation
**Official Description:**  
Spawn one static mesh per point in the provided point data.

**Detailed Explanation:**  
The **Static Mesh Spawner** node is a Procedural Content Generation (PCG) component responsible for instancing static meshes at every input point location.  
Each incoming point represents a spawn site, where a selected mesh will be instantiated according to the node’s **Mesh Selector**, **Instance Data Packer**, and **Property Override** configurations.

The node supports multiple mesh selection strategies through configurable *Mesh Selector Types*:
- **Weighted Selector** — chooses meshes based on user-defined weights.
- **By Attribute Selector** — selects meshes based on an attribute from the input points (e.g., “MeshType” or “Category”).
- **Weighted by Category Selector** — selects a category from an attribute, then chooses among meshes within that category using weights.

Each mesh entry contributes a *Weight* value; the final mesh choice is determined proportionally to the sum of all weights.  
The selected mesh is recorded into an attribute on the output point data, and its bounds are applied to the corresponding point’s `BoundsMin` / `BoundsMax` attributes if enabled.

The **Static Mesh Spawner** can also apply *per-instance property overrides* — mapping point attributes to mesh instance descriptor fields, enabling fine-grained control over materials, transforms, and other static mesh component parameters.  
It optionally supports synchronous loading for reliability in editor workflows, post-spawn callbacks on target actors, and debug validation to prevent redundant spawning.

This node is typically used for environment population, foliage or debris placement, procedural scene construction, and automated asset instancing workflows.  
When connected after a sampling node (e.g., *Surface Sampler* or *Scatter Points*), it forms the foundation of procedural world generation pipelines.


## Parameter Space
**StaticMeshPath** *(FString)* — Path to the static mesh asset to spawn (e.g., `"/Game/Meshes/Rock01.Rock01"`).  
**bApplyMeshBoundsToPoints** *(bool)* — When true, updates each point’s bounds based on the mesh’s geometry.  
**bSynchronousLoad** *(bool)* — Forces synchronous loading of the mesh before spawning (useful for editor workflows).  
**Seed** *(int32)* — Randomization seed for consistent results across runs (affects transform variation if present).

## Connection Patterns
### **Input Pins**
**Points** *(EPCGDataType::Point)* — Input point data defining spawn positions and attributes.

**Typical connections:**
→ *Surface Sampler (Out)* — Spawn meshes at evenly distributed surface sample points.  
→ *Transform Points (Out)* — Spawn meshes with transformed or randomized point data.  
→ *Merge Points (Out)* — Spawn meshes from combined point datasets.  
→ *Filter Points by Attribute (Out)* — Spawn meshes only on filtered point subsets.

---

### **Output Pins**
**Out** *(EPCGDataType::Point)* — Emits point data annotated with the selected mesh attribute and updated bounds information.

**Typical connections:**
→ *Set Attribute (Points)* — Add or modify custom attributes for spawned mesh data.  
→ *Debug Visualize Points (Points)* — Visualize spawn distribution before instantiation.  
→ *Merge Points (Points)* — Combine outputs from multiple spawners.  
→ *Export PCG Data (Points)* — Bake or serialize mesh spawn results for runtime use.