# Strike bot policies — v1.7

Default bot-policy-trained.txt is checkpoint400 selected by wins alone during the completed500-round win-only real-map campaign. F5 switches to bot-policy.txt, the exact decision-preserving calibrated starting policy, and restarts. Labels WIN ONLY/SELECTED400 and BEFORE WIN ONLY.

Fresh500 tests versus unchanged AI-player Level0: baseline186wins(37.2%), selected238(47.6%), final500229(45.8%). Selected damage61.7 versus54.1, taken58.85 versus68.3. Selected before fresh tests by validation wins only, no damage/time selection. All500 training rounds completed. Report and provenance: training/runs/2026-11-09-winonly500/REPORT.md. No general human/team skill claim.

Both sides in training share25body/75head rifle, normal cadence, physical recoil and independent per-encounter uniform reaction delays63.397–236.603ms (mean150,SD50). Runtime human input has no artificial delay; bots have it. Broad search remains evidence-only. Level0 learned weights remain frozen; updated controller rules apply. No runtime online learning. Historical bot-policy-after-100.txt remains unused. No Unreal/Blender rebuild.
