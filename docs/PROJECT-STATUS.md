# Project handoff — v1.7 current; latest1000-round research/audit below

## User requests

- Make bots move more naturally, including running, jumping and crouching.
- Bots must shoot only when the player is in their vision and objects do not block them.
- Preserve progress in the repository so another chat can continue without the old conversation.

## Implementation

The current game is native C++17/raylib under `native/`. The JavaScript files are an earlier browser prototype.

Bot state and combat checks: `native/gameplay.hpp`. Articulated poses: `native/bot_renderer.hpp`. Rendering/tracers: `native/main.cpp`. Regression checks: `native/tests.cpp`.

v1.2 adds distance-dependent run/walk speeds, gradual turning, staggered jump/crouch actions, posture-aware hitboxes and eye/muzzle origins. Shot checks run after movement and require field of view, aim alignment and clear eye, barrel and firing paths. Bots still navigate to the player when hidden. Jumping does not vault cover.

## Validation and next step

25/25 native gameplay tests and 8/8 browser prototype tests pass. The Windows package is cross-compiled with Zig 0.13.0. Graphical v1.2 Windows playtesting remains to be done by the user; this session has no Wine or graphical test runner installed. Prior v1.1 screenshot/smoke results do not establish v1.2 animation quality.

Download `downloads/TffereStrike-v1.2-Windows-x64.zip`, extract it separately from v1.1 and run `TffereStrike/TffereStrike.exe`, keeping `assets` beside the executable. Earlier versioned ZIPs are preserved.

For future sessions, read this file and the Git log first. Record new user requests and unfinished work here before a long break or usage limit.

## Security recheck follow-up

The user reported a virus warning and requested a recheck/repackage. Exact detection name is still unknown. See `docs/security/RECHECK.md` and included reports for results and limits. Original downloads had no ClamAV detections. The replacement is an MSVC Windows CI build from `4786a0c`, passed Windows gameplay tests and Defender, and was independently scanned with ClamAV. Use `downloads/TffereStrike-v1.2-Windows-x64-rebuilt.zip`; the default and v1.2 ZIP aliases were also replaced. Old cross-compile statements above describe the superseded package. Do not instruct the user to bypass antivirus. Graphical Windows playtesting remains outstanding.

## v1.3 — latest user requests and implementation

The user requested bots that explore and infer the player's location from sight, gunshots and footsteps, remembering/searching when contact is lost rather than using live hidden coordinates. They also requested persistent bullet marks on walls/buildings and a version increment for every shipped update going forward.

Implemented in `native/gameplay.hpp`: Patrol/Investigate/Chase/Search states, independent initial facing and reachable exploration goals, fixed remembered evidence, finite search, discrete gunshot/footstep/landing cues, wall attenuation, approximate hearing locations, and bounded surface-impact state. No live hidden-player location enters navigation. Rendering in `native/main.cpp` projects marks onto visible facade geometry; marks reset each round. All 31 native tests pass on Linux; Windows build/tests and Defender run in CI. Graphical Windows playtesting remains outstanding.

Release policy is now in `AGENTS.md`: increment every shipped version, including patch rebuilds; preserve existing tagged sources and versioned ZIPs; create a GitHub Release with notes and hashes. v1.2 was retrospectively tagged at b390847 with its historical rebuilt package. v1.0/v1.1 tags already exist. This feature release is v1.3; see its release and version-specific scan reports.

## v1.4 — crouch, bot collapse and sampled footsteps

User asked for roughly 15 minutes improving bot appearance/falls, realistic footsteps and player crouch. Implemented staged jointed death poses in `native/bot_pose.hpp` and `native/bot_renderer.hpp`, end-of-round animation completion, clearer knee proportions and movement weight shift. Ctrl crouch updates gameplay eye height, speed, grounded physics, hearing and audio events. Five Kenney CC0 concrete-step samples replace footstep synthesis; attribution/provenance/hashes are bundled. Nearby bot footsteps and landing sounds now follow events and volume attenuation. Camera bob follows actual grounded travel. All 36 native cases and eight prototype cases pass. Windows build/Defender evidence: run 42501343485, source 6022a35. Version-specific reports are under docs/security/v1.4/.

