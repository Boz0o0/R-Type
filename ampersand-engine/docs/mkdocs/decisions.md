# Design decisions

This page explains the main choices we made for the engine, and why.
For how things work, see [Architecture](architecture/index.md).

## ECS

### Why an ECS
For the game engine architecture, we decided to use an ECS, which we think is a good fit and is a 'norm' in game development nowadays, mostly because it separates the data from the logic. We considered other alternatives, firstly the classic C++ OOP inheritance, but it was fastly dropped, because is too rigid and couples data and behavior, which does not fit an engine that has to be focused on decoupling.

### First choice: a bitset ECS
From all the variants of ECS, we first decided to go for a bitset-based ECS, using mostly the bases set by [Austin Morlan](https://austinmorlan.com/posts/entity_component_system/). We had to choose from the different types of ECS architectures (which of course we'd have to remix). We did not consider the archetype-based one (used by UE's Mass framework and Unity DOTS) because it is way more complex and 'pays off' only at thousands of entities, which we think will not be our case. This left us with 2 choices: the sparse set ECS and the bitset ECS. The sparse set ECS keeps each component type packed in its own array, so a system only goes through the entities that have the component, and adding, removing or finding a component is direct, the cost being one lookup per component type when a system needs several of them. The bitset ECS, on the other hand, checks all the components of an entity in a single bitwise operation, but its bitset only says whether an entity has a component, not where it is, so it needs other structures next to it that all have to be kept in sync.
Lastly, one team member (Enzo) had already worked with a bitset ECS in the past (Zappy), so he was more comfortable with it, and we also thought that the sparse set ECS would be more complex to implement.

### Why we changed
When we started implementing it, we realised that Morlan's ECS is in fact already a sparse set (using hash maps), with the bitsets and the lists of each system added on top of it, which all have to be kept in sync. So instead of patching it, we decided to build our own ECS, one that we fully understand, and that is easier to understand for the whole team.

### Final choice: a sparse set with one row per entity
We went for a sparse set ECS, where each component type is packed in its own pool, but instead of having one lookup array per pool (like [EnTT](https://github.com/skypjack/entt)), each entity has a single row that holds the position of each of its components. This row works as the 'signature' of the bitset ECS and also says where the components are, so checking or reading a component is direct and there is nothing to keep in sync. The cost is a limit of 64 component types and a bit of memory per entity, which is fine for a game like R-Type. How it works in detail: [ECS](architecture/ecs.md).

## Separation of modules
We decided to separate the engine into different modules, each with its own responsibilities. The objective was to have a clear separation of concerns, which would make the engine easier to maintain and extend, and the compilation more effcient, by compiling only the necessary modules.
