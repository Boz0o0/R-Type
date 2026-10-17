#include <gtest/gtest.h>

#include <ampersand/audio/components/AudioSource.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/render/components/Animation.hpp>
#include <rtype/client/components/Explosion.hpp>
#include <rtype/client/systems/ExplosionSystem.hpp>

using ampersand::audio::AudioSource;
using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using ampersand::render::Animation;
using rtype::client::Explosion;
using rtype::client::ExplosionSystem;

// A 2-frame animation of 0.1 s per frame, played for elapsed seconds.
static Entity explosion(World& world, float elapsed, bool soundPlaying) {
  const Entity entity = world.create();
  world.add(entity, Explosion{});
  world.add(entity, Animation{
                        .frames = {{0, 0, 1, 1}, {1, 0, 1, 1}},
                        .frameDuration = 0.1F,
                        .loop = false,
                        .elapsed = elapsed,
                    });
  world.add(entity, AudioSource{.playing = soundPlaying});
  return entity;
}

TEST(ExplosionSystem, RemovesAnExplosionOnceItIsOver) {
  World world;
  const Entity over = explosion(world, 0.2F, false);

  ExplosionSystem{}.update(world);

  EXPECT_FALSE(world.alive(over));
}

TEST(ExplosionSystem, KeepsItWhileItAnimatesOrSounds) {
  World world;
  const Entity animating = explosion(world, 0.05F, false);
  const Entity sounding = explosion(world, 0.2F, true);

  ExplosionSystem{}.update(world);

  EXPECT_TRUE(world.alive(animating));
  EXPECT_TRUE(world.alive(sounding));
}

TEST(ExplosionSystem, LeavesOtherEntitiesAlone) {
  World world;
  const Entity entity = world.create();
  world.add(entity, Animation{
                        .frames = {{0, 0, 1, 1}},
                        .frameDuration = 0.1F,
                        .loop = false,
                        .elapsed = 1.0F,
                    });

  ExplosionSystem{}.update(world);

  EXPECT_TRUE(world.alive(entity));
}
