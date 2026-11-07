const assert = require("node:assert/strict");
const fs = require("node:fs");
const { chromium } = require("playwright");
(async () => {
  fs.mkdirSync("artifacts", { recursive: true });
  const browser = await chromium.launch({
    executablePath:
      process.env.CHROMIUM_PATH ||
      (fs.existsSync("/usr/bin/chromium") ? "/usr/bin/chromium" : undefined),
    headless: true,
    args: [
      "--no-sandbox",
      "--enable-unsafe-swiftshader",
      "--use-gl=angle",
      "--use-angle=swiftshader",
    ],
  });
  global._browser = browser;
  const page = await browser.newPage({
    viewport: { width: 1280, height: 720 },
  });
  const errors = [];
  page.on("pageerror", (error) => errors.push(error.message));
  await page.goto("http://127.0.0.1:4174/?test=1");
  await page.waitForFunction(
    () => window.__strike && document.querySelector("#game canvas"),
  );
  await page.screenshot({ path: "artifacts/start-screen.png" });
  await page.click("#start");
  await page.waitForFunction(() => window.__strike.state.mode === "playing");
  const before = await page.evaluate(() => window.__strike.snapshot());
  await page.keyboard.down("KeyW");
  await page.waitForFunction(
    (z) => window.__strike.state.z < z - 0.3,
    before.z,
    { timeout: 10000 },
  );
  await page.keyboard.up("KeyW");
  const moved = await page.evaluate(() => window.__strike.snapshot());
  assert.ok(moved.z < before.z - 0.2, "W moves forward");
  await page.keyboard.press("Space");
  await page.waitForFunction(() => window.__strike.state.y > 1.7, {
    timeout: 10000,
  });
  // A real raycast against a visible enemy must remove health and count a kill.
  const combat = await page.evaluate(() => {
    const g = window.__strike;
    g.reset();
    g.state.mode = "playing";
    g.aimAt(0);
    for (let i = 0; i < 3; i++) {
      g.rifle.cooldown = 0;
      g.shoot();
    }
    return { hp: g.bots[0].hp, kills: g.state.kills, ammo: g.rifle.ammo };
  });
  assert.ok(combat.hp <= 0, "three body hits eliminate an enemy");
  assert.equal(combat.kills, 1);
  assert.equal(combat.ammo, 27);
  await page.keyboard.press("KeyR");
  assert.ok(await page.evaluate(() => window.__strike.rifle.reloadTime > 0));
  await page.evaluate(() => window.__strike.rifle.update(2));
  assert.equal(await page.evaluate(() => window.__strike.rifle.ammo), 30);
  const cover = await page.evaluate(() => {
    const g = window.__strike;
    g.reset();
    g.state.mode = "playing";
    g.state.x = 13;
    g.state.z = 18;
    const b = g.bots[0];
    b.x = 13;
    b.z = -5;
    b.group.position.set(13, 0, -5);
    g.aimAt(0);
    g.shoot();
    return b.hp;
  });
  assert.equal(cover, 100, "container cover stops player bullets");
  // Actual bot AI should damage the player when it has line of sight.
  await page.evaluate(() => {
    const g = window.__strike;
    g.reset();
    g.state.mode = "playing";
    g.bots[0].cooldown = 0;
  });
  await page.waitForFunction(() => window.__strike.state.health < 100, {
    timeout: 15000,
  });
  await page.keyboard.press("Escape");
  await page.waitForFunction(() => window.__strike.state.mode === "paused");
  const paused = await page.evaluate(() => window.__strike.snapshot());
  await page.waitForTimeout(250);
  const afterPause = await page.evaluate(() => window.__strike.snapshot());
  assert.equal(afterPause.time, paused.time, "pause freezes round");
  assert.equal(afterPause.health, paused.health);
  await page.click("#start");
  await page.waitForFunction(() => window.__strike.state.mode === "playing");
  await page.evaluate(() => window.__strike.finish(true, "Smoke test victory"));
  assert.ok(
    (await page.locator("#overlay-title").textContent()) === "DEPOT SECURED",
  );
  await page.click("#start");
  assert.equal(await page.evaluate(() => window.__strike.state.kills), 0);
  await page.evaluate(() => {
    const g = window.__strike;
    g.reset();
    g.state.mode = "playing";
    g.state.health = 1;
    g.bots[0].cooldown = 0;
  });
  await page.waitForFunction(() => window.__strike.state.mode === "lost", {
    timeout: 15000,
  });
  assert.equal(
    await page.locator("#overlay-title").textContent(),
    "MISSION FAILED",
  );
  await page.click("#start");
  await page.waitForFunction(() => window.__strike.state.mode === "playing");
  await page.evaluate(() => {
    const g = window.__strike;
    g.state.pitch = -0.01;
    g.state.yaw = 0;
  });
  await page.waitForTimeout(250);
  await page.screenshot({ path: "artifacts/first-person.png" });
  // Exercise original AW-9 model, weapon selection, real zoom and scope overlay.
  await page.keyboard.press("Digit2");
  await page.waitForFunction(() => window.__strike.activeWeapon === "sniper");
  await page.waitForFunction(
    () =>
      document.getElementById("weapon-label").textContent ===
      "AW-9 / BOLT-ACTION SNIPER",
  );
  await page.screenshot({ path: "artifacts/sniper-first-person.png" });
  await page.mouse.click(640, 360, { button: "right" });
  await page.waitForFunction(
    () => window.__strike.scoped && window.__strike.camera.fov < 23,
    null,
    { timeout: 15000 },
  );
  assert.ok(
    Number(await page.locator("#scope").evaluate((el) => el.style.opacity)) >
      0.9,
  );
  await page.screenshot({ path: "artifacts/sniper-scope.png" });
  const sniperHit = await page.evaluate(() => {
    const g = window.__strike;
    g.aimAt(0);
    g.sniper.cooldown = 0;
    const first = g.shoot(),
      second = g.shoot();
    return {
      first,
      second,
      ammo: g.sniper.ammo,
      hp: g.bots[0].hp,
      kills: g.state.kills,
    };
  });
  assert.ok(sniperHit.first);
  assert.equal(sniperHit.second, false);
  assert.equal(sniperHit.ammo, 4);
  assert.ok(sniperHit.hp <= 0);
  assert.equal(sniperHit.kills, 1);
  await page.keyboard.press("KeyR");
  assert.ok(await page.evaluate(() => window.__strike.sniper.reloadTime > 0));
  await page.evaluate(() => window.__strike.sniper.update(3));
  assert.equal(await page.evaluate(() => window.__strike.sniper.ammo), 5);
  await page.keyboard.press("Digit1");
  await page.waitForFunction(
    () => !window.__strike.scoped && window.__strike.camera.fov > 70,
    null,
    { timeout: 15000 },
  );
  assert.equal(
    await page.evaluate(() => window.__strike.activeWeapon),
    "rifle",
  );
  assert.deepEqual(errors, [], "no browser exceptions");
  console.log(
    "PASS: WebGL render, keyboard movement/jump, raycast enemy hit/elimination, reload, cover occlusion, bot damage, pause/resume, victory/loss, restart, sniper selection/model, scope zoom animation, high-damage shot/cadence/reload, switching out of scope; no browser exceptions.",
  );
  await browser.close();
})().catch(async (error) => {
  console.error(error);
  if (global._browser) await global._browser.close();
  process.exitCode = 1;
});
