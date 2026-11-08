# Accessibility

What the game does for players with a disability, and what it does not do yet.

## Physical and motor

| Feature | How |
| --- | --- |
| Controller | the left stick moves, A shoots, Start quits |
| Any key | every action is a named button in `game/world/config.json`: changing a key, or giving an action a second key, only means editing this file |
| No mouse needed | the menus are played with the arrows, Tab and Enter |
| No timing in the menus | nothing in the login and lobby screens has to be done in time |

## Visual

| Feature | How |
| --- | --- |
| Colorblind players | the planes are red, blue, yellow and black and white, so they differ by brightness too, not only by color; the HUD shows our number ("P1" to "P4") in our color; the enemy bullets are pink on a dark background; the two enemies also differ by movement |
| Readable text | large texts (36 for the HUD, 80 for the titles and the banner) |
| Photosensitive epilepsy | no flash of the whole screen and nothing that blinks fast: the explosions and sparks are small and short |

## Audio

| Feature | How |
| --- | --- |
| Nothing only heard | every sound has a visual: a shot shows a flash, a hit a spark, a death an explosion or a ghost, the start and the end of a game a banner |
| Volume | the global volume is the `volume` of `AudioListener` in `game/world/config.json` |

## Mental and cognitive

| Feature | How |
| --- | --- |
| Simple controls | moving and one button to shoot |
| Help on screen | each menu shows its keys at the bottom |
| Clear messages | the banner says what happened ("Server unreachable", "Room full", "VICTORY") |

## Not done yet

- No settings screen: the keys and the volume are changed in `config.json`.
- No choice of difficulty or game speed.
- No autofire yet
