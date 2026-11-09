# Project instructions

Read `docs/PROJECT-STATUS.md` and the recent Git log before continuing work.

## User's release policy

Every newly shipped game update must increment the version. Keep the version in the window title, CMake project, README, release notes, Git tag and Windows ZIP filename consistent. Use a patch increment for rebuilt/repackaged binaries and a minor increment for gameplay features. Development commits may share an unreleased target version.

Create a Git tag and GitHub Release for each shipped version, with its versioned ZIP and SHA-256 checksum. Do not replace or delete previously published versioned assets or move existing tags. The unversioned latest-download alias may point to the newest release. Document validation and limitations in the release notes.

Run native gameplay tests and scan each newly built Windows executable/package before publishing. Preserve the exact scan reports and file hashes for that version. Do not describe an antivirus scan as a guarantee of safety or tell the user to bypass antivirus.

Record new user requests, completed work and unfinished work in `docs/PROJECT-STATUS.md` so another chat can resume without the prior conversation.

## Bot learning protocol

Use the real `strike::Game::step` code for future learning experiments, not the historical simplified Duel simulator. The latest requested protocol is two separate learners (one AI player, one AI game bot) for 100 rounds, followed by 20 frozen tests of the final game bot against the saved original player policy. Save before/after policies and every outcome. Do not count validation or testing as training. Report wins, losses and timeouts separately; a changed policy is not proof of improvement. Do not mix the tactical benchmark score with duel win rate. Retain poor/superseded results with corrections rather than hiding them.

## Latest protocol override

User now names the original frozen player policy AI-player Level0. New campaigns train ONLY the bot against that unchanged policy, with random valid connected-map spawns/headings,100 rounds per cycle repeated3 times. Preserve separate baselines for physical-rule changes and report damage/taken, conditional winning time, hit rate and mutual eliminations, plus10/50-round blocks and frozen tests. Latest AR-7 body damage25/head75 both sides. Do not resume old dual-learner campaigns without instruction.
