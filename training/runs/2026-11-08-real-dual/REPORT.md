# Authoritative real-game experiment: 100 training rounds + 20 tests

This is the latest experiment requested by the user. Earlier simplified-simulator and one-learner experiments are separate historical results; their percentages must not be mixed with this run.

## Exact procedure

1. Save the starting policy separately as `before-bot-policy.txt` and `before-player-policy.txt`. Both begin with the same previously validated Q values (the earlier simplified-run checkpoint 30), but are separate role policies.
2. Run exactly **100 training rounds** through the real `strike::Game::step`, on the desert map. One AI controls the real player character through normal `Input`; another controls one real game bot. The other five bots are disabled and kill-count accounting is adjusted for one opponent. Both active role policies learn independently.
3. Save checkpoints every 10 rounds and both final policies after round 100. There is no validation checkpoint selection in this experiment: the candidate is the final round-100 bot policy.
4. Disable learning and run exactly **20 tests**: final `after-bot-policy.txt` controls the game bot; saved original `before-player-policy.txt` controls the AI player. The newly learned player policy does NOT act as the opponent in these tests.
5. Verify that neither policy changes during frozen testing and that both role policies actually changed during training.

## Result

| Final trained game bot | Count | Rate |
|---|---:|---:|
| Wins | 0 | 0% |
| Losses | 16 | 80% |
| Timeouts | 4 | 20% |

Win rate is `wins / 20`; timeouts are not wins. A timeout means the game's normal round expired with both roles alive. The real game's update order does not generate the simplified simulator's simultaneous-elimination ties. No before-versus-before control batch was run for this latest 20-test request, so it does not provide an independent before/after improvement percentage. With only 20 tests, 0 observed wins is not proof that the true win probability is exactly zero.

**This run does not demonstrate an improvement.** Both policies learned in the sense that their Q values changed, but that alone does not mean they became better. Training against an evolving opponent and then testing against a frozen original policy are different conditions.

## Reproducibility and settings

- Implementation: `native/training/train_real_dual.cpp`, `actual_game.hpp`, and the actual `gameplay.hpp`.
- Training seeds: 920000–920099. Test seeds: 930000–930019. Test cases were not used to select or tune the final model.
- Two independent policy tables, each 192 states × 9 actions; separate exploration RNG seeds 910031 and 910037.
- Training exploration decreases from 0.35 to 0.10. Testing is greedy, with learning disabled.
- Q-learning: alpha 0.12, gamma 0.96 per decision; decisions every 0.25 seconds and simulation ticks every 0.05 seconds.
- Reward per role: +0.05 per actual damage dealt, −0.06 per damage taken, −0.003 per tick; terminal +20 win, −20 loss, −5 timeout. These rewards do not modify health, weapon damage or accuracy.
- Real map, spawn locations, collision, perception, hearing, ammunition, reloads, switching, jumping/crouching physics, player weapons and bot attack rules are used. Rendering/audio playback is omitted; this is headless execution of gameplay code, not 100 manual Windows playtests.
- The actual roles remain asymmetric: the player's rifle has 36 body damage and faster fire; bots retain 9 rifle damage and slower single-player cadence, plus 31-damage sniper support. Real player-first update ordering is preserved. Do not interpret the role-specific win rate as a balanced model-versus-model contest.
- Policy inputs are own state and sensory evidence. No hidden live opponent coordinates, opponent health or future actions enter the policy observation. Cover candidates, navigation and aiming remain programmed mechanics.

`training-100.csv` and `test-20.csv` contain every result; winner 0 means game bot, winner 1 means player, −1 means timeout. The original/final/checkpoint policies and their hashes are saved beside this report. Both final role policies are retained; only the bot candidate is an optional game asset.

## v1.5 deployment

The game starts with the prior validated bot policy. Press **F5** to try the clearly labelled after-100-round experimental candidate; switching restarts the round. F5 switches back. Both policy assets and earlier game releases are preserved. The candidate is not silently promoted as an improvement. The separately trained player policy is retained for research and does not control a human player in normal gameplay.

The frozen tactical benchmark is a separate measurement. With the default prior trained policy, v1.5 remains 62.5/100 overall: tactics 25/100, navigation 75/100, search 75/100, coordination 0/100, hearing/perception 100/100 on their four basic probes each. This is not the 20-test win rate or human IQ; navigation lost one scenario versus the v1.4 rule-based behavior. No broad tactical superiority is claimed.

No Unreal project was started. Future learning work should remain in the real game-code setting, per the user's instruction.
