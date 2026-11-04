#include <ampersand/core/ecs/EventQueue.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/scripting/systems/ScriptingSystem.hpp>
#include <cstdio>
#include <rtype/client/systems/ScriptErrorSystem.hpp>

namespace rtype::client {

void ScriptErrorSystem::update(ampersand::core::ecs::World& world) {
  const auto& errors = world.getResource<
      ampersand::core::ecs::EventQueue<ampersand::scripting::ScriptError>>();
  for (const auto& error : errors.read()) {
    std::fprintf(stderr, "[client] script %s: %s\n", error.path.c_str(),
                 error.message.c_str());
  }
}

}  // namespace rtype::client
