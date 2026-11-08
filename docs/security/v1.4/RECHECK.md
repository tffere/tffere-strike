# v1.4 validation and scan results

Windows executable source: `6022a35`, built with Visual Studio/CMake on a fresh GitHub Windows runner using checksum-pinned raylib 5.5.

CI: https://github.com/tffere/tffere-strike/actions/runs/42501343485

- All 36 native gameplay/pose-math cases pass on Linux and Windows; eight browser-prototype cases pass on Linux. New cases cover crouch height, grounded feet, movement/sprint rules, quieter footsteps, low-cover visibility, grounded sound events, airborne crouching, pause/restart and staged collapse curves.
- Microsoft Defender: no threats, exit 0; product 4.18.26100.4, antivirus signatures 1.463.110.0 last updated November 3, 2026. These signatures were not updated in the scan.
- ClamAV 1.5.4 scanned the downloaded build and assets (14 files): zero detections. Verified official databases main 63, daily 28177 and bytecode 339.
- Executable hash matches Defender's recorded hash. The only post-build change in the game directory is a plain-text addition to READ-ME.txt documenting Ctrl crouch and audio credits. Final package hashes and its separate archive scan cover the assembled distribution.
- Five unchanged Kenney CC0 concrete-footstep OGG files were checked with ffprobe and decoded with ffmpeg; their provenance/license and SHA-256 hashes are bundled in assets/footsteps/.

No graphical or listening playtest on Windows was performed in this environment. Pose mathematics and compilation checks do not establish subjective animation quality. Deaths remain procedural jointed animation; physical ragdolls and a Blender/Unreal rebuild are not included.

Scans do not guarantee safety. The executable is unsigned. If antivirus detects it, keep it quarantined and report the exact detection name; do not bypass protection.
