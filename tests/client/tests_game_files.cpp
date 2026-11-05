#include <gtest/gtest.h>

#include <ampersand/audio/components/AudioSource.hpp>
#include <ampersand/audio/resources/AudioListener.hpp>
#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <ampersand/render/components/Animation.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <ampersand/render/resources/Camera.hpp>
#include <ampersand/render/resources/Renderer.hpp>
#include <ampersand/scene/resources/SceneManager.hpp>
#include <ampersand/scripting/components/Scripts.hpp>
#include <ampersand/scripting/systems/ScriptingSystem.hpp>
#include <cstdint>
#include <optional>
#include <rtype/client/Controls.hpp>
#include <rtype/client/PrefabFactory.hpp>
#include <rtype/common/components/Kind.hpp>
#include <rtype/common/protocol/Protocol.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "GameFiles.hpp"

using ampersand::audio::AudioListener;
using ampersand::audio::AudioSource;
using ampersand::core::Clock;
using ampersand::core::Transform;
using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using ampersand::input::InputManager;
using ampersand::render::Animation;
using ampersand::render::Camera;
using ampersand::render::Renderer;
using ampersand::render::Sprite;
using ampersand::scene::SceneManager;
using ampersand::scripting::ScriptingSystem;
using ampersand::scripting::Scripts;
using rtype::client::Controls;
using rtype::client::PrefabFactory;
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

  Entity created(EntityKind kind, std::uint8_t ownerId = 0) {
    return loaded(this->factory.create(kind, ownerId));
  }

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

TEST_F(GameFiles, EachPlayerHasAShipOfItsOwnColour) {
  for (std::uint8_t owner = 0; owner < rtype::protocol::MAX_PLAYERS; ++owner) {
    const Entity ship = this->created(EntityKind::Ship, owner);
    EXPECT_EQ(this->world.get<Sprite>(ship).source.y, 2 + (owner * 17));
  }
}

TEST_F(GameFiles, EnemiesAndMissilesAreAnimated) {
  for (const EntityKind kind : {EntityKind::Enemy, EntityKind::PlayerMissile,
                                EntityKind::EnemyMissile}) {
    const Entity entity = this->created(kind);
    ASSERT_TRUE(this->world.has<Animation>(entity));
    const auto& source = this->world.get<Sprite>(entity).source;
    const auto& first = this->world.get<Animation>(entity).frames.front();
    EXPECT_EQ(source.x, first.x);
    EXPECT_EQ(source.y, first.y);
    EXPECT_EQ(source.width, first.width);
  }
}

TEST_F(GameFiles, OnlyPlayerMissilesPlayTheShootSound) {
  const Entity shot = this->created(EntityKind::PlayerMissile);
  const Entity enemyShot = this->created(EntityKind::EnemyMissile);

  ASSERT_TRUE(this->world.has<AudioSource>(shot));
  EXPECT_TRUE(this->world.get<AudioSource>(shot).playing);
  EXPECT_FALSE(this->world.has<AudioSource>(enemyShot));
}

TEST_F(GameFiles, ExplosionPlaysOnceOverTheShipsWithItsSound) {
  const Entity explosion = loaded(this->factory.createExplosion());

  EXPECT_GT(this->world.get<Sprite>(explosion).layer, 0);
  EXPECT_FALSE(this->world.get<Animation>(explosion).loop);
  EXPECT_TRUE(this->world.get<AudioSource>(explosion).playing);
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
