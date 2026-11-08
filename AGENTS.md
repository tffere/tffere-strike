# Project instructions

Read `docs/PROJECT-STATUS.md` and the recent Git log before continuing work.

## User's release policy

Every newly shipped game update must increment the version. Keep the version in the window title, CMake project, README, release notes, Git tag and Windows ZIP filename consistent. Use a patch increment for rebuilt/repackaged binaries and a minor increment for gameplay features. Development commits may share an unreleased target version.

Create a Git tag and GitHub Release for each shipped version, with its versioned ZIP and SHA-256 checksum. Do not replace or delete previously published versioned assets or move existing tags. The unversioned latest-download alias may point to the newest release. Document validation and limitations in the release notes.

Run native gameplay tests and scan each newly built Windows executable/package before publishing. Preserve the exact scan reports and file hashes for that version. Do not describe an antivirus scan as a guarantee of safety or tell the user to bypass antivirus.

Record new user requests, completed work and unfinished work in `docs/PROJECT-STATUS.md` so another chat can resume without the prior conversation.
