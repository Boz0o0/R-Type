# Ampersand Engine

![Ampersand Engine logo](./assets/ampersand-logo-dark.svg#only-light){ width="200" }
![Ampersand Engine logo](./assets/ampersand-logo-light.svg#only-dark){ width="200" }

Ampersand Engine is a modular C++20 game engine built around a sparse set ECS.
It is split into independent modules, so a game only compiles what it uses.

## Where to go next

- **[Getting started](getting-started.md)**: requirements, building the engine, choosing modules and using it in a game.
- **[Architecture](architecture/index.md)**: how the engine is organised and how each module works.
- **[Design decisions](decisions.md)**: the choices we made, the alternatives we considered, and why.

## Modules

- **[ECS](architecture/ecs.md)** (`core`): entities, components, systems and resources.
- **[Physics](architecture/physics.md)**: movement, gravity and collisions.
- **[Render](architecture/render.md)**: sprites, text, animation and camera, drawn by a `RenderSystem`.
- **[Input](architecture/input.md)**: keyboard and mouse of the frame, in one resource.
- **[Audio](architecture/audio.md)**: sound effects and music, played by an `AudioSystem`.
- **[Net](architecture/net.md)**: a non-blocking UDP socket.

## Used by

- **[R-Type](https://boz0o0.github.io/R-Type/)** ([source code](https://github.com/Boz0o0/R-Type)): a R-Type game built on top of Ampersand Engine.
