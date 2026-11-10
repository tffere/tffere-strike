# Bot policies — v1.8 source candidate

`bot-policy-context-v3.txt` is the selected expanded policy, STRIKE_Q_V3:49,152 possible context states ×12 actions. It uses297 learned sparse rows plus inherited/shared parent scores; this does not mean every possible state was trained. Source runtime defaults to this model. F5 switches to the unchanged v1.7 V2 baseline in `bot-policy-trained.txt` and restarts. Old `bot-policy.txt` and `bot-policy-after-100.txt` remain historical assets, unused by the new F5 comparison.

New context: aim alignment, own fire readiness, nearby usable cover, finer range, visible enemy facing/weapon and own map region. Enemy details are gated by actual visibility; hidden live target positions never enter the table. Three added route actions choose east, west or central cross-map paths from world information, with reachable goals and commitment until evidence interrupts them.

Exactly1,000 new bot-only win-only rounds ran through real Game::step at120Hz, high-level decisions250ms, against unchanged AI-player Level0. Initial inheritedV3 and V2 baseline have identical400-game outcomes. Checkpoint400 selected before fresh tests:234/400 validation wins versus201/400 baseline. Fresh1000:491 baseline wins→574 V3 wins(49.1→57.4%), damage61.55→70.075,taken54.65→46.8. Winning time increases13.54→20.82s; mutual deaths6→56 and3timeouts. Final1000 checkpoint also wins574; earlier checkpoint remains selected by prior validation, not fresh-test reselection. Full source snapshot, policies, metrics and gate are under training/runs/2026-11-10-expanded-q1000/.

This is sparse hierarchical tabular learning, not a neural network, online learning, or learned aiming/physics. Results concern one frozen opponent and map-start distribution, not guaranteed human/team performance.

No v1.8 Windows release ZIP is published while the reported v1.7 antivirus alert remains unresolved. See docs/security/v1.7/alert-2026-11-10/REPORT.md. New learning results do not resolve an antivirus alert.
