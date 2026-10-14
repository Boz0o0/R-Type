#include <gtest/gtest.h>

#include <algorithm>
#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/physics/components/BoxCollider.hpp>
#include <ampersand/physics/resources/Collisions.hpp>
#include <ampersand/physics/systems/CollisionSystem.hpp>

namespace {

using ampersand::core::Transform;
using ampersand::core::ecs::Entity;
using ampersand::core::ecs::ResourceNotFound;
using ampersand::core::ecs::World;
using ampersand::physics::BoxCollider;
using ampersand::physics::Collisions;
using ampersand::physics::CollisionSystem;

Entity createBox(World& world, float left, float top, float size) {
    const Entity entity = world.create();
    Transform transform;
    transform.position = {left, top, 0.0F};
    world.add(entity, transform);
    world.add(entity, BoxCollider{{0.0F, 0.0F, size, size}});
    return entity;
}

bool hasPair(const Collisions& collisions, Entity first, Entity second) {
    return std::ranges::any_of(collisions.pairs, [&](const auto& pair) {
        return (pair.first == first && pair.second == second) ||
               (pair.first == second && pair.second == first);
    });
}

}  // namespace

TEST(CollisionSystem, FindsOverlappingPair) {
    World world;
    world.addResource<Collisions>();
    const Entity ship = createBox(world, 0.0F, 0.0F, 32.0F);
    const Entity enemy = createBox(world, 20.0F, 20.0F, 32.0F);

    CollisionSystem{}.update(world);

    const auto& collisions = world.getResource<Collisions>();
    ASSERT_EQ(collisions.pairs.size(), 1U);
    EXPECT_TRUE(hasPair(collisions, ship, enemy));
}

TEST(CollisionSystem, IgnoresSeparatedAndTouchingBoxes) {
    World world;
    world.addResource<Collisions>();
    createBox(world, 0.0F, 0.0F, 12.0F);
    createBox(world, 12.0F, 0.0F, 12.0F);
    createBox(world, 100.0F, 100.0F, 12.0F);

    CollisionSystem{}.update(world);

    EXPECT_TRUE(world.getResource<Collisions>().pairs.empty());
}

TEST(CollisionSystem, ReportsEveryPairOnce) {
    World world;
    world.addResource<Collisions>();
    const Entity left = createBox(world, 0.0F, 0.0F, 32.0F);
    const Entity middle = createBox(world, 10.0F, 0.0F, 32.0F);
    const Entity right = createBox(world, 20.0F, 0.0F, 32.0F);

    CollisionSystem{}.update(world);

    const auto& collisions = world.getResource<Collisions>();
    ASSERT_EQ(collisions.pairs.size(), 3U);
    EXPECT_TRUE(hasPair(collisions, left, middle));
    EXPECT_TRUE(hasPair(collisions, left, right));
    EXPECT_TRUE(hasPair(collisions, middle, right));
}

TEST(CollisionSystem, ClearsPairsOfThePreviousFrame) {
    World world;
    world.addResource<Collisions>();
    createBox(world, 0.0F, 0.0F, 32.0F);
    const Entity enemy = createBox(world, 20.0F, 0.0F, 32.0F);
    CollisionSystem collision;

    collision.update(world);
    ASSERT_EQ(world.getResource<Collisions>().pairs.size(), 1U);

    world.get<Transform>(enemy).position.x = 500.0F;
    collision.update(world);
    EXPECT_TRUE(world.getResource<Collisions>().pairs.empty());
}

TEST(CollisionSystem, ReportsTheSamePairEveryFrame) {
    World world;
    world.addResource<Collisions>();
    createBox(world, 0.0F, 0.0F, 32.0F);
    createBox(world, 20.0F, 0.0F, 32.0F);
    CollisionSystem collision;

    collision.update(world);
    collision.update(world);

    EXPECT_EQ(world.getResource<Collisions>().pairs.size(), 1U);
}

TEST(CollisionSystem, OnlyCollidesOnTheSameZ) {
    World world;
    world.addResource<Collisions>();
    const Entity ship = createBox(world, 0.0F, 0.0F, 32.0F);
    const Entity enemy = createBox(world, 10.0F, 0.0F, 32.0F);
    const Entity background = createBox(world, 0.0F, 0.0F, 100.0F);
    world.get<Transform>(background).position.z = -1.0F;

    CollisionSystem{}.update(world);

    const auto& collisions = world.getResource<Collisions>();
    ASSERT_EQ(collisions.pairs.size(), 1U);
    EXPECT_TRUE(hasPair(collisions, ship, enemy));
}

TEST(CollisionSystem, IgnoresEntityWithoutTransform) {
    World world;
    world.addResource<Collisions>();
    createBox(world, 0.0F, 0.0F, 32.0F);
    const Entity ghost = world.create();
    world.add(ghost, BoxCollider{{0.0F, 0.0F, 32.0F, 32.0F}});

    EXPECT_NO_THROW(CollisionSystem{}.update(world));
    EXPECT_TRUE(world.getResource<Collisions>().pairs.empty());
}

TEST(CollisionSystem, ThrowsWithoutCollisions) {
    World world;
    createBox(world, 0.0F, 0.0F, 32.0F);

    EXPECT_THROW(CollisionSystem{}.update(world), ResourceNotFound);
}
