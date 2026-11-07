import { test } from "node:test";
import assert from "node:assert/strict";
import { circleBlocked, moveActor, Rifle, Sniper } from "../core.js";
const obstacle = [{ x: 0, z: 0, w: 2, d: 2 }];
test("circle collision distinguishes wall, corner, free area and map edge", () => {
  assert.equal(circleBlocked(0, 0, 0.35, obstacle), true);
  assert.equal(circleBlocked(1.2, 1.2, 0.35, obstacle), true);
  assert.equal(circleBlocked(1.4, 1.4, 0.35, obstacle), false);
  assert.equal(circleBlocked(21.3, 0, 0.35, []), true);
});
test("swept movement cannot tunnel through a thin wall", () => {
  const a = { x: -5, z: 0 };
  moveActor(a, 10, 0, obstacle);
  assert.ok(a.x <= -1.35);
  assert.ok(a.x > -1.6);
});
test("movement slides along a wall without entering it", () => {
  const a = { x: -1.4, z: 0 };
  moveActor(a, 1, 1, obstacle);
  assert.equal(a.x, -1.4);
  assert.ok(a.z > 0.9);
});
test("rifle enforces fire rate and magazine capacity", () => {
  const r = new Rifle();
  assert.ok(r.fire());
  assert.equal(r.ammo, 29);
  assert.equal(r.fire(), false);
  for (let i = 0; i < 29; i++) {
    r.update(0.2);
    assert.ok(r.fire());
  }
  r.update(0.2);
  assert.equal(r.fire(), false);
  assert.equal(r.ammo, 0);
});
test("reload consumes finite reserve and blocks firing until completed", () => {
  const r = new Rifle();
  r.ammo = 12;
  assert.ok(r.reload());
  assert.equal(r.fire(), false);
  r.update(1);
  assert.equal(r.ammo, 12);
  r.update(1);
  assert.equal(r.ammo, 30);
  assert.equal(r.reserve, 72);
  r.ammo = 0;
  r.reserve = 5;
  r.reload();
  r.update(2);
  assert.equal(r.ammo, 5);
  assert.equal(r.reserve, 0);
});
test("reset restores a fresh loadout", () => {
  const r = new Rifle();
  r.ammo = 0;
  r.reserve = 0;
  r.reset();
  assert.equal(r.ammo, 30);
  assert.equal(r.reserve, 90);
  assert.equal(r.reloadTime, 0);
});

test("sniper has a five-round magazine and slow bolt-action cadence", () => {
  const gun = new Sniper();
  assert.equal(gun.ammo, 5);
  assert.equal(gun.reserve, 20);
  assert.ok(gun.fire());
  gun.update(0.5);
  assert.equal(gun.fire(), false);
  gun.update(0.9);
  assert.ok(gun.fire());
  assert.equal(gun.ammo, 3);
});
test("sniper reload duration and reserve are separate from the rifle", () => {
  const gun = new Sniper(),
    rifle = new Rifle();
  gun.fire();
  gun.reload();
  gun.update(2);
  assert.equal(gun.ammo, 4);
  gun.update(0.6);
  assert.equal(gun.ammo, 5);
  assert.equal(gun.reserve, 19);
  assert.equal(rifle.ammo, 30);
  assert.equal(rifle.reserve, 90);
});
