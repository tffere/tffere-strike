# Tffere Strike — Depot

An original single-player tactical FPS prototype inspired by early-2000s PC shooters. Explore one compact industrial courtyard, use crates and shipping containers as cover, and eliminate six patrol units before the three-minute round expires. All map geometry, interface, procedural textures, sounds, and weapon models are original. It does not use Counter-Strike assets or networking.

## Run locally

Install Node.js 20 or newer. In this folder:

```sh
npm ci
npm start
```

Open `http://localhost:4174` in a desktop browser with WebGL enabled, then click **Deploy into Depot**. The page captures the mouse for first-person looking. The server uses port 4174 by default; set `PORT` to choose another port. Keep the server running while playing. On restricted systems where npm cannot write its default cache, use `npm ci --cache ./node_modules/.npm-cache`.

## Controls

| Input             | Action                          |
| ----------------- | ------------------------------- |
| W / A / S / D     | Move                            |
| Mouse             | Look                            |
| Left mouse / hold | Fire                            |
| R                 | Reload current weapon           |
| 1 / 2             | Select AR-7 rifle / AW-9 sniper |
| Right mouse       | Toggle AW-9 scope               |
| Space             | Jump                            |
| Left Shift        | Sprint                          |
| Escape            | Pause and release mouse         |
| Enter             | Start / resume / new round      |

The AR-7 has a 30-round magazine and 90 reserve rounds. Body hits deal 36 damage and head hits deal 75. The AW-9 bolt-action sniper has a five-round magazine and 20 reserve rounds, a 1.35-second shot interval, and a 2.5-second reload. Its body hits deal 125 damage and head hits 220; each patrol unit has 100 health. Right-click with the sniper selected to smoothly zoom through the scope; scoped mouse sensitivity is reduced. Switching back to the rifle exits the scope. Each weapon keeps its own ammo and reload state. Both weapons have original synthesized firing, handling, and reload sounds. The AW-9 is an original low-poly model with a long barrel, stock, magazine, rail, scope lenses and adjustment knobs, and a bolt handle. Enemies patrol or approach, see and shoot only with an unobstructed line of sight, and cannot shoot through solid cover. The radar shows the arena layout, player heading, and remaining enemies. Death, a cleared courtyard, or the time limit ends the round. Deploy again to reset the map.

## Checks

```sh
npm test
```

This runs collision, tunneling, wall sliding, rifle/sniper fire rates, separate ammo/reload behavior, and reset regressions. Browser smoke checks are in `tests/browser-smoke.cjs`; Playwright is pinned as a development dependency. Use installed system Chromium, or run `npx playwright install chromium` once. Set `CHROMIUM_PATH` to override the browser executable. With the server already running:

```sh
node tests/browser-smoke.cjs
```

The browser test opens explicit `?test=1` mode, which exposes test state/actions. That interface is absent during normal play. Screenshots are written to ignored `artifacts/`.

## Prototype scope

This is a small playable prototype, not a full Counter-Strike replacement. It has two selectable weapons, six simple bots, one map, collision, jumping, cover, pause, and rounds. There is no multiplayer, matchmaking, team economy, bomb objective, animation rig, or advanced pathfinding. Bots can occasionally linger behind cover; use the radar and change lanes to find them. Desktop keyboard and mouse are required.

Three.js is installed locally through npm and pinned in the lockfile. After installation, the game makes no external asset requests. Stop the development server with Ctrl+C. This server is intended for local development and is not a hardened public hosting service.

## If Deploy does nothing

Open `http://localhost:4174`, not `index.html` directly. Wait for the button to change from **Loading 3D game** to **Deploy into Depot**. Startup failures now appear in the menu. Run `npm ci` successfully before `npm start`; if a graphics error appears, enable hardware/graphics acceleration in Chrome or Edge and restart the browser. Use F12 → Console to see the specific error.
