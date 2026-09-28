# R-Type


# Stack

## Game Engine Architecture

### ECS
For the game engine architecture, we decided to use an ECS, which we think is a good fit and is a 'norm' in game development nowadays, mostly because it separates the data from the logic. From all the variants of ECS, we decided to go for a bitset-based ECS, using mostly the bases set by [Austin Morlan](https://austinmorlan.com/posts/entity_component_system/). We considered other alternatives, firstly the classic C++ OOP inheritance, but it is too rigid and couples data and behavior, which does not fit an engine that has to be focused on decoupling. Then we had to choose from the different types of ECS architectures (which of course we'd have to remix). We did not consider the archetype-based one (used by UE's Mass framework and Unity DOTS) because it is way more complex and 'pays off' only at thousands of entities, which we think will not be our case. This left us with 2 choices: the sparse set ECS and the bitset ECS. The sparse set ECS is more efficient in memory usage, but less so when it comes to manipulating multiple components, as it iterates multiple times, whereas the bitset ECS has to set a fixed-size bitset for each entity, but the component check is more efficient as it is a single bitwise operation.
Lastly, one team member (Enzo) had already worked with a bitset ECS in the past (Zappy), so he was more comfortable with it, and we also thought that the sparse set ECS would be more complex to implement. We also wanted to have a design that the whole team could understand easily, which is why we went for the bitset ECS.

### Separation of modules
We decided to separate the engine into different modules, each with its own responsibilities. The objective was to have a clear separation of concerns, which would make the engine easier to maintain and extend, and the compilation more effcient, by compiling only the necessary modules.