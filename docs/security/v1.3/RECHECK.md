# v1.3 validation and security checks

Windows build source: `17e02e7`, built from fresh checkout with Visual Studio/CMake and checksum-pinned raylib 5.5.

CI evidence: https://github.com/tffere/tffere-strike/actions/runs/42499789546

- All 31 native gameplay cases pass on Linux and Windows; eight browser prototype cases pass on Linux.
- Microsoft Defender scanned the rebuilt portable game and reported no threats (exit 0). Product 4.18.26100.4; signatures 1.463.110.0, last updated November 3, 2026. Signatures were not updated in this scan.
- ClamAV 1.5.4 scanned the downloaded files on November 8, 2026: seven files, zero detections. Official verified databases main 63, daily 28177, bytecode 339.
- Downloaded executable hash matches the Defender-recorded hash. Per-file hashes and full scanner logs are included.

These results do not guarantee safety. The executable remains unsigned, and the previously reported antivirus warning is not explained without its detection name. No graphical Windows playtest was performed. If antivirus detects the download, keep it quarantined and report the exact detection name; do not bypass protection.
