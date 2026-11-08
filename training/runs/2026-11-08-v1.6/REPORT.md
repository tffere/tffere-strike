# v1.6 real-game learning and combat fixes

## Latest requested two cycles: 200 rounds with recoil

Exactly two consecutive 100-round training cycles ran through actual `strike::Game::step` on the desert map, at fixed 0.05-second simulation steps without rendering. Separate bot and AI-player Q tables BOTH learned, at the same learning rate. Each next cycle continued both saved tables. No simplified Duel was used. Training is faster than real time because graphics and frame pacing are absent.

After shared recoil/stance accuracy was added, a NEW baseline was measured. Before training and after EACH cycle, the bot faced the SAME saved, frozen original player policy for 20 seeded tests (1010000–1010019). These 60 evaluation rounds do not train either role. Evaluation asserts policy values remain unchanged. The player opponent is not the concurrently learning player from training. These are repeated progress cases, not a fresh blind holdout or a human-player benchmark.

| Recoil phase | Bot wins | Losses | Mutual eliminations | Timeouts | Win rate | Mean damage | Bot shots | Weapon switches |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Before these cycles | 5 | 14 | 1 | 0 | 25% | 48 | 45 | 1 |
| After 100 rounds | 5 | 14 | 1 | 0 | 25% | 48 | 45 | 1 |
| After 200 rounds | 5 | 14 | 1 | 0 | 25% | 48 | 45 | 1 |

**No measured improvement from these additional 200 rounds.** Five of each 20 tests had no bot shots; this remains a limitation, and does not by itself identify whether an encounter was visible/aligned. Both tables changed in each cycle (bot 33 then 48 entries; player 45 then 63), but the evaluated trajectories/outcomes stayed unchanged. Updating values is not proof of better decisions. Training outcomes were 34W/63L/3 draws then 36W/60L/4 draws, with 245 and 242 bot shots; training win rates against a changing opponent are not frozen-test improvement.

The small, coarse 192-state representation omits positioning geometry and readiness from the Q state (readiness is masked separately). Imported action rankings can survive small Q updates. A global unmasked-rank audit found only 3/192 states changed their top action after 100 rounds and 4/192 after 200; this does not identify visited evaluation states or legal masked choices. Future experiments should instrument state/action visitation and action-rank changes, then evaluate a richer state or residual initialization against a separate validation set. Do not simply claim more rounds improved tactics. No further continuation beyond round 200 was run. A verification replay restarted from the same initial tables after final parity fixes (human weapon draw delay and crouched movement speed). All training/evaluation CSVs and final model files were byte-identical; this replay is not another 200 rounds of accumulated learning.

## Changes applied equally to both roles

Shared weapon specs, 100 health, hitboxes, gravity/jump values, movement speed limits, reload/ammo/switch timing, physical recoil and spread. Rifle: 36 body/75 head damage, 0.115-second cooldown; sniper: 125/220, 1.35 seconds. The old slower bot cadence and damage scaling are removed. Same-step lethal exchanges can draw rather than always favoring the player update order. Role controllers and map starting positions still differ: equal physical settings do not imply identical controllers or 50% win rate.

Recoil moves the actual ray and human camera, recovers over time, and both AI controllers compensate through the same bounded turn rate. Spread multipliers: stationary 1, slow walk 0.75, ordinary walk 1.5, sprint 3; crouch multiplies by up to 0.5 and airborne by 4. Weapon base angular spreads: rifle 0.008 rad, sniper 0.0025 rad per axis. Ctrl crouches; Alt slow walks. Learned actions choose crouch at cover; there is no learned jump action in the nine-action table.

Weapon commitment/range hysteresis prevent the previous rifle/sniper switch loop. Legal action masks also constrain TD bootstrap targets. Bots can return fire while moving with sensory/aim/readiness gates. Blocked flank/hide routes now path to the selected tactical goal. Reward/horizon configuration and every before/after table are saved beside the CSVs. Neither game runtime nor frozen evaluation learns online.

## Earlier phases — do not combine comparisons

`historical-asymmetric/` preserves three earlier 100-round troubleshooting cycles under the old unequal weapons. They failed to win the 20 frozen tests. That larger campaign was paused when equal settings were requested.

`equal-before-recoil/` records the explicitly requested equal-weapon comparison before recoil: 9W/8L/3 mutual eliminations (45%, mean damage 60) before 100 rounds, then 11W/9L/0 draws (55%, mean damage 62.2). This is an observed +10 percentage-point change on 20 repeated cases, not conclusive evidence of general improvement. It must NOT be compared directly to the recoil-phase results: physical rules and evaluation seeds changed. The earlier unequal 0% versus equal 45% also cannot be credited to learning.

## Runtime assets and reproduction

v1.6 defaults to `bot-policy-trained.txt`, the exact final bot after the two latest cycles, clearly labelled AFTER 200 / RECOIL. F5 restarts with `bot-policy.txt`, the exact before-these-200 baseline. Both achieve the same measured result. The legacy v1.5 `bot-policy-after-100.txt` remains archived but is no longer selected by the game.

Build `native/training` and run `strike-campaign BOT PLAYER REFERENCE EMPTY_OUT CYCLE CONFIG [test-only]`. Reproduce baseline from recoil-before's before tables, cycle 0 and test-only; cycle 1 from those same before tables, then cycle 2 from cycle 1 after tables. Use each saved config.txt and reference-player.txt. Model values, seeds and CSVs are preserved; SHA256SUMS.txt covers these records. C++ standard random distribution details may vary between platforms; use the same compiler/runtime when requiring exact stochastic trajectories.
