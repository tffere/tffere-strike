# Tffere Strike — Native C++ v1.2

A standalone Windows desktop FPS written in C++17 with raylib. Play against six bots in a connected desert quarter, choose an AR-7 rifle or AW-9 sniper, use cover, and clear the depot before the round ends.

## Download and play on Windows

1. [Download TffereStrike-Windows-x64.zip](https://github.com/tffere/tffere-strike/raw/refs/heads/main/downloads/TffereStrike-v1.2-Windows-x64-rebuilt.zip).
2. Right-click the ZIP and select **Extract All**. Open the extracted `TffereStrike` folder.
3. Double-click **TffereStrike.exe**. Keep its `assets` folder beside it.
4. Click **Deploy into Depot**, or press **Enter**.

No Node.js, npm, Python, browser, or development tools are needed to play. Requires Windows 10/11 x64 with an OpenGL 3.3-capable graphics driver. Extract the whole ZIP; do not run the executable from inside the archive. The executable is not digitally signed.

## November 8 package recheck

The current v1.2 ZIP was freshly built using Visual Studio on GitHub's Windows runner. Microsoft Defender and ClamAV reported no detections in the rebuilt game. Both scan reports and per-file hashes are bundled under `SCAN-REPORTS/`; see [recheck details and limits](docs/security/RECHECK.md). These results do not guarantee safety or explain the original reported warning. The executable is unsigned. If antivirus detects it, keep it quarantined and report the detection name.

## v1.2 bot update

Bots now run on approach, turn gradually, perform staggered jumps and crouch during combat. Longer running strides, airborne knee bends and lowered crouching poses follow gameplay state. Eye height, muzzle height and bullet hitboxes follow jumping and crouching.

Bots acquire shots within a 120-degree horizontal field of view and fire only after aiming within roughly 10 degrees of the player. Every shot rechecks visibility after movement: the eye-to-player, chest-to-muzzle and muzzle-to-player paths must all be clear of collision objects. Bots still navigate toward the player when hidden; this is not a stealth-awareness simulation. Jumping stays on navigable ground and does not vault obstacles.

## Keep previous versions

- [v1.0: original depot](https://github.com/tffere/tffere-strike/raw/refs/heads/main/downloads/TffereStrike-v1.0-Windows-x64.zip)
- [v1.1: Sandstone desert map and revised characters](https://github.com/tffere/tffere-strike/raw/refs/heads/main/downloads/TffereStrike-v1.1-Windows-x64.zip)

- [v1.2: running, jumping, crouching and stricter bot visibility](https://github.com/tffere/tffere-strike/raw/refs/heads/main/downloads/TffereStrike-v1.2-Windows-x64-rebuilt.zip)

Extract each version into a separate folder. The `v1.0` and `v1.1` Git tags preserve their source; `upgrade-desert-map` records the upgrade work. Neither download requires installing the other.

## Controls

| Input | Action |
| --- | --- |
| W / A / S / D | Move |
| Mouse | Look |
| Left mouse / hold | Fire |
| 1 / 2 | AR-7 rifle / AW-9 sniper |
| Right mouse | Toggle sniper scope |
| R | Reload; restart when paused |
| Space | Jump |
| Left Shift | Sprint |
| Escape | Pause/resume; close from the start/end menu |
| Enter | Deploy / resume / new round |
| F11 | Toggle fullscreen |
| M | Mute/unmute sound |
| F3 | Show performance info |
| F4 | Toggle shadows |

## Included

- Original desert quarter with connected mid, long-road and covered-passage routes, two courts, sandstone façades, shop entrances, arches, shutters, awnings, palms and cover. Directional shadows and restrained material textures keep the routes readable.
- Revised human-proportioned patrol models with shaped faces, helmets, goggles, tactical equipment and camouflage. Distance-driven alternating strides, bent knees, breathing, smooth turning, hit reactions and animated falls replace the original rigid patrol pose.
- Chamfered rifle/sniper models with compact sights, layered rails, scope/bolt detail, fitted black gloves and shaped bare forearms, plus recoil/reload movement.
- Animated sniper zoom and reticle, separate ammunition/reload state, and distinct fire rates and damage.
- Synthesized firing, reload, hit and footstep sounds. A missing audio device does not prevent play.
- Six bots with cover-aware vision/shooting and deterministic navigation around obstacles.
- Radar, hit feedback, health, ammunition, a three-minute round, pause, restart and win/loss states.

This is a compact single-player game with stylized original models, not a recreation of Counter-Strike or the earlier generated concept artwork. There is no multiplayer, matchmaking, campaign, or advanced character animation rig. The screenshots below show the v1.1 executable and map; v1.2 changes bot behavior and animation.

![Actual native gameplay](docs/native-gameplay.png)

![Actual long-road view](docs/native-long-road.png)

![Actual map overview](docs/native-map-overview.png)

## Source and builds

The native version is in [`native/`](native/). The earlier JavaScript prototype remains in the repository for reference; its instructions are in [BROWSER-PROTOTYPE.md](docs/BROWSER-PROTOTYPE.md). Use the Windows ZIP above for the current game.

CMake 3.24+, a C++17 compiler and Internet access to fetch checksum-pinned raylib 5.5 are needed to build from source. On Windows, install Visual Studio Build Tools with **Desktop development with C++**, then:

```sh
cmake -S native -B native-build -A x64
cmake --build native-build --config Release --parallel 2
ctest --test-dir native-build -C Release --output-on-failure
```

Run `native-build/Release/TffereStrike.exe`; assets are copied automatically. `.github/workflows/native.yml` defines the equivalent Windows CI build and artifact.

Linux builds require GCC/G++, CMake and standard X11/OpenGL development packages:

```sh
cmake -S native -B native-build
cmake --build native-build --parallel 2
ctest --test-dir native-build --output-on-failure
```

`native/build.py` is an alternative direct builder. Windows cross-compilation uses Zig 0.13.0:

```sh
python3 native/build.py --platform windows --zig /path/to/zig --package
```

## Validation

For v1.2, all 25 native gameplay tests pass on Linux, including field-of-view boundaries, gradual turning, muzzle obstruction despite visible eyes, running/jumping/landing, crouch transitions and stance-aware hitboxes. The eight earlier browser prototype tests also pass. The replacement Windows x64 package is built with Visual Studio on a fresh GitHub Windows runner; all 25 native gameplay cases also pass there.

A graphical Windows playtest of v1.2 has not been performed in this environment. The earlier v1.1 graphical smoke tests do not validate the new animation. Please try v1.2 on your Windows machine and report movement or rendering issues.

Launch with `--smoke-test` for a short graphical check and screenshot, or `--capture` for an initial gameplay view. Normal launches start at the menu.

## Assets and licenses

Game geometry and sounds are generated by the C++ application. The material atlas is original generated artwork bundled locally. DejaVu font license is in `native/assets/FONT-LICENSE.txt`; the raylib license is included in the downloadable package. Downloads and package hashes are in [`downloads/`](downloads/).
