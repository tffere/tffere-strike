# v1.7 build and scan evidence

Runtime source c864c143e667caac9bd5f130611929033a94a1ce. Fresh VisualStudio/CMake build with checksum-pinned raylib5.5.

Windows gameplay/build/Defender: https://github.com/tffere/tffere-strike/actions/runs/42565492964

Training builds/mechanics Linux+Windows: https://github.com/tffere/tffere-strike/actions/runs/42565492742

50 native cases pass, including reaction distribution moments, initial-fire delay and reacquisition; legacy mechanics fixtures explicitly disable reaction delay to isolate their subjects. Two headless training checks cover frozen weights, reward purity, first-visit updates, probability range, spawns and counter invariants. Earlier Windows training lambda-capture error was corrected before these successful builds.

MicrosoftDefender reports no threats, exit0; exact scanner/signature version and update date are preserved verbatim in defender-scan.txt (not claimed newly updated). Downloaded EXE matches Defender SHA256. ClamAV1.5.4 with verified official databases main63,daily28177,bytecode339 reports zero detections; full folder/archive logs are retained. These are the available verified databases, not a claim of latest signatures. Runtime policy values match committed experiment snapshots; trailing whitespace is normalized in source assets, Windows text usesCRLF. Distribution READ-ME received a plain-text note after folder scanning; final ZIP scan covers it. File hashes and ZIP checksums record exact bytes.

Latest experiment:500 bot-only real Game::step training rounds, exact win-only1/0 reward, Level0 Q weights frozen. Q calibration preserves all greedy masked choices. Checkpoint400 selected on validation wins before fresh tests; fresh500 wins186→238(37.2→47.6%), final500229(45.8%). No additional training or fresh-set-driven reselection. See training/runs/2026-11-09-winonly500/REPORT.md. Shared AI reaction population mean150ms/SD50ms; tick quantization and aim/readiness add latency.

No graphical/listening Windows playtest performed. No malware-absence guarantee; EXE remains unsigned. No Unreal/Blender work or online learning during human play.
