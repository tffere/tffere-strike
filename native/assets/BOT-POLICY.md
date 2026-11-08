# Strike bot policies — v1.6

STRIKE_Q_V2: 192 states × 9 high-level actions. Frozen inference only.

- `bot-policy-trained.txt`: default, final bot after two additional 100-round real-game dual-learner cycles with shared recoil and accuracy. Label AFTER 200 / RECOIL.
- `bot-policy.txt`: exact baseline before those 200 rounds under the same recoil rules. F5 switches and restarts the round.
- `bot-policy-after-100.txt`: historical v1.5 candidate, preserved but unused by v1.6.

Both current options scored 5 wins, 14 losses, 1 mutual elimination in the same 20 frozen tests (25%). The extra 200 rounds did not improve these results. Both bot and separate AI-player tables learned, but only the bot table is used in normal human play. See training/runs/2026-11-08-v1.6/REPORT.md and raw artifacts. Shared physical weapon/movement/recoil rules replace the historical weaker bot damage/cadence. Controllers and spawns still differ.

Sensory restrictions, collision, navigation and firing legality remain programmed. No online learning, team-training, Unreal or Blender rebuild is included.