The user asked about Blender, Source and Unreal and explicitly said NOT to start an Unreal project or Blender rebuild yet. Neither was started. Current bots remain procedural C++/raylib meshes, not imported rigged characters or physical ragdolls. A graphical/listening Windows playtest is outstanding; do not claim that pose-math tests verify visual realism.

## Tactical intelligence measurement

User requested a measurable bot IQ score and asked for the current score. Added `native/ai_benchmark.cpp`, a deterministic 24-probe, six-dimension tactical benchmark, plus methodology and a frozen v1.4 baseline under docs/. This is a scenario score, not human IQ or visual quality. It intentionally measures missing tactics/teamwork instead of masking them with basic correctness passes. No game behavior/binary was changed; game remains v1.4. Read `docs/AI-BENCHMARK.md` before comparing future versions. Keep benchmark thresholds and weights fixed, and do not claim four basic sensor passes mean universally perfect perception.

## Offline self-play training lab 0.1

User authorized training bots against each other for 100 rounds to improve combat action choices, with progress at least every 10 minutes. Implemented native/training/ (standalone C++17/CMake, no graphics dependency) and executed a 100-round pilot, then a corrected independent 100-round experiment after the pilot regressed. Reproduced the corrected run from scratch with matching CSV/policy values. Two fighters share one learning Q-table (192 states, 9 actions); evaluation freezes it against two fixed opponents using separate seeds and alternating sides. Validation selects checkpoint 30, although all 100 rounds completed. Final policy/10 checkpoints/raw logs/report are under training/runs/2026-11-08/.

Corrected held-out wins vs rifle chaser: 42% starting → 68% selected. Vs weapon-aware defender: 28% → 22% wins, losses 60% → 14%, draws 12% → 64%. Correction: 52 trained-policy draws were mutual eliminations and 12 were timeouts; the timeout count did not increase. The earlier survival/stalling interpretation was incorrect. Report retains rejected pilot (wins 43→22 and 31→17), policies, schema difference and limitations. Final tests use seeds 200000..200099; do not tune against them. Full gameplay regression tests still pass.

This is a simplified 1v1 simulator, not a new Windows game release. Live game remains v1.4 and tactical benchmark remains 62.5 overall, 0 tactics. No learned policy is installed in the live game. Future transfer needs a new version and aligned runtime weapons/observations/actions; the current game does not load STRIKE_Q_V2 policy files. User explicitly said not to start Unreal; no Unreal project was started.

## v1.5 and latest authoritative 100+20 experiment

User requested a versioned game update using trained policies, then explicitly required learning only through real game code: one AI controls the player and one controls a game bot, BOTH learn for 100 rounds, then test the final bot against the saved original player policy for 20 rounds. Implemented native/training/train_real_dual.cpp using actual Game::step and separate role Q tables. Exactly 100 training and 20 frozen tests completed. Result: 0 bot wins, 16 losses, 4 timeouts. Both role tables changed; evaluation checks no learning occurred. All originals, finals, 10 checkpoints per role, CSVs, hashes and the authoritative report are under training/runs/2026-11-08-real-dual/.

Earlier simplified/one-learner results must not be mixed with this test. The earlier claimed actual-game 2%→16% gain was invalid due to an inference/control mismatch and was withdrawn. Superseded actual-game exploration logs are preserved separately. In the corrected one-learner comparison, the starting policy already achieved 16/100 wins; the extra training showed no further gain. Do not use that preliminary claim.

v1.5 loads the prior validated policy by default. F5 switches to after-100-round experimental policy and restarts the round; F5 returns. Assets: bot-policy.txt (prior) and bot-policy-after-100.txt (final requested bot candidate). The trained player policy is saved for research only. New policy integration includes weapon selection/ammo/reload/switch delay, cover/chase/flank intent and sensory gates. Rifle damage stays 9; new bot sniper is 31, preserving deliberate player-vs-bot role asymmetry. No game-time online learning, Unreal project or Blender rebuild was started.

42 native tests pass on Linux/Windows; training mechanics pass on both. Windows build/Defender source 5b6d756, run 42518359634. Both model assets match training snapshots. Scans and policy hashes live in docs/security/v1.5. Graphical Windows playtest is outstanding. Default-policy tactical benchmark remains 62.5 overall, tactics 25 and navigation 75; this is separate from the 0/20 win result and is not broad improvement.

