# PCGGetLandscapeSettings
## Documentation
Official Description:
Specialization of the **Get Actor Data** node that returns appropriately typed and constructed Landscape data.

Detailed Explanation:  
The **Get Landscape Data** node is a procedural content generation (PCG) utility used to extract information from Unreal Engine Landscape actors.  
It specializes the more generic *Get Actor Data* node to handle Landscape-specific data structures, such as heightmaps, weightmaps, and landscape bounds.  
This node is typically used when you need to sample or analyze terrain information as part of a procedural generation graph.

When executed, it builds a collection of landscape data from all selected Landscape actors in the scene, returning it as PCG data for downstream nodes.  
It automatically filters the selected actors to only include those of type **Landscape**, ensuring that subsequent graph nodes receive structured terrain data.
## Parameter Space

## Connection Patterns
Input Pins: (None)
Output Pins:
Out (EPCGDataType::Landscape) — Provides sampled landscape data to downstream nodes.
Typical Connections:
→ Surface Sampler (Surface) — Uses the landscape surface to scatter points for foliage, props, or actor placement.
→ Filter by Landscape Attributes (In) — Selects terrain regions based on height, slope, or layer values.
→ Blend Terrain Height (In) — Combines multiple landscape sources to achieve layered terrain effects.
→ Attribute Sampler (In) — Reads terrain-based attributes for custom procedural logic.
Each connection allows downstream nodes to interpret or manipulate the landscape as a spatial data source for procedural generation and object placement.