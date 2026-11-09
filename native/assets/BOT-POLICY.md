# Strike bot policies — v1.7

Both player and bots use AR-7 body25/head75, shared recoil and normal weapon cadence. Broad evidence-only search is enabled for both F5 bot options. Frozen runtime inference only.

Default bot-policy.txt is the preserved baseline after the first300 rounds under25HP. F5 selects bot-policy-trained.txt, the candidate after900 cumulative rounds under25HP (latest300), and restarts. Historical bot-policy-after-100.txt remains unused.

Latest survival-focused300 fixed progress tests:43→50 wins/100; damage73→65, mutual deaths28→1. Separate final-only100-case test:52→45 wins. Fewer mutual deaths but poorer separate-test winning; candidate stays optional. These are1v1 tests against AI-player Level0, not six-bot human play. Level0 is the unchanged original frozen before-player-policy.txt; it is not used to control the human player.

Reports/models/logs: training/runs/2026-11-08-frozen25-300/REPORT.md and training/runs/2026-11-08-frozen25-next300/REPORT.md and training/runs/2026-11-08-frozen25-survival300/REPORT.md. Before those,300 rounds under36body damage are preserved separately in2026-11-08-frozen300. Do not mix physical-rule/seed changes as learning gains. No Unreal/Blender rebuild or online runtime learning.