## v1.6 — current work, shared rules and latest 200-round result

User explicitly requested all physical player/bot settings equal, normal weapon fire rates, physical recoil, crouch/slow-walk accuracy improvements, sprint/jump accuracy penalties, then another 100 rounds repeated twice. Implemented shared weapon damage/cooldowns, health/hitboxes/movement/gravity, recoil/spread and Alt slow walk; simultaneous same-step lethal exchange draws avoid player-first damage bias. Guarded switching/TD masks, firing while moving and tactical-goal path fix address prior failures. Both AI controllers retain different routing/spawn roles but same physical rules; nine learned actions do not include jumping.

Latest authoritative recoil experiment completed EXACTLY 2×100 training rounds; both separate role tables changed each cycle, learning rate equal. Before/after each cycle: same frozen original player, same 20 seeds, no learning. All three results 5W/14L/1 mutual elimination/0 timeout =25%, mean damage48,45 bot shots,1 switch,5 zero-shot games. NO improvement measured. Training outcomes34W/63L/3 draws then36W/60L/4 draws are against changing opponents and cannot establish improvement. Final bot is default AFTER200/RECOIL; F5 selects exact starting BEFORE200/RECOIL baseline. Values differ but test trajectories stayed unchanged.

Preserved all raw records in training/runs/2026-11-08-v1.6/: older asymmetric3×100 (all0/20 wins); equal-before-recoil100 (45%→55%, small sample); current recoil2×100 (25% unchanged). Do not combine rule/seed phases or credit weapon fixes as learning. Larger5×100 and4×200 campaigns were paused when user narrowed request; do not resume them blindly. Current requested two additional cycles are finished. No Unreal/Blender rebuild started.

49/49 native cases pass on Linux and Windows; training mechanics pass on both, plus 8/8 historical prototype checks. Windows runtime source b2661a4, native CI 42532099126, training CI 42532099157. Defender/ClamAV folder scans report no detections; exact logs and hashes are under docs/security/v1.6/. Final package scan is recorded there separately. v1.6 ZIP is versioned and older assets preserved. A verification replay from the same initial tables after human draw-delay/crouched-speed parity fixes produced identical CSVs and models; this was not continued learning beyond round 200. Graphical/listening Windows playtest remains outstanding. User wants versioned GitHub releases/EXEs, all prior assets preserved. Further improvement needs state/action visitation/rank diagnostics and richer positional/readiness observations rather than assuming more rounds guarantees gain.

## Frozen-player random-spawn 300-round campaign (36 HP rifle)

Completed user-requested3×100 bot-only learning versus original frozen player, starting latest v1.6 bot. Random connected-map spawns/headings; broadened evidence-only search included in baseline. New per-frame diagnostics and10-round aggregates/checkpoints. Frozen100-case progress35→47→50→48 wins; damage59.07→61.35→63.11→62.30, taken72.20→58.79→57.32→58.96. Separate final-only100-case comparison39→47 wins, damage57.88→62.08,taken66.04→56.27, winning time14.73→12.45s. All raw files, metrics definitions/charts and policies in training/runs/2026-11-08-frozen300/. Player values/visit counts asserted unchanged. These results use36 HP body damage and are not measurements of the subsequently requested25 HP rule. Larger historical campaigns remain paused.

Latest steering: reduce AR-7 body36→25 (36×0.7=25.2 rounded), both sides, retain75head; then another3×100 from latest bot versus frozen player, random starts, same metrics. New baseline required. Work underway; no new Windows game shipped yet.

## v1.7 and latest25HP campaigns

AR-7 body36→25 for both sides (round25.2), head75 unchanged. Completed requested3×100 versus unchanged AI-player Level0, starting the preceding36HP300-round final; then user requested another3×100, completed from first25HP final. Both use real-map random connected spawns/headings, same broad sensory search, bot-only learning, all diagnostics/checkpoints. First25HP campaign fixed100 wins49→46→48→50; separate51→50. Latest300 fixed100 wins40→40→40→42, damage58→75,taken64.5→67, mutual2→32; separate51→41. No overall improvement, latest candidate remains optional; default preserved before-latest300 baseline. Mild mutual-death penalty5 versus loss150 rewards trades; next optimization should validate stronger trade penalties, NOT silently add rounds.

