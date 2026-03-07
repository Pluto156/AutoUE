# PCGSpawnActorSettings 
## Documentation
Official Description:
Spawns either the contents of an actor or an actor per point in the provided input data. The actor is driven by the template actor class or the instanced templated actor or by attribute depending on the settings.
It contains the following options:
Template Actor Class: List of available Actors in your project.
Option:
Collapse Actors: Gathers some of the actor components (Static Mesh Components and PCG Components) and acts collapsed inside of the target actor.
Merge PCG only: Spawns one actor per point If the spawned actor has a PCG component, its inputs are bundled into a single graph execution.
No Merging: Spawns one actor per point.
In the No Merging case, it is possible to set properties to the actors from attributes on the points through the ‘Spawned Actor Property Override Descriptions’.
Attach mode:
Not attached: No engine-aware relationship will exist between the original actor (owner of the PCG component) and the created actor.
Attached: The created target actor will be attached as a child to the actor owning the PCG component. Note that this has an impact on streaming of said created actor in World Partition enabled maps, e.g. this actor will be streamed in with the parent.
In Folder: No engine-aware relationship will exist but the actor(s) will be placed in a folder named according to the actor owning the PCG component for easier visualization in the Scene Outliner. Will not impact streaming.
This node can be used to create partition-like actors and gather artifacts (visual and otherwise) on actors that can be properly streamed in on play.

## Parameter Space
TemplateActor (FString) — Path to the template actor asset to use for spawning (e.g., `"/Game/Blueprints/BP_Chest.BP_Chest"`). When editing is allowed and the spawn option is not CollapseActors, the spawned actors will inherit properties and components from the referenced template actor.


## Connection Patterns
