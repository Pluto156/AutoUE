# PCGSurfaceSamplerSettings
## Documentation

Official Description:
Samples points on a Surface data, distributing them in a regular grid pattern across the input surface geometry.

Detailed Explanation:
The Surface Sampler node is a Procedural Content Generation (PCG) utility designed to generate evenly distributed sample points across any surface-type PCG data.
It constructs a sampling grid that covers the surface and creates points according to configurable density, cell size, and looseness parameters.
Each generated point represents a localized area on the surface, which can then be used for scattering, placement, or density-based modifications in subsequent PCG operations.

Internally, the node computes an adaptive grid based on the Point Extents and Looseness parameters.
The grid defines a set of potential sampling cells, and a subset of these cells is converted into output points based on the Points Per Square Meter value.
This ensures consistent sampling density while providing flexibility to introduce variation and prevent overly regular patterns.

The Surface Sampler can also operate in an unbounded mode, allowing sampling across the entire surface domain even when no bounding shape is provided.
When combined with additional nodes, it serves as the foundation for procedural placement systems such as foliage scattering, debris generation, or terrain-based object placement.

## Parameter Space
PointsPerSquaredMeter (float) — Controls the density of generated points per square meter. Must be ≥ 0; smaller values generate fewer points, larger values generate more. Default: 0.1.
PointExtents (FVector) — Defines the half-size of each point’s bounds in world units. Determines the area each point occupies. Default: (50, 50, 50).
Looseness (float) — Controls the randomness of points placement within a grid cell. 0 places points at the cell center, 1 allows points anywhere inside the cell. Clamped ≥ 0. Default: 1.0.
PointSteepness (float) — Determines how sharply each point affects density. 0 = linear ramp, 1 = hard box influence. Clamped 0.0–1.0. Default: 0.5.
Seed (int32) — Controls the randomness of sample generation. Changing the seed produces a different distribution of points even with identical settings.

## Connection Patterns
Input Pins:
Surface (EPCGDataType::Surface) — The input surface geometry used for sampling.
Typical connections:
→ Get Landscape Data (Out) — Samples points directly from landscape surfaces.
→ Get Actor Data (Out) — Samples points on static mesh or spline actors’ surfaces.
→ Blend Landscape Data (Out) — Samples points from blended terrain datasets for complex surface generation.
Output Pins:
Out (EPCGDataType::Point) — Emits generated point data distributed across the surface.
Typical connections:
→ Transform Points (Points) — Applies random rotation, translation, or scaling to scattered points.
→ Filter Points by Attribute (Points) — Selectively filters sampled points based on custom attributes (e.g., slope, altitude).
→ Spawn Actors / Instances (Points) — Instantiates gameplay objects, meshes, or foliage at sampled point locations.
→ Merge Points (Points) — Combines outputs from multiple samplers into a unified point dataset.
Each connection allows the Surface Sampler to integrate seamlessly within the PCG data flow — converting surface information into spatially distributed point data for downstream procedural operations.