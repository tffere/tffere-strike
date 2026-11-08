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

Policies are text, `STRIKE_Q_V2`, with 192 × 9 finite Q values. V1 had different action semantics and is deliberately rejected. These files are not understood by the current live game; a future integration needs observation/action parity and regression testing first.

## Evidence

See [the run report](../../training/runs/2026-11-08/REPORT.md). It includes the rejected pilot, corrected run, held-out win/draw/loss results, selected checkpoint, reproducibility checks and transfer limitations. The stronger opponent exposed a draw/stalling problem despite improved survival. The live game's prior tactical score remains unchanged.
