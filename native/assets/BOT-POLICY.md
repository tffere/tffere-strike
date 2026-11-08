# Strike bot policy — v1.5

Format: STRIKE_Q_V2; 192 states × 9 actions.

Selected checkpoint: actual-game training round 10, after a complete 100-round run. Selection used separate validation seeds; it did not select on final evaluation results. This starts from simplified-run checkpoint 30 and was further trained through the real Game::step implementation on the desert map.

Frozen gameplay inference only; the distributed game does not train during play.

Bot rifle damage stays 9. New bot sniper damage is 31 with its own ammunition, reload state and cooldown; bot shooting retains its deliberately slower single-player cadence. Those role differences were present in both initial and selected policies in the actual-game evaluation. This is not a claim of symmetric human-versus-bot combat.

The policy is a high-level decision table. Visibility, memory, collision, cover candidates and pathfinding remain programmed. Unknown targets keep the existing sensory patrol/search system. No hidden live player coordinates are passed to the policy. No team-training or Unreal project is included.
