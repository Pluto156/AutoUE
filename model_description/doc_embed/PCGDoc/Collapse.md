# PCGCollapseSettings 
## Documentation
This node converts incoming Spatial or Attribute Set (Param) data into Point Data.
It ensures all input data can be represented as points for downstream PCG operations.

When the input is Spatial Data, it collapses it directly into base point data.

When the input is Attribute Set Data, it generates points based on metadata entries.

Optionally, empty attribute sets can be passed through unchanged (for backward compatibility).

This node is primarily used to unify data types in a PCG graph, simplifying further spatial or point-based processing.

## Parameter Space


## Connection Patterns
