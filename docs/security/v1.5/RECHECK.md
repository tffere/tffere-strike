# v1.5 validation and security results

Windows executable build source: `5b6d756`. Fresh Visual Studio/CMake build, checksum-pinned raylib 5.5.

Build, gameplay tests and Defender evidence: https://github.com/tffere/tffere-strike/actions/runs/42518359634

- All 42 native gameplay/pose/policy integration cases passed on Linux and Windows. Headless training mechanics also pass on Linux and Windows. Eight browser-prototype tests pass.
- Microsoft Defender custom scan: no threats, exit 0. Product 4.18.26100.4; signatures 1.463.110.0, last updated November 3, 2026, not updated by this scan.
- ClamAV 1.5.4: 17 files in the downloaded build scanned, zero detections. Verified official databases main 63, daily 28177, bytecode 339. The assembled ZIP also has a separate scan log.
- Downloaded executable matches Defender's recorded SHA-256. Both policy assets match their committed source values; Windows checkout changes LF to CRLF in these text assets. Distribution hashes record the exact Windows bytes. The distribution's READ-ME.txt received a plain-text control/result note after the folder scan; the final archive scan covers that addition.
- Policy files are finite 192×9 Q tables. Tests verify loading both, actual changes after training, weapon-switch delays/ammunition/reloads, obstruction/empty-magazine firing gates, restart persistence, hidden-position isolation and autonomous inference.

The latest training evidence is in training/runs/2026-11-08-real-dual/REPORT.md: exactly 100 rounds with two role-specific learners in real Game::step, then exactly 20 frozen tests against the original player policy. Result 0 wins, 16 losses, 4 timeouts. This is not a demonstrated improvement. The game defaults to the prior validated policy; F5 selects the labelled experimental candidate and restarts the round.

No graphical/listening Windows playtest was performed here. These tests do not establish visual polish, human-like tactics, or malware absence. The executable remains unsigned; if antivirus detects it, keep it quarantined and report the exact detection name. Do not bypass protection.
