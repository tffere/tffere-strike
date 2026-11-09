# Latest frozen-opponent campaigns

`strike-campaign BOT LEVEL0 LEVEL0 EMPTY_OUT CYCLE CONFIG [test-only]` supports freeze_player,random_starts,wide_search,evaluation_games and metric logs/checkpoints every10. Latest protocol: only bot learns3×100, player Level0 frozen,25HP rifle both sides. See [latest report](../../training/runs/2026-11-08-frozen25-next300/REPORT.md); earlier material below is historical.

# Current v1.6 campaign

Use `strike-campaign BOT PLAYER REFERENCE EMPTY_OUT CYCLE CONFIG [test-only]` after building this CMake project. The latest requested two 100-round real-Game::step cycles and 20-round frozen progress evaluations are documented in [the v1.6 report](../../training/runs/2026-11-08-v1.6/REPORT.md). Separate roles both learn; shared physical rules include normal weapon rates and recoil. No measured improvement: 25% before, after 100 and after 200. F5 in v1.6 switches exact before/after policies. Older commands/results below are historical; do not use the simplified Duel for new learning.

# Strike training lab 0.1

A standalone C++17 headless 1v1 self-play trainer for experimenting with high-level combat decisions. No GPU, raylib runtime, Python packages or Unreal installation is needed. It includes collision/visibility/navigation/weapon mechanics from `gameplay.hpp`, but is a simplified simulator, not the full game. Nothing automatically changes the playable v1.4 bots.

## Build and run

```sh
cmake -S native/training -B work/training-build -DCMAKE_BUILD_TYPE=Release
cmake --build work/training-build --config Release --parallel 2
ctest --test-dir work/training-build -C Release --output-on-failure
./work/training-build/strike-trainer work/new-run 17000 200000
```

On Windows, use Visual Studio's CMake tools; the executable is normally `work/training-build/Release/strike-trainer.exe`.

Or use GCC directly:

```sh
g++ -std=c++17 -O2 -Wall -Wextra native/training/train_bots.cpp -o work/strike-trainer
./work/strike-trainer work/new-run 17000 200000
```

The output directory must be empty. Arguments are output directory, training-seed base and final-evaluation-seed base. Each execution trains from scratch for exactly 100 rounds; evaluation/validation do not update the policy. Keep seed ranges separate from each other and the fixed 50000–50019 validation range. Reusing final evaluation seeds to tune the policy invalidates their held-out status. Same-build seeded runs are reproducible; different compilers/platforms may differ in floating-point geometry.

The run saves an initial policy, 10 checkpoints, final-round policy, validation-selected policy (`trained-policy.txt`), complete CSV round/evaluation results, Q values/update counts and metadata. The selected checkpoint can be earlier than round 100. Zero is not a failure result; inspect wins, losses, draws and damage rather than trusting one scalar.

## Policy and learning

One shared tabular Q policy controls both training fighters. Observations encode visibility, approximate remembered range, own health/ammo, remembered cover relationship, evidence availability and selected weapon. No hidden opponent coordinates, health, weapon or action are provided to the learner.

Nine actions: shoot current weapon, engage rifle, engage sniper, hide, chase, flank left, flank right, reload and search. Movement/aiming/pathfinding remain programmed. A switch-and-engage action respects the switch delay and gun cooldown before firing. Shooting is gated by current visibility and obstruction. Both actors' damage is applied simultaneously each simulation tick.

Rewards: time cost −0.01/tick; +0.04 per actual damage dealt; −0.05 per damage received; +12/−12 for win/loss; at timeout +0.02 times HP difference; +0.12 when a critically wounded actor enters cover. Smaller penalties discourage blocked movement, invalid shots, wasted reloads and unnecessary switching. Q-learning uses alpha 0.12, gamma 0.96 per decision and exploration 0.45 → 0.10. Health/damage/accuracy are not improved by training.

Policies are text, `STRIKE_Q_V2`, with 192 × 9 finite Q values. V1 had different action semantics and is deliberately rejected. v1.5 can load this policy schema; deployment still requires matching observations/actions and an explicit versioned policy choice.

## Evidence

See [the run report](../../training/runs/2026-11-08/REPORT.md). It includes the rejected pilot, corrected run, held-out win/draw/loss results, selected checkpoint, reproducibility checks and transfer limitations. The stronger opponent exposed many mutual eliminations; the earlier survival/stalling interpretation was incorrect. The live game's prior tactical score remains unchanged.

## Current workflow: real game code only

Per the user's instruction, future learning must use actual `Game::step`, not the historical simplified simulator. The latest requested workflow is **100 rounds with two independent learners**, then **20 frozen tests of the final game-bot policy against the saved original player policy**:

```sh
./work/training-build/strike-real-dual native/assets/bot-policy.txt work/new-real-dual-run
```

On Windows, the executable is normally `work/training-build/Release/strike-real-dual.exe`. Output must be empty. Both original policies, two final policies, 10 checkpoints per role, all 100 training and 20 test results are saved. Winner 0 is game bot; 1 is player; −1 is timeout. Evaluation checks that no Q values change. The latest result is 0 wins / 16 losses / 4 timeouts; it is not an established improvement.

The optional `strike-actual-trainer` is an older one-learner experiment, while `strike-trainer` is historical simplified training. Neither is the current user-requested workflow. See [the authoritative report](../../training/runs/2026-11-08-real-dual/REPORT.md).
