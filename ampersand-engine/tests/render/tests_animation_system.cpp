#include <gtest/gtest.h>

#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/render/components/Animation.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <ampersand/render/systems/AnimationSystem.hpp>
#include <stdexcept>
#include <utility>

namespace {

using ampersand::core::Clock;
using ampersand::core::ecs::Entity;
using ampersand::core::ecs::ResourceNotFound;
using ampersand::core::ecs::World;
using ampersand::render::Animation;
using ampersand::render::AnimationSystem;
using ampersand::render::isFinished;
using ampersand::render::Sprite;

/** @brief Three 16x16 frames side by side, half a second each. */
Animation threeFrames() {
    return {
        .frames = {{0, 0, 16, 16}, {16, 0, 16, 16}, {32, 0, 16, 16}},
        .frameDuration = 0.5F,
    };
}

Entity createAnimated(World& world, Animation animation) {
    const Entity entity = world.create();
    world.add(entity, Sprite{.texture = 1});
    world.add(entity, std::move(animation));
    return entity;
}

/** @brief Runs the AnimationSystem once with the given dt. */
void step(World& world, float deltaTime) {
    world.getResource<Clock>().dt = deltaTime;
    AnimationSystem{}.update(world);
}

int frameX(World& world, Entity entity) {
    return world.get<Sprite>(entity).source.x;
}

}  // namespace

TEST(AnimationSystem, ShowsTheFirstFrameRightAway) {
    World world;
    world.addResource<Clock>();
    const Entity ship = createAnimated(world, threeFrames());

    step(world, 0.0F);

    EXPECT_EQ(frameX(world, ship), 0);
    EXPECT_EQ(world.get<Sprite>(ship).source.width, 16);
}

TEST(AnimationSystem, ChangesFrameEveryFrameDuration) {
    World world;
    world.addResource<Clock>();
    const Entity ship = createAnimated(world, threeFrames());

    step(world, 0.25F);
    EXPECT_EQ(frameX(world, ship), 0);
    step(world, 0.25F);
    EXPECT_EQ(frameX(world, ship), 16);
    step(world, 0.5F);
    EXPECT_EQ(frameX(world, ship), 32);
}

TEST(AnimationSystem, LoopsBackToTheFirstFrame) {
    World world;
    world.addResource<Clock>();
    const Entity ship = createAnimated(world, threeFrames());

    step(world, 1.5F);
    EXPECT_EQ(frameX(world, ship), 0);
    step(world, 0.5F);
    EXPECT_EQ(frameX(world, ship), 16);
}

TEST(AnimationSystem, StaysOnTheLastFrameWithoutLoop) {
    World world;
    world.addResource<Clock>();
    Animation explosion = threeFrames();
    explosion.loop = false;
    const Entity boom = createAnimated(world, explosion);

    step(world, 10.0F);
    EXPECT_EQ(frameX(world, boom), 32);
    step(world, 10.0F);
    EXPECT_EQ(frameX(world, boom), 32);
}

TEST(AnimationSystem, ResettingElapsedRestarts) {
    World world;
    world.addResource<Clock>();
    const Entity ship = createAnimated(world, threeFrames());
    step(world, 1.0F);
    ASSERT_EQ(frameX(world, ship), 32);

    world.get<Animation>(ship).elapsed = 0.0F;
    step(world, 0.0F);

    EXPECT_EQ(frameX(world, ship), 0);
}

TEST(AnimationSystem, SkipsEntityWithoutSprite) {
    World world;
    world.addResource<Clock>();
    const Entity ghost = world.create();
    world.add(ghost, threeFrames());

    step(world, 0.5F);

    EXPECT_FALSE(world.has<Sprite>(ghost));
    EXPECT_FLOAT_EQ(world.get<Animation>(ghost).elapsed, 0.0F);
}

TEST(AnimationSystem, SkipsAnimationWithoutFrames) {
    World world;
    world.addResource<Clock>();
    const Entity ship = world.create();
    world.add(ship, Sprite{.texture = 1, .source = {5, 5, 10, 10}});
    world.add(ship, Animation{});

    step(world, 0.5F);

    EXPECT_EQ(frameX(world, ship), 5);
}

TEST(AnimationSystem, ThrowsOnInvalidFrameDuration) {
    World world;
    world.addResource<Clock>();
    Animation broken = threeFrames();
    broken.frameDuration = 0.0F;
    createAnimated(world, broken);

    EXPECT_THROW(step(world, 0.1F), std::invalid_argument);
}

TEST(AnimationSystem, ThrowsWithoutClock) {
    World world;

    EXPECT_THROW(AnimationSystem{}.update(world), ResourceNotFound);
}

TEST(AnimationSystem, NonLoopingAnimationFinishesAfterItsLastFrame) {
    World world;
    world.addResource<Clock>();
    Animation explosion = threeFrames();
    explosion.loop = false;
    const Entity boom = createAnimated(world, explosion);

    step(world, 1.0F);
    EXPECT_FALSE(isFinished(world.get<Animation>(boom)));
    step(world, 0.5F);
    EXPECT_TRUE(isFinished(world.get<Animation>(boom)));
}

TEST(AnimationSystem, LoopingAnimationNeverFinishes) {
    World world;
    world.addResource<Clock>();
    const Entity ship = createAnimated(world, threeFrames());

    step(world, 100.0F);

    EXPECT_FALSE(isFinished(world.get<Animation>(ship)));
}
