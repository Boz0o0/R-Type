# Audio

The audio module plays the sound effects and the music. The game only adds
components: the `AudioSystem` plays them through the `IAudioEngine` stored in
the `AudioListener` resource (`SfmlAudioEngine` here), so the game never calls
SFML itself.

## Concepts

| Name            | Kind      | What it does                                                                    |
| --------------- | --------- | ------------------------------------------------------------------------------- |
| `AudioSource`   | component | a sound or a music the entity plays: `source`, `volume`, `loop`, `playing`      |
| `AudioListener` | resource  | the `IAudioEngine` (`engine`) and the global `volume`                           |
| `SceneMusic`    | resource  | music of the whole scene, belonging to no entity, optional                      |
| `AudioSystem`   | system    | starts and stops every `AudioSource` and the `SceneMusic`, follows their changes |
| `IAudioEngine`  | interface | loads and plays the sounds and music (`SfmlAudioEngine`)                        |

The `AudioSystem` throws `ResourceNotFound` without an `AudioListener`. Every
volume goes from 0 (mute) to 100 (full).

## Example

```cpp
namespace audio = ampersand::audio;

audio::SfmlAudioEngine sfml;  // the only line that names the backend
auto& listener = world.addResource<audio::AudioListener>(sfml);
const auto laser = listener.engine.loadSound("assets/laser.wav");
const auto theme = listener.engine.openMusic("assets/theme.ogg");

world.addResource<audio::SceneMusic>() = {.music = theme, .playing = true};

audio::AudioSystem audioSystem;  // finds the engine in the AudioListener

while (running) {
    if (fired) {
        // each shot is an entity: its sound starts with it, stops with it
        const ecs::Entity shot = world.create();
        world.add(shot, audio::AudioSource{.source = audio::SoundRef{laser},
                                           .playing = true});
    }
    audioSystem.update(world);
}
```

The backend (`sfml` in the example) is not copied: it must stay alive as long
as the World, so declare it before the World.

## Sounds and music

A **sound** (`loadSound`, a `SoundId`) is loaded in memory once, for short
effects. A **music** (`openMusic`, a `MusicId`) is read from the disk while it
plays, for long tracks. A failed load returns `0`.

`source` holds a `SoundRef` or a `MusicRef`. Both ids are numbers, so these
wrappers tell them apart:

```cpp
world.add(boss, audio::AudioSource{.source = audio::SoundRef{roar}});
world.add(radio, audio::AudioSource{.source = audio::MusicRef{song}, .loop = true});
```

## Playing

`playing` is what the game wants: `true` to start, `false` to stop.

- The `AudioSystem` sets `playing` back to `false` once a non-looping sound or
  music reaches its end.
- Changing `volume` while it plays changes it at once, and changing `source`
  switches to the new sound.
- An unknown id plays nothing, and `playing` goes back to `false`.
- Destroying the entity, or removing its `AudioSource`, stops its sound: a
  sound never outlives its entity.
- An entity plays **one sound at a time**: setting `playing` to `true` while
  its sound still plays does not start a second one. To hear the same sound
  several times at once (rapid fire), give each one its own entity, as in the
  example.

`SceneMusic` follows the same rules, for a music that belongs to no entity (the
music of a level). Changing its `music` while it plays switches to the new
one, and removing the resource stops it.

!!! warning "One source per music"
    A `MusicId` is a single stream. If two sources (two `AudioSource`s, or one
    and the `SceneMusic`) play the same `MusicId`, stopping one stops the other.
    Open the file twice with `openMusic` to play it from two sources.

## Global volume

`AudioListener::volume` is applied to every sound and music, on top of their
own volume. The game changes it at any time, from an options menu for example:

```cpp
world.getResource<audio::AudioListener>().volume = 50.0F;
```

## Playing by hand

`IAudioEngine` (`listener.engine`) can also play without the ECS. `play` starts
a **voice**, one playing instance of a sound, and returns its `VoiceId`: the
same sound can have several voices at once.

```cpp
const auto voice = listener.engine.play(laser, /*volume*/ 80.0F,
                                        /*pitch*/ 1.0F, /*loop*/ false);
listener.engine.setVolume(voice, 50.0F);

listener.engine.playMusic(theme, /*volume*/ 100.0F, /*loop*/ true);
listener.engine.setMusicVolume(theme, 60.0F);
```

A finished voice is dropped the next time `play` is called, so one-shot sounds
do not pile up. `setPosition` and `setListenerPosition` place the sounds in
space, but the `AudioSystem` does not use them.

Do not drive by hand a sound that an `AudioSource` plays: the `AudioSystem`
would undo it at its next update.

## Limits

| Rule     | Value                                                                                   |
| -------- | --------------------------------------------------------------------------------------- |
| Entities | one sound or music at a time per `AudioSource`: one entity per shot for rapid fire      |
| Music    | a `MusicId` is one stream: two sources playing it stop each other                       |
| Space    | no positional sound through the ECS (`setPosition` by hand only)                        |
| Pitch    | always 1 through the ECS (`setPitch` by hand only)                                      |