Records training/runs/2026-11-08-frozen25-300 and2026-11-08-frozen25-next300; earlier36HP campaign separate. Level0 policy weights never changed; physical rules changed between campaigns. Default runtime labels AFTER300/BASELINE; F5 AFTER600/EXPERIMENTAL, counting25HP rounds only. User asked detailed50-round metrics and reaction timing; current training sensing/firing every50ms, high-level choices250ms, maxaim3.5rad/s, no fixed human delay.

v1.7 runtime source/build/package verification underway. 49 native tests passed after damage reduction. Graphical Windows playtest still outstanding. No Unreal/Blender work started.

## Latest continuation: stronger trade penalty, additional300 completed

User asked another300 from latest bot against Level0. Completed3×100 from previous final, same25HP physics/broadsearch/frozenplayer, new seeds. Disclosed mutual-death reward penalty raised5→150, all else fixed. Fixed100 progress43→44→49→50 wins, mutual28→14→3→1; damage73→65. Separate final-only100 comparison52→45 wins, mutual15→1: no established general win improvement. Raw logs and10/50-round metrics/policies/checkpoints in training/runs/2026-11-08-frozen25-survival300. This makes900 accumulated25HP rounds over3 requested batches, preceded by300 at36HP. Finalafter900 candidate optional; default remains preservedafter300 baseline.

Added weapon-specific lethal-hit counters. Prior batch300 replay verified every original round field and final table bytes identical; counted sniper kills per50 as2,17,28,21,19,23, including mutual deaths. Saved audit beside prior report, not new accumulated training. Fixed MSVC lambda capture in spawn-pool utility after Windows training CI failed; Linux tests pass. v1.7 release verification in progress.

## Win-only500 and reaction delay: latest authoritative result

Added shared ReactionDelay to bots and AI-player controller: independent seeded uniform[63.397,236.603]ms per new/reacquired sight, exact population150ms mean/50ms SD. Gates firing only, no human input delay. Sampling once per encounter; per-episode independent role seeds. Sim tick50ms quantizes actual firing; aiming/readiness adds time. Native50/50 and training2 checks pass on Linux. Prior shaped-reward500 complete (reaction500 folder), regressed37→32% fresh.

User explicitly requested win-rate-only500. Implemented exact1win/0allother outcome, no shaping/time discount. First-visit episodic MonteCarlo,alpha.08, oldQscale calibrated[.45,.55] preserving all192×511 masked choices. Exactly500 rounds from latest shaped500 final, onlybot learns, frozenLevel0 unchanged. Validation400 wins156→161→145→165→195→184; selectedcheckpoint400 by wins-only before fresh tests, tiesbaselineearlier. Fresh500 baseline186W/300L/14mutual, selected238W/253L/9mutual, final500229W/259L/12mutual. Selected37.2→47.6%, damage54.1→61.7,taken68.3→58.85. Paired safety gate gained68/lost16 wins,p4.27e-9 passed. This is improvement on defined one-opponent test distribution, not universal guarantee. Full audit/source/checkpoints in training/runs/2026-11-09-winonly500/.

v1.7 target updated to default selected400 andF5startingbaseline; title/CMake1.7. Preserves all older releases. Windows runtime c864c14: native CI42565492964 passed50 cases+Defender, training CI42565492742 passed bothOS. ClamAV folder and finalZIP scans zero detections; exact reports docs/security/v1.7. Downloaded EXE matched Defender hash; assets matched selected400/baseline; versioned ZIP assembled, all oldZIPs preserved. Releasev1.7 publication follows. User wants full50-round tables with sniper kills; metrics saved. No more training beyond requested500.

## Latest1000 learning rounds and frozen diagnostic audit — complete

