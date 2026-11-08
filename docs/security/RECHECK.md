# Windows package recheck — November 8, 2026

The user reported a virus warning for the earlier download. The exact detection name has not been supplied. It is not established whether that warning was a malware detection or an app-reputation warning.

## Checks performed

- Reviewed the native game source and changes, build scripts, distribution contents and Windows PE imports. No obvious malicious behavior was identified. The game loads bundled font/image assets, uses window/input/graphics/audio services and can write a gameplay screenshot for capture/smoke-test flags. Static review is not proof of safety.
- Scanned the original downloads and local cross-compiled executable with ClamAV 1.5.4: zero detections. See `original-clamav-scan.txt`.
- Fixed an MSVC lambda-capture compilation error and rebuilt from source commit `4786a0c` on a fresh GitHub-hosted Windows runner using Visual Studio/CMake, rather than reusing the earlier Zig-built executable. Raylib 5.5 is fetched with its pinned SHA-256.
- Windows gameplay tests passed (all 25 cases in the gameplay test executable).
- Microsoft Defender custom scan of the rebuilt portable directory reported no threats, exit code 0. Product version 4.18.26100.4; antivirus signatures 1.463.110.0, last updated November 3, 2026. The scan did not update those signatures. See `defender-scan.txt`.
- ClamAV 1.5.4 scanned the downloaded rebuild and bundled assets: seven files, zero detections. Official databases were downloaded and verified during this recheck (main 63, daily 28177, bytecode 339). See `rebuilt-clamav-scan.txt`.
- The rebuilt executable SHA-256 exactly matches the hash recorded by Defender. Per-file hashes are recorded in `rebuilt-file-hashes.json`.

Build/test/Defender evidence: https://github.com/tffere/tffere-strike/actions/runs/42498115610

## Limits

No scanner or static review guarantees absence of malware. The executable remains unsigned and may still trigger an unknown-publisher or SmartScreen warning. We have not reproduced or resolved the user's original detection without its name. No protection exclusions, warning bypasses, obfuscation or executable packing were added. No graphical Windows playtest was performed in this session.

The replacement retains the v1.2 gameplay features. Extract the complete ZIP into a new folder. If your antivirus still detects it, keep it quarantined and report the exact detection name and file SHA-256; do not disable protection.
