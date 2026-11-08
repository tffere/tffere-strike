# Project handoff — v1.2 bot update

## User requests

- Make bots move more naturally, including running, jumping and crouching.
- Bots must shoot only when the player is in their vision and objects do not block them.
- Preserve progress in the repository so another chat can continue without the old conversation.

## Implementation

The current game is native C++17/raylib under `native/`. The JavaScript files are an earlier browser prototype.

Bot state and combat checks: `native/gameplay.hpp`. Articulated poses: `native/bot_renderer.hpp`. Rendering/tracers: `native/main.cpp`. Regression checks: `native/tests.cpp`.

v1.2 adds distance-dependent run/walk speeds, gradual turning, staggered jump/crouch actions, posture-aware hitboxes and eye/muzzle origins. Shot checks run after movement and require field of view, aim alignment and clear eye, barrel and firing paths. Bots still navigate to the player when hidden. Jumping does not vault cover.

## Validation and next step

25/25 native gameplay tests and 8/8 browser prototype tests pass. The Windows package is cross-compiled with Zig 0.13.0. Graphical v1.2 Windows playtesting remains to be done by the user; this session has no Wine or graphical test runner installed. Prior v1.1 screenshot/smoke results do not establish v1.2 animation quality.

Download `downloads/TffereStrike-v1.2-Windows-x64.zip`, extract it separately from v1.1 and run `TffereStrike/TffereStrike.exe`, keeping `assets` beside the executable. Earlier versioned ZIPs are preserved.

For future sessions, read this file and the Git log first. Record new user requests and unfinished work here before a long break or usage limit.