User authorized up to1000 win-only rounds to improve wins vs Level0. Exactly2 independent500-round branches from v1.7 default, unchanged physical rules/Level0. Decreasingalpha/epsilon conservative/exploratory. Best raw288/600validation vsincumbent284. Before fresh access, explicitly disclosed adaptive extension of5 Qblends (no additional learning); .5blend selected291/600. Fresh1000 incumbent477W/492L/31mutual, selected479W/445L/76mutual, no timeouts. Gained74/lost72 wins,p.467,95%paired difference roughly−2.17..+2.57pp. Gate FAILED: keep v1.7, no new release/model replacement.1000 budget exhausted. Archive training/runs/2026-11-09-winonly1000-audit/REPORT.md includes allrawdata/configs/plans/amendment/selection/checkpoints.

User then requested hypotheses/testing: concern close spawns and strong human play despite flat frozen wins. Completed frozen-only diagnostics (no additionallearning): training median34.75m, only5.3%under12m. Actual v1.7ZIP default equals1000incumbent; prior37.2→47.6% gain already shipped. Standard400 firstsight5.5%; contact firstshotmedian25.32m amongfiringrounds. Near200 wins56.5%,mid52%,far40.5% (v1.7), so close-spawn explanation unsupported. Same200 cases20Hz v1.7wins82→120Hz102; microreactionready174.75→153.90ms, rifleinterval150→116.67ms. Symmetric20m sniper control1000 seedpairs mutual26.1%at20Hz→5.6%at120Hz. Important benchmark-fidelity issue, not a bad win counter; finer-step baselines needed before futurelearning.

State capacity test100 deliberatelysameencoded130 snapshots×4 one-decisioncounterfactuals:23 havewin/nonwin dependingonaction, contradictoryshoot/chase preferences; supportsstatealiasing. About86.7%standardv1.7decisionmasksonlySearch, so learner cannotlearnmostsearchrouting. Identicalweightsroleprobesfavorbotintheseconditions, not universalplayeradvantage. Full DIAGNOSTIC-REPORT.md andraw4800labelledprobes,2000distance replays,400acceptedcounterfactualrollouts plusmicrocontrols archived. No changes to shipped physics/models or covertLevel0learning.

Added headless-only audit tools and generalized PlayerOpponent.observe/input(dt=.05) for fine-step diagnosticfairness: timers, reaction andaimlimitcorrectlyscaled. Default50msbehavior preserved;50goldenepisodesmatchoutcomes/ticks/damage/shots. Testsinclude dt-scaledaim. Nextrecommended: training/eval120Hzwith250msdecisions, richerobservations andmeaningfullearnablesearchchoices, retainwin-onlyrewardandfreshgate. Do NOT runmorelearningwithoutnewauthorization;1000completed. v1.7 release54e1205 remains current.

Audit source d4ae439 passed Linux+Windows headless builds and2 tests (CI42570420559), plus Windows native build/gameplay checks (CI42570420555). Full report and archives pushed; no new release or policy replacement. v1.7 remains current.

## v1.7 antivirus alert Nov10; expansion work newly authorized

User reportsv1.7virus. Keepquarantined/no bypass; requested AVproduct/exactname/path asynchronously, not yet supplied. ExactpublicZIP freshlydownloaded matchesall17runtimehashes. NewDefenderUpdate-MpSignature succeeded, signatures1.463.220.0 updatedNov9, archive/extracted scans0, unsignedEXE. CI42761632741; reports docs/security/v1.7/alert-2026-11-10/. ClamAVavailableNov8definitions0detections; officialupdateHTTP403, notlatest. No false-positive conclusion, no executable run or replacementrelease.

User then explicitly authorized implementing audit hypotheses, expandingQtable, and NEW1000traininground budget. Proceed with source/headless-only work; frozenLevel0weights unchanged, win-onlyreward; finer timestep and richer context/search choices. Do not conflate antivirus investigation with newcandidate quality or automatically publish another flagged Windows package.

## ExpandedV3 hypothesis implementation and NEW1000 budget — complete

User explicitly authorized implementing timing/context/search hypotheses, expandingQtable and another1000rounds afterv1.7virusreport. Implemented native/contextual_policy.hpp STRIKE_Q_V3, capacity49152states×12actions (192×6binaryfeatures×4regions), sparse localrows/sharedV2parent androutepriors, parentstrength2. Featuresownaim/fire-ready/nearcover/finerange/visibleenemyfacing+weapon/ownregion; newworld-only reachablecommitted east/west/centralroutes, interruptedbyreal evidence. No hiddenfeatureleak. Gamecontextinference usedidenticallyforlearning/eval, actualexecutedcontext/action logged atdecisionpoint.

