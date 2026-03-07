# PCGDifferenceSettings 
## Documentation
Official Description:
Outputs the result of subtracting one or more spatial datasets (differences) from a primary source dataset, producing only the regions or points unique to the source.

Detailed Explanation:
The Difference node performs a Boolean subtraction operation on spatial or point-based data in PCG (Procedural Content Generation).
It takes a Source input and one or more Difference inputs, and removes all regions or points that overlap with any of the differences.
This operation is analogous to performing a “source minus others” subtraction in geometry or density space.

The node is frequently used to carve out exclusion zones, remove overlapping areas, or isolate unique portions of procedural datasets.
For example, it can remove scatter points within designated forbidden regions or cut away overlapping meshes in environment generation workflows.

The behavior of the resulting dataset depends on two key configuration parameters:

Density Function — Determines how point densities are computed after subtraction. Options include:

Minimum: The final density equals the source density minus the maximum density among all difference inputs.

Clamped Subtraction: The final density equals the source density minus the sum of all difference densities, clamped between 0 and 1.

Binary: The final density is 0 if any difference has a non-zero density at that location, otherwise it remains equal to the source density.

Mode — Defines how the difference operation behaves with respect to discrete (point) versus continuous (field) data.

Inferred: Automatically selects Discrete if any of the inputs are point data; otherwise defaults to Continuous.

Continuous: Performs the subtraction in density space without collapsing the data into points.

Discrete: Collapses the result into point data, effectively producing a concrete, sampled representation.

This distinction allows the Difference node to function seamlessly in both analytical (density field) and concrete (point data) contexts.
When working with meshes or landscapes, Continuous mode can yield smooth transitions, while Discrete mode is ideal for point-based scatter operations.

Example:
Using a Difference node with a building footprint as the source and road splines as the difference removes all points that fall on road areas, ensuring procedurally placed objects do not overlap with roads or paths.

This node is a fundamental boolean operation tool in PCG graphs, enabling precise spatial exclusion and dataset refinement across terrain, foliage, and structure generation pipelines.


## Parameter Space
DensityFunction (EPCGDifferenceDensityFunction) — Determines how the density of each point or voxel is recalculated after the difference operation.
Available options:
Minimum — The output density equals the source density minus the maximum density of all difference inputs.
Clamped Subtraction — The output density equals the source density minus the sum of all difference densities, clamped between 0 and 1.
Binary — The output density is 0 if any difference has non-zero density at that location; otherwise it retains the source density.
## Connection Patterns

Input Pins:

Source (EPCGDataType::Spatial) — The primary dataset from which other datasets will be subtracted.

Differences (EPCGDataType::Spatial) — One or more datasets whose spatial regions or densities will be subtracted from the source.

Typical input connections:
→ Surface Sampler (Out) — Removes regions or points within another sampled area (e.g., terrain exclusion zones).
→ Merge (Out) — Uses a merged set of regions or masks as the subtraction input.
→ Volume / Attribute Mask (Out) — Carves out areas defined by density fields or attribute-driven masks.

Output Pins:

Difference (EPCGDataType::Spatial) — The resulting dataset containing only regions or points remaining after subtraction.

Typical output connections:
→ Scatter Points (Points) — Spawns instances or meshes only in the remaining (non-excluded) areas.
→ Filter Points by Attribute (Points) — Further filters or classifies the resulting dataset.
→ Union / Intersection (Spatial) — Combines or intersects with other boolean operations for layered procedural effects.
→ Attribute Visualizer (Points) — Visualizes resulting density or mask differences for debugging.
