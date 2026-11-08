# Strike bot policy — v1.5

Format: STRIKE_Q_V2; 192 states × 9 actions.

Selected checkpoint: actual-game round 0 (the imported simplified-run checkpoint 30). A complete 100-round real Game::step experiment was run on the desert map, but no later checkpoint beat the starting policy in validation. The best existing trained policy is retained. Final tests use the same autonomous inference path as the game; the new 100 rounds showed no additional win-rate gain. See training/runs/2026-11-08-actual-game/REPORT.md for corrections to earlier preliminary comparisons.

Frozen gameplay inference only; the distributed game does not train during play.

Bot rifle damage stays 9. New bot sniper damage is 31 with its own ammunition, reload state and cooldown; bot shooting retains its deliberately slower single-player cadence. Those role differences were present in both initial and selected policies in the actual-game evaluation. This is not a claim of symmetric human-versus-bot combat.

The policy is a high-level decision table. Visibility, memory, collision, cover candidates and pathfinding remain programmed. Unknown targets keep the existing sensory patrol/search system. No hidden live player coordinates are passed to the policy. No team-training or Unreal project is included.

## Dual-learner real-game experiment

`bot-policy-after-100.txt` is the game-bot policy after the newly requested 100 real-game rounds with two independent learners: one controls the player and the other controls the game bot. Both roles learned. Its 20 frozen tests against the saved before-training player policy produced 0 wins, 16 losses, and 4 timeouts. It is a candidate, not an established improvement.

v1.5 starts with the prior validated bot policy. Press F5 to switch to the clearly labelled after-100-round experimental policy; switching restarts the round. Press F5 again to return. Both policies are preserved. The separately trained player policy and full records are saved in training/runs/2026-11-08-real-dual/; it is not used to control a human player in normal gameplay.
