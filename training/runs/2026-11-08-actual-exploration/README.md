# Superseded exploration — not the latest requested test

These are earlier one-learner actual-game trials and an additional classic-mode reference. The authoritative dual-learner 100+20 experiment is in ../2026-11-08-real-dual/REPORT.md.

The preliminary forced-control comparisons appeared to improve wins from 2/100 to 16/100. That comparison was invalid: the frozen starting policy was not evaluated through exactly the playable autonomous control path. After matching the runtime path, starting and selected policies both produced 16/100 wins. The corrected single-learner validation retained checkpoint 0. Earlier headlines claiming a new 14-point gain were withdrawn.

The separate classic-mode reference uses different seeds (840000..840099): classic 0/50 wins vs chaser and 0/50 vs defender; trained mode 0/50 vs chaser and 17/50 vs defender. This compares a rule-based rifle-only mode with a policy-enabled mode that also has new weapon mechanics. It does not isolate the effect of the new 100 training rounds and is not the latest 20-test result.

Keep all CSVs and policy files for audit. Do not present any of these as evidence that the latest dual-learner candidate improved.
