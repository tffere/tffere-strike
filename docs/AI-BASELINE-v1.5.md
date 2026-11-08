# Bot tactical benchmark v1

Baseline: v1.5 gameplay; 50 Hz fixed step, deterministic seed 711.

| Dimension | Passed | Score / 100 |
|---|---:|---:|
| Coordination | 0/4 | 0.0 |
| Hearing | 4/4 | 100.0 |
| Navigation | 3/4 | 75.0 |
| Perception | 4/4 | 100.0 |
| Search | 3/4 | 75.0 |
| Tactics | 1/4 | 25.0 |

**Overall: 62.5 / 100** (equal dimension weights).

This is a project-specific scenario score, not human IQ, a difficulty rating, or visual-quality evaluation. Basic functionality and currently missing tactical capabilities are intentionally tested separately. Freeze these scenarios/thresholds when comparing versions.

- PASS — Perception: Acquire a visible player
- PASS — Perception: Reject a silent player behind the bot
- PASS — Perception: Do not acquire or shoot through a wall
- PASS — Perception: Hidden player position cannot change patrol
- PASS — Hearing: Investigate an audible shot behind the bot
- PASS — Hearing: Ignore footsteps beyond hearing range
- PASS — Hearing: Walls attenuate distant shots
- PASS — Hearing: Old sound does not track new hidden position
- PASS — Search: Remember last sight rather than hidden live position
- PASS — Search: Search when contact memory expires
- FAIL — Search: Move during a local search
- PASS — Search: Resume patrol after unsuccessful search
- PASS — Navigation: All depot spawns can reach an evidence location
- PASS — Navigation: All desert spawns can reach an evidence location
- FAIL — Navigation: Live AI routes around cover
- PASS — Navigation: Live route stays outside solid cover
- FAIL — Tactics: Disengage when critically wounded
- FAIL — Tactics: Strafe while exchanging fire
- FAIL — Tactics: Seek cover under threat
- PASS — Tactics: Enforce reaction time before first shot
- FAIL — Coordination: Separate overlapping teammates
- FAIL — Coordination: Choose distinct approaches to cover
- FAIL — Coordination: Hold fire when teammate blocks the firing lane
- FAIL — Coordination: Communicate a sighting to a teammate
