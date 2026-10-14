# Ampersand Engine

![Ampersand Engine logo](./assets/ampersand-logo-dark.svg#only-light){ width="200" }
![Ampersand Engine logo](./assets/ampersand-logo-light.svg#only-dark){ width="200" }

Ampersand Engine is a modular C++20 game engine built around a sparse set ECS.
It is split into independent modules (core, audio, input, network, physics, render), so you only compile what you need.

## Where to go next

- **[Getting started](getting-started.md)**: requirements, building the engine, choosing modules and running the tests.
- **[Architecture](architecture/index.md)**: how the engine is organised and how its parts work together.
- **[Design decisions](decisions.md)**: the choices we made, the alternatives we considered, and why.

## Modules

- **[Render](architecture/rendering.md)**: sprites, text, animation and camera, drawn by a `RenderSystem`.

## Used by

- **[R-Type](https://boz0o0.github.io/R-Type/)** ([source code](https://github.com/Boz0o0/R-Type)): a R-Type game built on top of Ampersand Engine.
