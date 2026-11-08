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

## Tactical intelligence measurement

User requested a measurable bot IQ score and asked for the current score. Added `native/ai_benchmark.cpp`, a deterministic 24-probe, six-dimension tactical benchmark, plus methodology and a frozen v1.4 baseline under docs/. This is a scenario score, not human IQ or visual quality. It intentionally measures missing tactics/teamwork instead of masking them with basic correctness passes. No game behavior/binary was changed; game remains v1.4. Read `docs/AI-BENCHMARK.md` before comparing future versions. Keep benchmark thresholds and weights fixed, and do not claim four basic sensor passes mean universally perfect perception.

## Offline self-play training lab 0.1

User authorized training bots against each other for 100 rounds to improve combat action choices, with progress at least every 10 minutes. Implemented native/training/ (standalone C++17/CMake, no graphics dependency) and executed a 100-round pilot, then a corrected independent 100-round experiment after the pilot regressed. Reproduced the corrected run from scratch with matching CSV/policy values. Two fighters share one learning Q-table (192 states, 9 actions); evaluation freezes it against two fixed opponents using separate seeds and alternating sides. Validation selects checkpoint 30, although all 100 rounds completed. Final policy/10 checkpoints/raw logs/report are under training/runs/2026-11-08/.

Corrected held-out wins vs rifle chaser: 42% starting → 68% selected. Vs weapon-aware defender: 28% → 22% wins, losses 60% → 14%, draws 12% → 64%. Correction: 52 trained-policy draws were mutual eliminations and 12 were timeouts; the timeout count did not increase. The earlier survival/stalling interpretation was incorrect. Report retains rejected pilot (wins 43→22 and 31→17), policies, schema difference and limitations. Final tests use seeds 200000..200099; do not tune against them. Full gameplay regression tests still pass.

This is a simplified 1v1 simulator, not a new Windows game release. Live game remains v1.4 and tactical benchmark remains 62.5 overall, 0 tactics. No learned policy is installed in the live game. Future transfer needs a new version and aligned runtime weapons/observations/actions; the current game does not load STRIKE_Q_V2 policy files. User explicitly said not to start Unreal; no Unreal project was started.

## v1.5 and latest authoritative 100+20 experiment

User requested a versioned game update using trained policies, then explicitly required learning only through real game code: one AI controls the player and one controls a game bot, BOTH learn for 100 rounds, then test the final bot against the saved original player policy for 20 rounds. Implemented native/training/train_real_dual.cpp using actual Game::step and separate role Q tables. Exactly 100 training and 20 frozen tests completed. Result: 0 bot wins, 16 losses, 4 timeouts. Both role tables changed; evaluation checks no learning occurred. All originals, finals, 10 checkpoints per role, CSVs, hashes and the authoritative report are under training/runs/2026-11-08-real-dual/.

Earlier simplified/one-learner results must not be mixed with this test. The earlier claimed actual-game 2%→16% gain was invalid due to an inference/control mismatch and was withdrawn. Superseded actual-game exploration logs are preserved separately. In the corrected one-learner comparison, the starting policy already achieved 16/100 wins; the extra training showed no further gain. Do not use that preliminary claim.

v1.5 loads the prior validated policy by default. F5 switches to after-100-round experimental policy and restarts the round; F5 returns. Assets: bot-policy.txt (prior) and bot-policy-after-100.txt (final requested bot candidate). The trained player policy is saved for research only. New policy integration includes weapon selection/ammo/reload/switch delay, cover/chase/flank intent and sensory gates. Rifle damage stays 9; new bot sniper is 31, preserving deliberate player-vs-bot role asymmetry. No game-time online learning, Unreal project or Blender rebuild was started.

42 native tests pass on Linux/Windows; training mechanics pass on both. Windows build/Defender source 5b6d756, run 42518359634. Both model assets match training snapshots. Scans and policy hashes live in docs/security/v1.5. Graphical Windows playtest is outstanding. Default-policy tactical benchmark remains 62.5 overall, tactics 25 and navigation 75; this is separate from the 0/20 win result and is not broad improvement.
