#include <gtest/gtest.h>

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <ampersand/render/TextureId.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <rtype/client/Starfield.hpp>
#include <rtype/client/components/Scrolling.hpp>
#include <vector>

using ampersand::core::Transform;
using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using ampersand::physics::Velocity;
using ampersand::render::Sprite;
using ampersand::render::TextureId;
using rtype::client::Scrolling;
using rtype::client::Starfield;

static constexpr TextureId BACK = 1;
static constexpr TextureId FRONT = 2;

static std::vector<Entity> copiesOf(World& world, TextureId texture) {
  std::vector<Entity> copies;
  for (const Entity entity : world.owners<Scrolling>()) {
    if (world.get<Sprite>(entity).texture == texture) {
      copies.push_back(entity);
    }
  }
  return copies;
}

TEST(Starfield, PutsTwoCopiesOfEachLayerSideBySide) {
  World world;
  Starfield::spawn(world, {BACK, FRONT});

  for (const TextureId texture : {BACK, FRONT}) {
    const std::vector<Entity> copies = copiesOf(world, texture);
    ASSERT_EQ(copies.size(), 2U);
    const float first = world.get<Transform>(copies[0]).position.x;
    const float second = world.get<Transform>(copies[1]).position.x;
    EXPECT_FLOAT_EQ(first, 0.0F);
    EXPECT_FLOAT_EQ(second, world.get<Scrolling>(copies[0]).width);
  }
}

TEST(Starfield, DrawsBehindTheGame) {
  World world;
  Starfield::spawn(world, {BACK, FRONT});

  for (const Entity entity : world.owners<Scrolling>()) {
    EXPECT_LT(world.get<Sprite>(entity).layer, 0);
  }
  EXPECT_LT(world.get<Sprite>(copiesOf(world, BACK)[0]).layer,
            world.get<Sprite>(copiesOf(world, FRONT)[0]).layer);
}

TEST(Starfield, ScrollsLeftWithTheFrontLayerFaster) {
  World world;
  Starfield::spawn(world, {BACK, FRONT});

  const float back = world.get<Velocity>(copiesOf(world, BACK)[0]).value.x;
  const float front = world.get<Velocity>(copiesOf(world, FRONT)[0]).value.x;
  EXPECT_LT(back, 0.0F);
  EXPECT_LT(front, back);
}
