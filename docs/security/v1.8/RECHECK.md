# v1.8 Windows build and security review

Built from c1113719a530e76660ba9dfe11ea22027c200cb7 using Visual Studio/CMake on a fresh GitHub-hosted Windows runner: https://github.com/tffere/tffere-strike/actions/runs/42960034600 . The native gameplay suite passed. Earlier Linux/Windows training mechanics checks also passed for that source. No new learning was performed for packaging.

Microsoft Defender 4.18.26100.4, signatures 1.463.235.0, definition date recorded by the Windows runner as 11/10/2026 9:03:57 PM: runtime-folder scan found no threats, exit 0. Signature update succeeded. The downloaded EXE hash exactly matches the Defender log. Exact logs are retained here.

ClamAV 1.5.4 scanned the complete runtime folder and final release ZIP: 20 files, zero infected, exit 0. Official database refresh succeeded on retry after configuring the relocated certificate directory; daily28179, main63, bytecode339. Both update attempts and the scan log are preserved. No definitions were bypassed. Scans do not guarantee malware absence.

The package was reassembled from the scanned CI artifact, with a plain-text security note, per-file manifest and Defender reports. Archive CRC passes; every runtime source asset matches the source commit, allowing Windows newline conversion. SHA256SUMS identifies exact released bytes. The PE imports WINMM, KERNEL32, USER32, GDI32 and SHELL32; its certificate directory is empty (unsigned). Source searches of the game found no process-launch or download calls; the runtime reads its local graphics/audio/policy assets. These bounded static checks do not prove absence of malicious behavior. The game executable was not run locally; no graphical Windows playtest was performed.

The user explicitly requested exporting v1.8 after the earlier v1.7 alert. The original alert remains unresolved because the antivirus product, detection name and flagged path are unknown. This release is not represented as a verified fix or a confirmed false positive. Keep a flagged copy quarantined and preserve its detection details. No antivirus bypass or exclusion is recommended. Previous versioned releases are preserved.
