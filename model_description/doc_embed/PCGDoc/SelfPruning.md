# PCGSelfPruningSettings 
## Documentation
Official Description:
Removes intersections between points in the same point data, prioritizing data based on the settings (Large to Small, etc.). Points with a similar radius can be randomly selected using randomized pruning to prevent patterns from emerging.

Detailed Explanation:
The Self Pruning node in Unreal Engine’s Procedural Content Generation (PCG) framework automatically removes overlapping or redundant points within a single point dataset.
It is typically used after sampling or spawning stages to ensure spatial separation between generated points—such as foliage instances, debris, or object spawn positions—by pruning out intersecting points according to configurable rules.

Internally, the node compares points based on their extents (or other chosen numeric attributes) and removes points that fall within the same or intersecting bounds.
Depending on the selected PruningType, it determines which points take precedence—larger ones, smaller ones, or random ones.

This node supports both bounding-box–based pruning (default) and collision-based pruning using mesh collision data. When collision is enabled, each point’s associated geometry is tested for overlap to achieve more accurate spatial pruning, albeit with higher computational cost.

The Self Pruning node can also introduce randomized pruning, ensuring that results are not overly deterministic when points are similar in size or priority. This helps generate natural-looking, irregular distributions.

It is commonly used before Spawn Actor, Merge Points, or Difference nodes to clean up dense or overlapping regions in point datasets, improving both performance and visual fidelity.


## Parameter Space


## Connection Patterns

Input Pins:
In (Points) — The input point dataset that will be self-pruned to remove overlaps.
Typical connections:
→ Surface Sampler (Out) — To clean up densely sampled surface points.
→ Merge Points (Out) — To resolve overlaps after merging multiple point sets.
→ Transform Points (Out) — To eliminate intersections after spatial transformations.

Output Pins:
Out (Points) — The resulting point dataset with overlaps removed according to the pruning mode.
Typical connections:
→ Spawn Actor / Instance (In) — To spawn objects without spatial overlap.
→ Bounds Modifier (In) — To adjust bounds after pruning.
→ Difference / Intersect (In) — For cleaner spatial Boolean operations.
→ Collapse (In) — To merge refined point datasets for final output.