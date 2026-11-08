> Historical simplified-simulator results. The latest requested real-game experiment is in [the dual-learner report](../2026-11-08-real-dual/REPORT.md).

# Self-play training report — November 8, 2026

## What ran

A headless 1v1 Q-learning experiment. Both training fighters share one learned policy; both receive learning updates. The policy sees only its own status and sensory evidence, not hidden enemy coordinates/HP/weapon. Health, damage, weapon cooldowns and movement rules are equal on both sides.

Two distinct experiments were performed: a rejected 100-round pilot and a corrected 100-round run. The corrected run was reproduced from scratch with identical seeds; its raw evaluation/training CSVs and policy values match exactly. This is not one 200-round policy. Each run starts from its own initial policy.

## Corrected experiment

- Exactly 100 training rounds, seeds 17000–17099, exploration seed 99117.
- Decisions every 0.25 seconds; simulation ticks every 0.05 seconds; each duel capped at 45 simulated seconds.
- One shared Q-table: 192 coarse tactical states × 9 high-level actions. Checkpoints saved every 10 rounds.
- Separate validation: 20 seeds, alternating starting sides, against two fixed opponents (40 duels per checkpoint). No learning during validation.
- Checkpoint **30** scored best in validation (29/40 match points versus initial 19.5/40), so it is saved as `trained-policy.txt`. All 100 training rounds still completed. `final-policy.txt` is round 100; later learning was not consistently better.
- Final evaluation: 100 previously unused seeds per matchup, alternating learner side. Initial and selected policies face identical seed/seat pairs against each fixed opponent. Four matchups = 400 evaluation duels; no policy updates or selection based on this final test set.

## Held-out results

| Fixed opponent | Policy | Wins | Draws | Losses | Match score* |
|---|---|---:|---:|---:|---:|
| rifle chaser | initial | 42% | 18% | 40% | 51.0% |
| rifle chaser | trained | 68% | 12% | 20% | 74.0% |
| weapon aware defender | initial | 28% | 12% | 60% | 34.0% |
| weapon aware defender | trained | 22% | 64% | 14% | 54.0% |

*Match score = wins + half of draws, divided by 100 duels. It is not win rate. Raw rows, damage/shooting/switching/cover counts and paired-seed bootstrap uncertainty are saved beside this report.

Against the rifle chaser, win rate increased by 26 percentage points (42% → 68%). Against the weapon-aware defender, wins decreased (28% → 22%), losses decreased sharply (60% → 14%), and draws increased (12% → 64%). Of the 64 trained-policy draws, 52 ended in simultaneous elimination and 12 at the time limit; the starting policy had 12 time-limit draws. The timeout count did not increase. Earlier descriptions of improved survival or more stalling were incorrect: this is primarily more mutual kills, not stronger winning play. Do not hide the lower win rate by reporting only match score or accuracy.

Only 176 of 1,728 state/action pairs were visited over the 100-round run. These small, controlled results do not establish broad intelligence or successful transfer to the real game.

## Rejected pilot

The first experiment used weapon-switch-only actions that left the bot idle, encouraging ineffective action loops. Its learned policy regressed: rifle-opponent wins 43% → 22%, defender wins 31% → 17%. That run, its final policy and its CSVs are retained under `pilot-rejected/`. Its `STRIKE_Q_V1` action semantics are incompatible with the corrected `STRIKE_Q_V2` policy format and cannot be loaded by the current trainer.

The corrected actions mean engage-with-rifle/sniper: switch if necessary, respect the switch delay, then aim and fire. This is a simulator/action-definition correction, not an aim/damage/health bonus. The starting policy and fixed opponents were reevaluated under the corrected rules. Final evaluation uses different seeds from the pilot.

## What this does and does not change

This is an offline training lab, not an Unreal project and not a new Windows game release. The live v1.4 bot code and its prior benchmark remain unchanged: overall 62.5/100, tactics 0/100. No new IQ score was calculated.

The lab reuses the game's collision, visibility, navigation and weapon-state code in four compact cover-layout templates with seed-based offsets/spawns. It is not the full rendered game. Live v1.4 bots use different combat integration (including flat bot damage), so transfer is unproven. No team coordination, articulated animation, crouch/jump tactics or visual/audio quality is learned here.

Learning chooses high-level actions (shoot current gun, engage rifle/sniper, hide, chase, flank left/right, reload, search). Cover-position generation, pathfinding, perception and aiming remain programmed mechanics, not learned geometry or neural animation. Evaluation opponents are fixed rule-based strategies, not separate trained policies and not a measurement of v1.4's 62.5 benchmark.

Historical follow-up: subsequent actual-game experiments are documented separately. See ../2026-11-08-real-dual/REPORT.md for the latest 100+20 experiment. Only actual-game code should be used for future learning, per the user. No inference from this simplified score alone establishes live-game improvement.
