# Bot tactical benchmark v1

This evaluates observable behavior in 24 deterministic capability probes. It is not human IQ, a general intelligence measure, difficulty, kill rate or visual quality. Damage, health and speed bonuses do not add points. Higher scores mean passing more of this particular suite, not universally better AI.

## Reproduce

From the repository root, compile with a C++17 compiler:

```sh
g++ -std=c++17 -O2 -Wall -Wextra native/ai_benchmark.cpp -o ai-benchmark
./ai-benchmark v1.4
```

The optional label only names the report. The compiled gameplay header determines the AI being measured. To compare versions, compile the identical benchmark against each version's gameplay source. Keep the scenarios, thresholds, 50 Hz timestep, seed 711 and dimension weights unchanged. A changed benchmark needs a new benchmark version and cannot be compared directly with old scores.

## Score

Six equally weighted dimensions each contain four binary probes. Each dimension scores `100 * passed / 4`; the overall score is the mean of the six dimension scores. Both passes and failures are printed. Capability failures return process exit zero because they are measurement results, not a broken benchmark tool.

- Perception: acquire visible targets, reject rear/occluded targets and avoid hidden-position leakage.
- Hearing: investigate nearby shots, ignore remote footsteps, attenuate through walls and consume evidence once.
- Search: hold fixed last-seen evidence, enter search, move during search, then abandon stale evidence.
- Navigation: reach evidence from depot/desert spawns, detour around a barrier and stay out of solids. These are basic reachability tests, not navigation-efficiency scores.
- Tactics: retreat at critical health, strafe in an exchange, seek cover and delay a first shot until after acquisition. These are currently missing capabilities; the benchmark must show those failures rather than award points for unimplemented features.
- Coordination: resolve overlaps, take distinct approaches, respect teammate firing lanes and share a sighting with a teammate who cannot directly see the player.

## Interpretation and limitations

The v1.4 baseline is in `AI-BASELINE-v1.4.md`. The search-movement scenario exposes a case where remembered evidence is separated by an impassable wall and the local search stays stuck. Teamwork/tactical tests expose missing behaviors. Perception/hearing scores of 100 mean passing four basic probes each, not perfect perception/hearing in every map.

This first suite uses controlled arenas and primarily bots 0 and 1; reachability checks cover all six spawn positions in both maps. It is not a distribution of per-bot intelligence and is not a human-playtest result. Future work should add varied maps, player strategies and repeated-seed evaluations as a separately versioned benchmark.

This tooling does not change game behavior or ship a new Windows game binary; the measured game remains v1.4.
