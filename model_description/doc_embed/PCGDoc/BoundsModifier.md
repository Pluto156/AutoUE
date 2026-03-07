# PCGBoundsModifierSettings 
## Documentation
Official Description:
Modifies the bounds property on points in the provided point data.
This node adjusts or transforms the bounding box (BoundsMin / BoundsMax) associated with each point in the dataset.
It is particularly useful before applying nodes like Self Pruning, Intersection, or Difference, allowing users to refine or tweak the spatial extent of points to control overlap, intersection detection, or spatial filtering behavior.

Detailed Explanation:
The Bounds Modifier node is a point operation in Unreal Engine’s Procedural Content Generation (PCG) framework that provides a direct way to manipulate the per-point bounding volumes stored in the point data.

## Parameter Space
BoundsMin (FVector) — Defines the minimum bound or offset, depending on the mode. Typically represents the lower corner or translation/scale vector for bound modification.
BoundsMax (FVector) — Defines the maximum bound or offset, depending on the mode. Typically represents the upper corner or translation/scale vector for bound modification.
ss (float) — (Used only when bAffectSteepness = true) Controls how strongly steepness influences bounds modification (0.0 = no effect, 1.0 = full adjustment).

## Connection Patterns

Input Pins

In (Points) — Input point dataset whose per-point bounds will be modified.
Typical connections:
→ Surface Sampler (Out) — Modify sampled surface points’ bounds for slope-aware intersection.
→ Select Random Points (Out) — Adjust bounds after random selection for better spatial control.
→ Merge Points (Out) — Normalize or unify bounds after merging multiple point datasets.

Output Pins

Out (Points) — Output point dataset with updated bounds per point.
Typical connections:
→ Self Pruning (In) — Control pruning sensitivity by adjusting point bounds.
→ Intersection / Difference (In) — Fine-tune overlap behavior before boolean spatial operations.
→ Spawn Actor / Instance (In) — Use modified bounds to influence actor placement areas.
→ Attribute Transfer (In) — Use refined bounds for region-based data blending.