Newcontexttrainer uses120HzGame::step, bothactorcontrollerdt,250msdecisions, correctedmetricstepSeconds (notold.05 multiplier). Only1win/0otherwise, firstvisitcontext/parentupdates,1000continuouscampaignrounds10×100,globalEPS.25→.02,alpha.12/.02parent first500→.08/.01last500. V2 vsinheritedV3 all400corebaselinefieldsidentical201wins(50.25%). Source-as-trainedsnapshot/hashes matchlockedplan; sourceformatting/parserguards addedafterrun withoutvalidprotocolchanges.

Validationmax234/400atcheckpoint400, selectedbeforefresh; final1000 222/400. Fresh1000 V2baseline491W/503L/6mutual,selectedV3574W/367L/56mutual/3timeouts,finalV3574W/353L/70mutual/3timeouts. Win49.1→57.4(+8.3pp); gained218/lost135,p5.84e-6,paired95%delta+4.65..+11.95pp. Damage61.55→70.075,taken54.65→46.8, win-time13.54→20.82s(slower);shared356wins14.19→19.23s. Selected297sparserows,final445. Independentagent verifiedreward,frozenweights,source,lineage,selectionandfreshstats. All1000authorizedcampaignroundsdone; no furtherlearning.

Sourcecandidatev1.8 window/CMake, loads native/assets/bot-policy-context-v3.txt (selected400); F5loadsunchangedv1.7V2 inbot-policy-trained.txt. Olderassets/releasespreserved. Source09e1618passed51nativeWindows/Linuxcasesand3headlesstests,CI42765187660/42765187650. Extra boundary/context tests locallypass; finalCI follows nextcommit. Fullarchive training/runs/2026-11-10-expanded-q1000/REPORT.md withrawperround/50round/validation/configs/snapshots/models.

NO v1.8WindowsreleaseZIPornewtagpublished whilev1.7AVproduct/exactdetectiondetailsremainrequestedandunresolved. UpdatedDefenderscan oforiginalv1.7negativeisnotproof offalsepositive. No flaggedEXEexecuted; source/headlessworkseparate. Draftdocs/releases/v1.8-draft.md,officiallastreleasedv1.7 remainsunchanged. Nextactiononnewinstruction: resolvealertdetailsbeforepackaging/release; no silentextra training.

## v1.8 export requested and completed

User explicitly requested investigating safety and exporting v1.8 now, superseding the earlier packaging hold. Fresh Windows MSVC build from c111371 passed gameplay tests and Defender (updated signatures1.463.235.0), run42960034600. Independently refreshed ClamAV daily28179/main63/bytecode339 scanned runtime folder and finalZIP: zero infected/exit0. Exact hashes/logs under docs/security/v1.8. All source assets verified against package, CRC passed. No additional learning; selectedV3checkpoint400 remains default. New versioned ZIP/checksums, tag and releasev1.8 preserve older releases. Unsigned EXE; no graphical Windows playtest. Originalv1.7alert still unresolved without product/detection/path; no claim of verified fix/falsepositive or guaranteed safety.

Release attachment uploads returned HTTP401 from uploads.github.com despite successful Git push/tag/release API. v1.8 release page links directly to the ZIP/checksums already committed at immutable v1.8 tag. No tag moved or older asset replaced.

## v1.8 repeated Windows warning rechecked

User reported virus again; answered “windows” then “that is what windows says” to a question about exact “Windows protected your PC” wording. Likely SmartScreen, not conclusively established; named Protection history detection/path remains unavailable. Rechecked exact tagged public package and19runtimefilehashes/CRC. New WindowsDefender run42960633934: signatureupdate succeeded,1.463.235.0,ZIP/extractedexit0,NotSigned,applicationExecuted=false. ClamAV23files/zeroinfected. Bounded source/security review found no firstparty shell/download/persistence/injection calls. Evidence docs/security/v1.8/user-alert-recheck/. Do not claim guarantee or confirmedfalsepositive. No rebuild; unsigned publisher reputation may require realcode-signingcertificate, not bytechanges/bypass.
