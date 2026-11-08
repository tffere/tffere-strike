# v1.6 validation and security results

Windows runtime build source: `b2661a407888c276e46d508d1e7655580cc8a5b5`. Fresh Visual Studio/CMake build, checksum-pinned raylib 5.5.

Build, native tests and Defender: https://github.com/tffere/tffere-strike/actions/runs/42532099126

Training build/mechanics (Linux and Windows): https://github.com/tffere/tffere-strike/actions/runs/42532099157

- All 49 native gameplay/pose/policy cases passed on Linux and Windows. Headless training mechanics pass on both. Eight historical browser-prototype tests pass.
- Microsoft Defender custom scan: no threats, exit 0. Product 4.18.26100.4; signatures 1.463.110.0, last updated November 3, 2026, not updated by this scan.
- ClamAV 1.5.4: 18 files in the downloaded build scanned, zero detections. Verified official databases main 63, daily 28177, bytecode 339. The assembled ZIP also has a separate scan log.
- Downloaded executable matches Defender's recorded SHA-256. All three policy assets match committed source values. Current baseline/final model values match exact experiment snapshots (only trailing whitespace is normalized in runtime text). Windows checkout converts LF to CRLF; the distribution manifest records exact Windows bytes.
- Distribution READ-ME.txt received a plain-text control/result note after the folder scan; the final archive scan covers the addition. ZIP integrity and executable hash were checked after assembly. Individual file hashes are bundled; final ZIP hashes are in downloads/SHA256SUMS.txt and the release asset.
- Tests cover stance spread ordering, recoil/recovery, Alt walking, policy loading, switch-loop prevention/draw delay, legal TD masks, blocked flank routing, shared damage/cadence, simultaneous elimination draws, ammo/reloads, sensory isolation and collisions.

Latest training: two consecutive 100-round cycles in actual Game::step, with two separate learning roles, then frozen 20-round tests after each. Recoil-phase baseline/after100/after200 all score 5 wins, 14 losses, 1 mutual elimination, 0 timeouts (25%). Both models changed, but no measured improvement. Verification replay from the initial models after final parity fixes produced identical CSVs/models. Full records: training/runs/2026-11-08-v1.6/REPORT.md.

No graphical/listening Windows playtest was performed here. Scans do not guarantee malware absence; the executable remains unsigned. No Unreal or Blender rebuild was performed.
