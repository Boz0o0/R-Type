#pragma once

#include <ampersand/audio/SoundId.hpp>
#include <ampersand/render/components/Animation.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <array>
#include <rtype/common/protocol/Protocol.hpp>

namespace rtype::client {

struct Look {
  ampersand::render::Sprite sprite;
  ampersand::render::Animation animation;
  float scale{1.0F};
};

struct EntityLooks {
  std::array<Look, protocol::MAX_PLAYERS> ships;
  Look enemy;
  Look playerMissile;
  Look enemyMissile;
  Look powerUp;
  Look obstacle;
  Look explosion;
  ampersand::audio::SoundId shootSound{};
  ampersand::audio::SoundId explosionSound{};
};

}  // namespace rtype::client
