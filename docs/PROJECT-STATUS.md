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

## Security recheck follow-up

The user reported a virus warning and requested a recheck/repackage. Exact detection name is still unknown. See `docs/security/RECHECK.md` and included reports for results and limits. Original downloads had no ClamAV detections. The replacement is an MSVC Windows CI build from `4786a0c`, passed Windows gameplay tests and Defender, and was independently scanned with ClamAV. Use `downloads/TffereStrike-v1.2-Windows-x64-rebuilt.zip`; the default and v1.2 ZIP aliases were also replaced. Old cross-compile statements above describe the superseded package. Do not instruct the user to bypass antivirus. Graphical Windows playtesting remains outstanding.

## v1.3 — latest user requests and implementation

The user requested bots that explore and infer the player's location from sight, gunshots and footsteps, remembering/searching when contact is lost rather than using live hidden coordinates. They also requested persistent bullet marks on walls/buildings and a version increment for every shipped update going forward.

Implemented in `native/gameplay.hpp`: Patrol/Investigate/Chase/Search states, independent initial facing and reachable exploration goals, fixed remembered evidence, finite search, discrete gunshot/footstep/landing cues, wall attenuation, approximate hearing locations, and bounded surface-impact state. No live hidden-player location enters navigation. Rendering in `native/main.cpp` projects marks onto visible facade geometry; marks reset each round. All 31 native tests pass on Linux; Windows build/tests and Defender run in CI. Graphical Windows playtesting remains outstanding.

Release policy is now in `AGENTS.md`: increment every shipped version, including patch rebuilds; preserve existing tagged sources and versioned ZIPs; create a GitHub Release with notes and hashes. v1.2 was retrospectively tagged at b390847 with its historical rebuilt package. v1.0/v1.1 tags already exist. This feature release is v1.3; see its release and version-specific scan reports.

## v1.4 — crouch, bot collapse and sampled footsteps

User asked for roughly 15 minutes improving bot appearance/falls, realistic footsteps and player crouch. Implemented staged jointed death poses in `native/bot_pose.hpp` and `native/bot_renderer.hpp`, end-of-round animation completion, clearer knee proportions and movement weight shift. Ctrl crouch updates gameplay eye height, speed, grounded physics, hearing and audio events. Five Kenney CC0 concrete-step samples replace footstep synthesis; attribution/provenance/hashes are bundled. Nearby bot footsteps and landing sounds now follow events and volume attenuation. Camera bob follows actual grounded travel. All 36 native cases and eight prototype cases pass. Windows build/Defender evidence: run 42501343485, source 6022a35. Version-specific reports are under docs/security/v1.4/.

The user asked about Blender, Source and Unreal and explicitly said NOT to start an Unreal project or Blender rebuild yet. Neither was started. Current bots remain procedural C++/raylib meshes, not imported rigged characters or physical ragdolls. A graphical/listening Windows playtest is outstanding; do not claim that pose-math tests verify visual realism.
