# 25 HP rifle: next 300 rounds versus AI-player Level 0

Exactly300 real-map training rounds, three consecutive100-round bot-only cycles, latest bot continued each cycle against **AI-player Level0**, the original frozen before-player-policy.txt. Both spawn at independently seeded random connected navigation cells, minimum8m separation, independent random headings. Game::step, desert map,0.05s physics steps, no graphics/frame pacing. Simulation runs faster than wall-clock time. Both aim to kill; the AI-player reacts but NEVER learns or explores random actions. Frozen-player Q values/visits asserted unchanged per episode and original/final files match. No simplified simulator. Both sides share25body/75head AR-7 damage (36×0.7 rounded),0.115s rifle cooldown; sniper125/220,1.35s. Physical recoil/spread, health and other rules unchanged.

## Fixed progress tests (100 each, no learning or exploration)

| Additional rounds | Wins | Losses | Mutual eliminations | Timeouts | Avg damage dealt | Avg taken | Mean win seconds | Hit rate |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
|0|40|58|2|0|58.00|64.50|10.97|53.3%|
|100|40|28|32|0|75.00|68.25|10.61|66.7%|
|200|40|28|32|0|74.00|68.25|10.49|57.6%|
|300|42|26|32|0|75.00|67.00|10.34|65.4%|

Same100 seeded cases at each checkpoint; starting positions/headings match across policies. Search/controller/weapon rules included BEFORE baseline, not credited to learning. Config/reward settings fixed through all3 cycles. Evaluations don't alter policies. Intermediate damage gains must be read alongside mutual eliminations and losses; damaging a target while dying is not winning or damage avoidance. Training metrics use changing policy, exploration and different encounters; they cannot establish controlled improvement.

## Every50 training rounds

| Rounds | Avg damage dealt | Avg taken | Hit rate | Mean win seconds | Wins/50 |
|---|---:|---:|---:|---:|---:|
|1–50|68.5|62.0|58.1%|16.07|22|
|51–100|59.5|78.5|56.9%|13.35|15|
|101–150|78.5|60.0|58.1%|14.79|21|
|151–200|65.0|62.5|47.1%|12.34|22|
|201–250|75.0|59.5|67.6%|11.31|24|
|251–300|71.0|69.0|60.2%|14.41|20|

![Fixed-test progress](progress.png)

## Separate final-only test cases

heldout-before: 51 wins, 48 losses, 1 mutual eliminations, 0 timeouts; damage 65, taken 53.25, mean winning seconds 11.1441.

heldout-after: 41 wins, 30 losses, 29 mutual eliminations, 0 timeouts; damage 73, taken 62, mean winning seconds 12.0878.

Final-only win change: -10 percentage points on100 cases. This small sample against one controller does not establish broad skill improvement. Repeated progress and separate final-only cases have disjoint seeds from training. No weight/reward selection after the final-only comparison. Final300 checkpoint is preserved even when another checkpoint scores better on one metric.

## Search, learning and metric definitions

Broad search patrols world-centered radius40 clamped to map; evidence-centered search radius12 and18s after lost contact; longer route commitment; legal sprint toward distant unseen goals. No routing from live hidden target position. This is programmed control, not a learned navigation network. Only high-level192×9 Q scores learn. Player and bot controllers still differ despite equal physical settings. Current settings favor wins/damage/survival/time via +200win,-150loss,-120timeout,+0.5 per effective HP dealt,-0.45 per HP taken,-0.02 per0.05s frame,-0.5switch,+0.05 once per new4m cell while unseen. Mutual elimination terminal penalty remains5; this is much milder than a loss and may incentivize trades. The intermediate high-trade results suggest future validation should test a stronger mutual-death penalty; no extra optimization rounds were silently added. Alpha0.15,gamma0.998,epsilon0.25→0.05 each cycle. All configs saved.

Hit rate=PlayerHit events from bot0/BotShot events. Damage=actual health removed (capped by remaining HP). First-shot time from round start,-1 when never fires; aggregates exclude those rounds and show denominator. Visibility sampled before each0.05s frame; encounter=contiguous visible segment, can flicker. Damage/encounter is pooled damage divided by visible segment count, not a tactical engagement classifier. Visible-without-shot time includes normal cooldown; ready-visible-without-shot excludes cooldown/switch/reload/empty ammo but NOT aim misalignment. These counters alone do not prove a firing bug. Winning-time means exclude losses/draws; compared subsets differ. Unique4m cells measure traversed cells, not observable coverage. Six50-round blocks and thirty10-round blocks, all raw rounds,10-round bot checkpoints, before/after/reference policies and configs are preserved.

Reproduce with strike-campaign BOT LEVEL0 LEVEL0 EMPTY_OUT CYCLE CONFIG; cycle0 plus test-only for baseline;1→2→3 continue only bot. evaluation-games is100 for these experiments. campaign-tests checks frozen roles, spawn clearance, deterministic replay and counters/hidden-position isolation. Historical36-HP results and previous25-HP runs have different seeds and may not be directly compared as learning gains. Windows six-bot human play remains different from1v1 training.
