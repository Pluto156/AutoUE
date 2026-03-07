# PCGSelectRandomPointsSettings 
## Documentation
Official Description:
Randomly selects a subset of points from an input point dataset, either by fixed count or by ratio, introducing controlled randomness into point-based procedural operations.

Detailed Explanation:
The Select Random Points node is a utility within the Unreal Engine 5 Procedural Content Generation (PCG) framework that filters point data by randomly selecting a subset of the input points.
It is commonly used to introduce controlled variation or to reduce the density of a point dataset before applying transformations, spawning, or filtering operations.

This node supports two selection modes:

By Count — Selects an exact number of points (NumPoints) from the total input points.

By Ratio — Selects a proportional subset of points based on a given ratio (Ratio), e.g., 0.25 to select 25% of all points.

Internally, the node:

Reads the input spatial data and converts it into base point data.

Generates a randomized sequence of indices using a deterministic FRandomStream seeded by the graph’s execution seed.

Selects and copies the corresponding points to an output dataset while preserving all per-point attributes (position, rotation, scale, metadata, etc.).

This ensures stable randomization: given the same input and seed, the same subset of points will always be selected — an essential property for reproducible procedural generation.

It is typically used between sampling and spawning stages in a PCG graph to create natural distribution variation or selective downsampling.

## Parameter Space

SelectMode (EPCGSelectRandomMode) — Determines how points are randomly selected. Options include ByCount (fixed number) or ByRatio (percentage-based).
NumPoints (int32) — Number of points to select when using ByCount mode. Clamped between 1 and the total number of available points.
Ratio (float) — Fraction of points to keep when using ByRatio mode. Valid range is 0.0–1.0; for example, 0.1 keeps 10% of all input points.
Seed (int32) — Controls the deterministic random sequence used for selection. Using the same seed produces the same random subset each time.
## Connection Patterns

Input Pins:

In (Points) — The input point dataset from which a random subset will be selected.
Typical connections:
→ Surface Sampler (Out) — To randomly thin sampled surface points.
→ Transform Points (Out) — To apply random selection after spatial transformations.
→ Merge Points (Out) — To randomly sample merged point datasets for variety.

Output Pins:

Out (Points) — Outputs the selected random subset of input points.
Typical connections:
→ Bounds Modifier (In) — To further refine spatial distribution.
→ Self Pruning (In) — To eliminate overlapping points.
→ Spawn Actor / Instance (In) — To spawn a random subset of objects or actors.
→ Difference / Collapse (In) — For spatial operations or merging after filtering.
