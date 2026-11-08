#include <gtest/gtest.h>

#include <ampersand/audio/components/AudioSource.hpp>
#include <ampersand/audio/resources/AudioListener.hpp>
#include <ampersand/audio/systems/AudioSystem.hpp>
#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/Scheduler.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <ampersand/physics/systems/MovementSystem.hpp>
#include <ampersand/render/components/Animation.hpp>
#include <ampersand/render/components/AnimationStateMachine.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <ampersand/render/resources/Camera.hpp>
#include <ampersand/render/resources/Renderer.hpp>
#include <ampersand/render/systems/AnimationSystem.hpp>
#include <ampersand/scene/resources/SceneManager.hpp>
#include <ampersand/scripting/components/Scripts.hpp>
#include <ampersand/scripting/systems/ScriptingSystem.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <rtype/client/Controls.hpp>
#include <rtype/client/PrefabFactory.hpp>
#include <rtype/client/WorldMirror.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/protocol/Packets.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "GameFiles.hpp"

using ampersand::audio::AudioListener;
using ampersand::audio::AudioSource;
using ampersand::audio::AudioSystem;
using ampersand::core::Clock;
using ampersand::core::Transform;
using ampersand::core::ecs::Entity;
using ampersand::core::ecs::Scheduler;
using ampersand::core::ecs::World;
using ampersand::input::InputManager;
using ampersand::physics::MovementSystem;
using ampersand::physics::Velocity;
using ampersand::render::Animation;
using ampersand::render::AnimationStateMachine;
using ampersand::render::AnimationSystem;
using ampersand::render::Camera;
using ampersand::render::Renderer;
using ampersand::render::Sprite;
using ampersand::scene::SceneManager;
using ampersand::scripting::ScriptingSystem;
using ampersand::scripting::Scripts;
using rtype::client::Controls;
using rtype::client::PrefabFactory;
using rtype::client::WorldMirror;
using rtype::client::test::FakeBackends;
using rtype::client::test::loadGame;
using rtype::common::EntityKind;

// A broken file throws in loadGame, or makes the factory return nothing.
class GameFiles : public testing::Test {
 protected:
  void SetUp() override { loadGame(this->scenes, this->world, this->backends); }

  // The star-field copies: the scripted entities with a sprite.
  std::vector<Entity> stars() {
    std::vector<Entity> found;
    for (const Entity entity : this->world.owners<Scripts>()) {
      if (this->world.has<Sprite>(entity)) {
        found.push_back(entity);
      }
    }
    return found;
  }

  static Entity loaded(std::optional<Entity> entity) {
    if (!entity) {
      throw std::runtime_error("the prefab did not load");
    }
    return *entity;
  }

  Entity created(EntityKind kind, std::uint8_t ownerId = 0,
                 std::uint8_t variant = 0) {
    return loaded(this->factory.create(kind, variant, ownerId));
  }

  Entity death(EntityKind kind, std::uint8_t ownerId = 0,
               std::uint8_t variant = 0) {
    return loaded(this->factory.createDeath(kind, variant, ownerId));
  }

  // Runs the scripts, the moves and the animations, like the client.
  void play(Scheduler& scheduler, float seconds) {
    for (float elapsed = 0.0F; elapsed < seconds; elapsed += TICK) {
      scheduler.update(TICK);
    }
  }

  static constexpr float TICK = 1.0F / 60.0F;

  FakeBackends backends;
  SceneManager scenes;
  World world;
  PrefabFactory factory{this->world, this->scenes};
};

TEST_F(GameFiles, ConfigOpensTheWindowAndDeclaresTheActions) {
  EXPECT_TRUE(this->world.hasResource<Renderer>());
  EXPECT_TRUE(this->world.hasResource<AudioListener>());
  EXPECT_TRUE(this->world.hasResource<Camera>());
  EXPECT_EQ(this->scenes.getCurrentScene(), "game");
  const auto& input = this->world.getResource<InputManager>();
  EXPECT_NO_THROW((void)input.getAxisRaw(std::string(Controls::HORIZONTAL)));
  EXPECT_NO_THROW((void)input.getAxisRaw(std::string(Controls::VERTICAL)));
  EXPECT_NO_THROW((void)input.getButton(std::string(Controls::FIRE)));
  EXPECT_NO_THROW((void)input.getButton(std::string(Controls::QUIT)));
}

TEST_F(GameFiles, SceneHasTwoCopiesOfEachStarLayerSideBySide) {
  const auto stars = this->stars();

  ASSERT_EQ(stars.size(), 4U);
  float widthCovered = 0.0F;
  for (const Entity star : stars) {
    widthCovered += this->world.get<Transform>(star).position.x;
    EXPECT_LT(this->world.get<Sprite>(star).layer, 0);
  }
  EXPECT_FLOAT_EQ(widthCovered, 2.0F * 1920.0F);
}

TEST_F(GameFiles, EachPlayerHasItsOwnPlaneAndBullets) {
  std::set<std::uint32_t> planes;
  std::set<std::uint32_t> bullets;
  for (std::uint8_t owner = 0; owner < rtype::protocol::MAX_PLAYERS; ++owner) {
    planes.insert(
        this->world.get<Sprite>(this->created(EntityKind::Ship, owner))
            .texture);
    bullets.insert(
        this->world.get<Sprite>(this->created(EntityKind::PlayerMissile, owner))
            .texture);
  }
  EXPECT_EQ(planes.size(), rtype::protocol::MAX_PLAYERS);
  EXPECT_EQ(bullets.size(), rtype::protocol::MAX_PLAYERS);
}

TEST_F(GameFiles, MissilesAreAnimated) {
  for (const EntityKind kind :
       {EntityKind::PlayerMissile, EntityKind::EnemyMissile}) {
    const Entity entity = this->created(kind);
    ASSERT_TRUE(this->world.has<Animation>(entity));
    const auto& source = this->world.get<Sprite>(entity).source;
    const auto& first = this->world.get<Animation>(entity).frames.front();
    EXPECT_EQ(source.x, first.x);
    EXPECT_EQ(source.y, first.y);
    EXPECT_EQ(source.width, first.width);
  }
}

TEST_F(GameFiles, SineEnemyHasItsOwnLook) {
  const Entity enemy = this->created(EntityKind::Enemy);
  const Entity sine = this->created(EntityKind::Enemy, 0, 1);
  const Entity unknown = this->created(EntityKind::Enemy, 0, 200);

  const auto texture = this->world.get<Sprite>(enemy).texture;
  EXPECT_NE(this->world.get<Sprite>(sine).texture, texture);
  EXPECT_EQ(this->world.get<Sprite>(unknown).texture, texture);
}

TEST_F(GameFiles, MirrorGivesThePacketVelocityOverThePrefabOne) {
  WorldMirror mirror(this->world, this->factory);
  mirror.setLocalPlayer(0);

  mirror.spawn(rtype::protocol::EntitySpawn{
      .entityId = 1, .entityType = EntityKind::Enemy, .velocityX = -123.0F});
  mirror.spawn(rtype::protocol::EntitySpawn{
      .entityId = 2, .entityType = EntityKind::Ship, .ownerId = 0});

  const auto enemy = mirror.find(1);
  const auto ship = mirror.find(2);
  ASSERT_TRUE(enemy && ship);
  EXPECT_FLOAT_EQ(this->world.get<Velocity>(*enemy).value.x, -123.0F);
  EXPECT_FALSE(this->world.has<Velocity>(*ship));
}

TEST_F(GameFiles, EveryKillHasItsEffectWithItsSound) {
  for (std::uint8_t owner = 0; owner < rtype::protocol::MAX_PLAYERS; ++owner) {
    EXPECT_TRUE(
        this->world.has<AudioSource>(this->death(EntityKind::Ship, owner)));
  }
  for (std::uint8_t variant = 0; variant < 2; ++variant) {
    EXPECT_TRUE(this->world.has<AudioSource>(
        this->death(EntityKind::Enemy, 0, variant)));
  }
  EXPECT_TRUE(
      this->world.has<AudioSource>(this->death(EntityKind::PlayerMissile)));
  EXPECT_TRUE(
      this->world.has<AudioSource>(this->death(EntityKind::EnemyMissile)));
  EXPECT_FALSE(this->factory.createDeath(EntityKind::PowerUp, 0, 0));
}

TEST_F(GameFiles, EffectsAreGoneOnceOver) {
  Scheduler scheduler(this->world);
  scheduler.add<ScriptingSystem>();
  scheduler.add<MovementSystem>();
  scheduler.add<AnimationSystem>();
  scheduler.add<AudioSystem>();
  this->play(scheduler, TICK);
  const std::size_t before = this->world.entityCount();
  (void)this->death(EntityKind::Ship);
  (void)this->death(EntityKind::Enemy, 0, 1);
  (void)this->death(EntityKind::PlayerMissile);
  (void)this->death(EntityKind::EnemyMissile);
  ASSERT_GT(this->world.entityCount(), before);

  this->play(scheduler, 5.0F);

  EXPECT_EQ(this->world.entityCount(), before);
}

TEST_F(GameFiles, PlaneTiltsWhileItClimbsOrDives) {
  Scheduler scheduler(this->world);
  scheduler.add<ScriptingSystem>();
  scheduler.add<AnimationSystem>();
  const Entity plane = this->created(EntityKind::Ship);
  const auto& machine = this->world.get<AnimationStateMachine>(plane);

  for (int frame = 0; frame < 20; ++frame) {
    this->world.get<Transform>(plane).position.y -= 6.0F;
    scheduler.update(TICK);
  }
  EXPECT_EQ(machine.current(), "up");
  for (int frame = 0; frame < 20; ++frame) {
    this->world.get<Transform>(plane).position.y += 6.0F;
    scheduler.update(TICK);
  }
  EXPECT_EQ(machine.current(), "down");
  this->play(scheduler, 1.0F);
  EXPECT_EQ(machine.current(), "straight");
}

TEST_F(GameFiles, BlimpPlaysItsAttackWhenItFires) {
  Scheduler scheduler(this->world);
  scheduler.add<ScriptingSystem>();
  scheduler.add<AnimationSystem>();
  const Entity blimp = this->created(EntityKind::Enemy);
  const auto& machine = this->world.get<AnimationStateMachine>(blimp);

  this->play(scheduler, 1.0F);
  EXPECT_FALSE(machine.isPlayingAction());
  this->play(scheduler, 0.4F);
  EXPECT_TRUE(machine.isPlayingAction());
}

TEST_F(GameFiles, KindsWithoutALookStillGetAnEntity) {
  const Entity powerUp = this->created(EntityKind::PowerUp);

  EXPECT_TRUE(this->world.has<Transform>(powerUp));
  EXPECT_FALSE(this->world.has<Sprite>(powerUp));
}

TEST_F(GameFiles, StarScriptPutsACopyThatLeftTheScreenBehindTheOther) {
  this->world.addResource<Clock>();
  const auto stars = this->stars();
  const Entity gone = stars.front();
  const Entity visible = stars.back();
  this->world.get<Transform>(gone).position.x = -1925.0F;
  this->world.get<Transform>(visible).position.x = -100.0F;

  ScriptingSystem{}.update(this->world);

  EXPECT_FLOAT_EQ(this->world.get<Transform>(gone).position.x, 1915.0F);
  EXPECT_FLOAT_EQ(this->world.get<Transform>(visible).position.x, -100.0F);
}
