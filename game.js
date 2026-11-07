import * as THREE from "./node_modules/three/build/three.module.js";
import { Rifle, Sniper, moveActor, circleBlocked } from "./core.js";

const $ = (id) => document.getElementById(id);
const scene = new THREE.Scene();
scene.background = new THREE.Color("#aebbc4");
scene.fog = new THREE.Fog("#aebbc4", 32, 100);
const camera = new THREE.PerspectiveCamera(
  76,
  innerWidth / innerHeight,
  0.05,
  160,
);
camera.rotation.order = "YXZ";
scene.add(camera);
let renderer;
try {
  renderer = new THREE.WebGLRenderer({
    antialias: true,
    powerPreference: "high-performance",
  });
} catch (error) {
  $("error").hidden = false;
  $("error").textContent =
    "A WebGL-capable desktop browser is required. Enable hardware acceleration and refresh.";
  throw error;
}
renderer.setPixelRatio(Math.min(devicePixelRatio, 1.5));
renderer.setSize(innerWidth, innerHeight);
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFSoftShadowMap;
renderer.outputColorSpace = THREE.SRGBColorSpace;
renderer.toneMapping = THREE.ACESFilmicToneMapping;
renderer.toneMappingExposure = 1.12;
$("game").appendChild(renderer.domElement);
scene.add(new THREE.HemisphereLight("#d9e9ff", "#6c5c45", 2));
const sun = new THREE.DirectionalLight("#ffe4b7", 2.8);
sun.position.set(-22, 35, 18);
sun.castShadow = true;
sun.shadow.camera.left = -30;
sun.shadow.camera.right = 30;
sun.shadow.camera.top = 30;
sun.shadow.camera.bottom = -30;
sun.shadow.mapSize.set(1024, 1024);
sun.shadow.bias = -0.0005;
scene.add(sun);

let seed = 711;
const random = () => {
  seed = (seed * 1664525 + 1013904223) >>> 0;
  return seed / 4294967296;
};
function texture(base, planks = false) {
  const c = document.createElement("canvas");
  c.width = c.height = 128;
  const x = c.getContext("2d");
  x.fillStyle = base;
  x.fillRect(0, 0, 128, 128);
  for (let i = 0; i < 1600; i++) {
    x.fillStyle = `rgba(${random() > 0.5 ? "255,255,255" : "0,0,0"},${random() * 0.12})`;
    x.fillRect(
      random() * 128,
      random() * 128,
      random() * 3 + 1,
      random() * 3 + 1,
    );
  }
  if (planks) {
    x.strokeStyle = "#3e312a";
    for (let i = 0; i < 128; i += 26) {
      x.beginPath();
      x.moveTo(i, 0);
      x.lineTo(i, 128);
      x.stroke();
    }
  }
  const t = new THREE.CanvasTexture(c);
  t.colorSpace = THREE.SRGBColorSpace;
  t.wrapS = t.wrapT = THREE.RepeatWrapping;
  t.anisotropy = 4;
  return t;
}
const groundTex = texture("#847c6c");
groundTex.repeat.set(30, 30);
const plasterTex = texture("#b8a18a");
plasterTex.repeat.set(3, 1);
const crateTex = texture("#8e6949", true);
const mat = {
  wall: new THREE.MeshStandardMaterial({ map: plasterTex, roughness: 0.95 }),
  trim: new THREE.MeshStandardMaterial({ color: "#56636a", roughness: 0.85 }),
  metal: new THREE.MeshStandardMaterial({ color: "#78868a", roughness: 0.6 }),
  crate: new THREE.MeshStandardMaterial({ map: crateTex, roughness: 0.95 }),
  orange: new THREE.MeshStandardMaterial({ color: "#ad6039", roughness: 0.85 }),
  teal: new THREE.MeshStandardMaterial({ color: "#487271", roughness: 0.85 }),
  dark: new THREE.MeshStandardMaterial({ color: "#283136", roughness: 0.7 }),
  yellow: new THREE.MeshStandardMaterial({ color: "#c6a34c", roughness: 0.8 }),
};
const colliders = [],
  solidMeshes = [];
function box(x, y, z, w, h, d, material, solid = false) {
  const mesh = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), material);
  mesh.position.set(x, y, z);
  mesh.castShadow = true;
  mesh.receiveShadow = true;
  scene.add(mesh);
  if (solid) {
    colliders.push({ x, z, w, d });
    solidMeshes.push(mesh);
  }
  return mesh;
}
function localBox(group, x, y, z, w, h, d, material) {
  const m = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), material);
  m.position.set(x, y, z);
  m.castShadow = true;
  group.add(m);
  return m;
}
function cylinder(x, y, z, radius, height, material) {
  const m = new THREE.Mesh(
    new THREE.CylinderGeometry(radius, radius, height, 12),
    material,
  );
  m.position.set(x, y, z);
  m.castShadow = true;
  scene.add(m);
  return m;
}
const floor = new THREE.Mesh(
  new THREE.PlaneGeometry(230, 230),
  new THREE.MeshStandardMaterial({ map: groundTex, roughness: 1 }),
);
floor.rotation.x = -Math.PI / 2;
floor.receiveShadow = true;
scene.add(floor);
// Original, compact depot layout: three lanes with crates and containers.
box(0, 3, -23, 48, 6, 2, mat.wall, true);
box(-23, 3, 0, 2, 6, 48, mat.wall, true);
box(23, 3, 0, 2, 6, 48, mat.wall, true);
box(0, 3, 23, 48, 6, 2, mat.wall, true);
for (const [x, z, w, d] of [
  [-16, -10, 12, 14],
  [17, -15, 10, 12],
]) {
  box(x, 4, z, w, 8, d, mat.wall, true);
  box(x, 8.1, z, w + 0.5, 0.3, d + 0.5, mat.trim);
  for (let a = -1; a <= 1; a++) {
    box(x + a * 2.6, 5.8, z + d / 2 + 0.04, 1.8, 1.4, 0.08, mat.trim);
    box(
      x + a * 2.6,
      5.8,
      z + d / 2 + 0.1,
      1.5,
      1.1,
      0.06,
      new THREE.MeshStandardMaterial({ color: "#4b626b", roughness: 0.4 }),
    );
  }
  box(x, 8.8, z, 2.2, 1.2, 1.7, mat.metal);
  box(x + 3, 9.6, z - 2, 0.1, 3.5, 0.1, mat.dark);
}
function container(x, z, color) {
  box(x, 1.6, z, 4.4, 3.2, 8, color, true);
  for (let i = 0; i < 16; i++) {
    const zz = z - 3.7 + i * 0.5;
    box(x - 2.23, 1.6, zz, 0.07, 3, 0.055, mat.trim);
    box(x + 2.23, 1.6, zz, 0.07, 3, 0.055, mat.trim);
  }
  for (const xx of [x - 1.5, x + 1.5]) {
    box(xx, 1.6, z + 4.04, 0.06, 2.8, 0.08, mat.metal);
  }
  box(x, 3.25, z, 4.5, 0.1, 8.1, mat.trim);
}
container(-11, 7, mat.orange);
container(13, 2, mat.teal);
function crate(x, z, size = 1.8, y = 0) {
  const body = box(x, y + size / 2, z, size, size, size, mat.crate, y === 0);
  for (const zz of [z - size / 2 - 0.03, z + size / 2 + 0.03]) {
    box(x, y + 0.16, zz, size, 0.16, 0.1, mat.crate);
    box(x, y + size - 0.16, zz, size, 0.16, 0.1, mat.crate);
    const beam = box(x, y + size / 2, zz, 0.14, size * 1.25, 0.1, mat.crate);
    beam.rotation.z = 0.7;
  }
  return body;
}
crate(-4, 8, 2.2);
crate(-5.9, 6.6, 1.5);
crate(5, -2, 2);
crate(7.1, -3, 1.7);
crate(-4, -12, 2.2);
crate(-4, -12, 1.6, 2.2);
crate(17, 14, 2.2);
crate(18, -6, 1.8);
// Rolling shutters, overhead pipes and painted wayfinding.
for (const [x, z, w] of [
  [-16, -2.94, 5],
  [17, -8.94, 4],
]) {
  box(x, 1.9, z, w, 3.8, 0.12, mat.trim);
  for (let j = 0; j < 12; j++)
    box(x, 0.2 + j * 0.3, z + 0.09, w, 0.035, 0.03, mat.metal);
  box(x, 4.2, z + 0.1, w + 0.5, 0.25, 0.5, mat.trim);
}
for (let z = -17; z < 20; z += 4) {
  box(-20.6, 0.015, z, 0.14, 0.025, 1.3, mat.yellow);
  box(20.6, 0.015, z, 0.14, 0.025, 1.3, mat.yellow);
}
const pipe = cylinder(-20, 5, -9, 0.16, 10, mat.metal);
pipe.rotation.z = Math.PI / 2;
function sign(text, x, y, z, ry = 0) {
  const canvas = document.createElement("canvas");
  canvas.width = 512;
  canvas.height = 192;
  const ctx = canvas.getContext("2d");
  ctx.fillStyle = "#b9a58c";
  ctx.fillRect(0, 0, 512, 192);
  ctx.fillStyle = "#47545b";
  ctx.font = "bold 56px Arial";
  ctx.fillText(text, 24, 80);
  ctx.fillStyle = "#a15b37";
  ctx.font = "30px Arial";
  ctx.fillText("SECTOR 07 // AUTHORIZED ONLY", 24, 133);
  const tex = new THREE.CanvasTexture(canvas);
  tex.colorSpace = THREE.SRGBColorSpace;
  const m = new THREE.Mesh(
    new THREE.PlaneGeometry(6, 2.25),
    new THREE.MeshStandardMaterial({ map: tex, roughness: 1 }),
  );
  m.position.set(x, y, z);
  m.rotation.y = ry;
  scene.add(m);
}
sign("DEPOT / A", -16, 5, -2.85);
sign("LOADING BAY", 5, 3.8, -21.94);
// Distant silhouettes give the small arena a surrounding industrial skyline.
for (let i = 0; i < 14; i++)
  box(-65 + i * 10, 5 + random() * 5, -42, 7, 10 + random() * 10, 8, mat.trim);

const rifle = new Rifle(),
  sniper = new Sniper();
let activeWeapon = "rifle",
  scoped = false,
  scopeBlend = 0;
const currentGun = () => (activeWeapon === "rifle" ? rifle : sniper);
const weapon = new THREE.Group();
camera.add(weapon);
weapon.position.set(0.31, -0.29, -0.61);
const gunMat = new THREE.MeshStandardMaterial({
  color: "#30373b",
  roughness: 0.65,
});
const stockMat = new THREE.MeshStandardMaterial({
  color: "#625045",
  roughness: 0.9,
});
localBox(weapon, 0, 0, 0, 0.14, 0.15, 0.47, gunMat);
localBox(weapon, 0, 0.09, -0.02, 0.08, 0.04, 0.36, mat.dark);
localBox(weapon, 0, -0.04, -0.29, 0.12, 0.11, 0.26, stockMat);
localBox(weapon, 0, -0.12, 0.04, 0.09, 0.17, 0.12, gunMat).rotation.x = -0.15;
localBox(weapon, 0, -0.16, -0.07, 0.09, 0.25, 0.14, mat.dark).rotation.x = 0.17;
localBox(weapon, 0, 0.03, -0.54, 0.04, 0.04, 0.33, gunMat);
localBox(weapon, 0, 0.1, -0.52, 0.015, 0.09, 0.025, mat.dark);
localBox(weapon, 0, 0.11, 0.08, 0.09, 0.035, 0.025, mat.dark);
localBox(weapon, -0.13, -0.15, -0.22, 0.13, 0.14, 0.2, mat.dark).rotation.z =
  -0.3;
localBox(weapon, 0.01, -0.2, 0.13, 0.14, 0.13, 0.23, mat.dark);
localBox(
  weapon,
  0.06,
  -0.29,
  0.2,
  0.17,
  0.17,
  0.27,
  new THREE.MeshStandardMaterial({ color: "#52604b" }),
);
const flash = new THREE.Mesh(
  new THREE.ConeGeometry(0.07, 0.22, 5),
  new THREE.MeshBasicMaterial({
    color: "#ffc270",
    transparent: true,
    opacity: 0.95,
  }),
);
flash.rotation.x = -Math.PI / 2;
flash.position.set(0, 0.03, -0.78);
flash.visible = false;
weapon.add(flash);
const flashLight = new THREE.PointLight("#ffb05f", 0, 3);
flashLight.position.set(0, 0.1, -0.7);
weapon.add(flashLight);

// Original AW-9: long barrel, stock, magazine, rail, scope and bolt details.
const sniperModel = new THREE.Group();
camera.add(sniperModel);
sniperModel.scale.set(0.85, 0.85, 0.85);
sniperModel.position.set(0.34, -0.32, -0.72);
const olive = new THREE.MeshStandardMaterial({
  color: "#637058",
  roughness: 0.8,
});
localBox(sniperModel, 0, -0.035, 0.015, 0.15, 0.18, 0.68, olive);
localBox(sniperModel, 0, 0.075, -0.04, 0.11, 0.095, 0.48, gunMat);
localBox(sniperModel, 0, -0.065, 0.39, 0.12, 0.19, 0.27, olive);
localBox(sniperModel, 0, -0.05, 0.535, 0.15, 0.24, 0.035, mat.dark);
localBox(sniperModel, 0, -0.17, 0.14, 0.08, 0.2, 0.12, olive).rotation.x = -0.2;
localBox(sniperModel, 0, -0.17, -0.055, 0.1, 0.16, 0.16, gunMat);
localBox(sniperModel, 0, 0.005, -0.48, 0.045, 0.045, 0.65, gunMat);
localBox(sniperModel, 0, 0.005, -0.835, 0.065, 0.065, 0.07, mat.dark);
for (let i = 0; i < 8; i++)
  localBox(sniperModel, 0, 0.137, -0.2 + i * 0.045, 0.1, 0.02, 0.013, mat.dark);
for (const z of [-0.2, 0.1])
  localBox(sniperModel, 0, 0.19, z, 0.08, 0.12, 0.06, gunMat);
function tube(group, x, y, z, r, len, material) {
  const m = new THREE.Mesh(new THREE.CylinderGeometry(r, r, len, 20), material);
  m.rotation.x = Math.PI / 2;
  m.position.set(x, y, z);
  group.add(m);
  return m;
}
tube(sniperModel, 0, 0.265, -0.06, 0.066, 0.48, mat.dark);
tube(sniperModel, 0, 0.265, -0.34, 0.094, 0.12, gunMat);
tube(sniperModel, 0, 0.265, 0.22, 0.08, 0.09, gunMat);
const lens = new THREE.MeshStandardMaterial({
  color: "#306c77",
  metalness: 0.6,
  roughness: 0.13,
  emissive: "#102a35",
  emissiveIntensity: 0.4,
});
tube(sniperModel, 0, 0.265, -0.405, 0.084, 0.008, lens);
tube(sniperModel, 0, 0.265, 0.269, 0.069, 0.008, lens);
localBox(sniperModel, 0, 0.35, -0.04, 0.08, 0.055, 0.08, gunMat);
localBox(sniperModel, 0.08, 0.265, -0.04, 0.055, 0.08, 0.08, gunMat);
localBox(
  sniperModel,
  0.105,
  0.065,
  0.15,
  0.13,
  0.025,
  0.025,
  gunMat,
).rotation.z = -0.45;
const bolt = new THREE.Mesh(new THREE.SphereGeometry(0.025, 10, 8), mat.dark);
bolt.position.set(0.17, 0.035, 0.15);
sniperModel.add(bolt);
localBox(
  sniperModel,
  -0.09,
  -0.15,
  -0.26,
  0.14,
  0.14,
  0.2,
  mat.dark,
).rotation.z = -0.25;
localBox(sniperModel, 0.02, -0.2, 0.18, 0.14, 0.13, 0.22, mat.dark);
localBox(sniperModel, 0.06, -0.29, 0.26, 0.17, 0.17, 0.25, olive);
const sniperFlash = flash.clone();
sniperFlash.position.z = -0.93;
sniperFlash.scale.set(1.3, 1.5, 1.3);
sniperModel.add(sniperFlash);
for (let i = 1; i <= 5; i++) {
  const h = document.createElement("i");
  h.className = "tick";
  h.style.cssText = `left:calc(50% - 4px);top:${50 + i * 5}%;width:8px;height:1px`;
  $("scope").firstElementChild.appendChild(h);
}
function selectWeapon(name) {
  if (!["rifle", "sniper"].includes(name)) return;
  activeWeapon = name;
  scoped = false;
  firing = false;
  flashTime = 0;
  sound("switch");
}

const bots = [];
const uniform = new THREE.MeshStandardMaterial({
  color: "#314b50",
  roughness: 1,
});
const armor = new THREE.MeshStandardMaterial({
  color: "#26353a",
  roughness: 0.8,
});
const skin = new THREE.MeshStandardMaterial({ color: "#a88968", roughness: 1 });
const starts = [
  [0, 1],
  [-3, -6],
  [8, 10],
  [4, -15],
  [-16, 16],
  [17, -4],
];
function makeBot(index) {
  const g = new THREE.Group();
  scene.add(g);
  const bot = {
    group: g,
    x: starts[index][0],
    z: starts[index][1],
    hp: 100,
    phase: index,
    cooldown: 1 + index * 0.35,
    legs: [],
    start: starts[index],
  };
  const body = localBox(g, 0, 1.06, 0, 0.52, 0.67, 0.32, uniform);
  body.userData.bot = bot;
  const vest = localBox(g, 0, 1.08, 0.18, 0.45, 0.53, 0.07, armor);
  vest.userData.bot = bot;
  const head = localBox(g, 0, 1.59, 0, 0.29, 0.3, 0.28, skin);
  head.userData = { bot, head: true };
  const helmet = localBox(g, 0, 1.76, 0, 0.35, 0.13, 0.34, armor);
  helmet.userData = { bot, head: true };
  localBox(g, 0, 1.59, 0.15, 0.25, 0.08, 0.025, mat.dark);
  localBox(g, 0.285, 1.25, 0, 0.08, 0.16, 0.21, mat.orange);
  for (const side of [-1, 1]) {
    const leg = localBox(g, side * 0.15, 0.38, 0, 0.2, 0.75, 0.22, uniform);
    leg.userData.bot = bot;
    bot.legs.push(leg);
    const arm = localBox(g, side * 0.34, 1.01, 0.1, 0.16, 0.55, 0.18, uniform);
    arm.rotation.x = -0.6;
    arm.userData.bot = bot;
  }
  localBox(g, 0, 0.95, 0.39, 0.13, 0.13, 0.6, mat.dark);
  g.position.set(bot.x, 0, bot.z);
  bots.push(bot);
  return bot;
}
for (let i = 0; i < 6; i++) makeBot(i);
const state = {
  x: 0,
  z: 18,
  y: 1.7,
  vy: 0,
  yaw: 0,
  pitch: 0,
  health: 100,
  time: 180,
  kills: 0,
  mode: "ready",
};
const keys = new Set();
let firing = false,
  clock = 0,
  hitTime = 0,
  hurtTime = 0,
  flashTime = 0,
  recoil = 0,
  feedTime = 0;
const ray = new THREE.Raycaster();
const direction = new THREE.Vector3();
let audio;
function sound(type) {
  try {
    audio ??= new AudioContext();
    if (audio.state === "suspended") audio.resume();
    const now = audio.currentTime;
    if (type === "rifle" || type === "sniper") {
      const length = type === "sniper" ? 0.38 : 0.13,
        buffer = audio.createBuffer(
          1,
          Math.ceil(audio.sampleRate * length),
          audio.sampleRate,
        ),
        data = buffer.getChannelData(0);
      for (let i = 0; i < data.length; i++)
        data[i] =
          (Math.random() * 2 - 1) *
          Math.exp(
            -i / (audio.sampleRate * (type === "sniper" ? 0.07 : 0.025)),
          );
      const source = audio.createBufferSource(),
        filter = audio.createBiquadFilter(),
        gain = audio.createGain();
      source.buffer = buffer;
      filter.type = "lowpass";
      filter.frequency.value = type === "sniper" ? 1700 : 2600;
      gain.gain.value = type === "sniper" ? 0.22 : 0.11;
      source.connect(filter);
      filter.connect(gain);
      gain.connect(audio.destination);
      source.start();
    }
    const ticks = type === "reload" ? [0, 0.45, 1.3] : [0];
    for (const delay of ticks) {
      const osc = audio.createOscillator(),
        gain = audio.createGain();
      osc.type = type === "sniper" ? "sine" : "triangle";
      osc.frequency.setValueAtTime(
        type === "sniper"
          ? 82
          : type === "rifle"
            ? 135
            : type === "reload"
              ? 480
              : 330,
        now + delay,
      );
      osc.frequency.exponentialRampToValueAtTime(
        type === "reload" ? 170 : 38,
        now + delay + 0.11,
      );
      gain.gain.setValueAtTime(
        type === "sniper" ? 0.13 : type === "rifle" ? 0.055 : 0.025,
        now + delay,
      );
      gain.gain.exponentialRampToValueAtTime(0.001, now + delay + 0.13);
      osc.connect(gain);
      gain.connect(audio.destination);
      osc.start(now + delay);
      osc.stop(now + delay + 0.15);
    }
  } catch {}
}

function clearLine(from, to) {
  direction.copy(to).sub(from);
  const length = direction.length();
  ray.set(from, direction.normalize());
  const hits = ray.intersectObjects(solidMeshes, false);
  return !hits.length || hits[0].distance > length - 0.2;
}
function shoot() {
  if (state.mode !== "playing") return false;
  const gun = currentGun();
  if (!gun.fire()) {
    if (!gun.ammo && !gun.reloadTime)
      $("message").textContent = "PRESS R TO RELOAD";
    return false;
  }
  sound(activeWeapon);
  flashTime = activeWeapon === "sniper" ? 0.09 : 0.055;
  recoil = activeWeapon === "sniper" ? 0.14 : 0.055;
  camera.getWorldDirection(direction);
  ray.set(camera.position, direction);
  const targets = [
    ...solidMeshes,
    ...bots.filter((b) => b.hp > 0).map((b) => b.group),
  ];
  const hits = ray.intersectObjects(targets, true);
  if (hits.length && hits[0].object.userData.bot) {
    const target = hits[0].object.userData.bot;
    target.hp -=
      activeWeapon === "sniper"
        ? hits[0].object.userData.head
          ? 220
          : 125
        : hits[0].object.userData.head
          ? 75
          : 36;
    hitTime = 0.15;
    if (target.hp <= 0) {
      state.kills++;
      target.group.rotation.z = Math.PI / 2;
      target.group.position.y = 0.25;
      $("feed").textContent =
        `${activeWeapon === "sniper" ? "AW-9" : "AR-7"}  /  PATROL ${bots.indexOf(target) + 1} NEUTRALIZED`;
      feedTime = 2.5;
      if (state.kills === 6)
        finish(true, "All six patrol units neutralized. Depot secured.");
    }
  }
  return true;
}
function reset() {
  Object.assign(state, {
    x: 0,
    z: 18,
    y: 1.7,
    vy: 0,
    yaw: 0,
    pitch: 0,
    health: 100,
    time: 180,
    kills: 0,
  });
  rifle.reset();
  sniper.reset();
  activeWeapon = "rifle";
  scoped = false;
  scopeBlend = 0;
  keys.clear();
  firing = false;
  hitTime = hurtTime = flashTime = recoil = 0;
  bots.forEach((b, i) => {
    b.hp = 100;
    b.x = starts[i][0];
    b.z = starts[i][1];
    b.cooldown = 1 + i * 0.35;
    b.group.rotation.set(0, 0, 0);
    b.group.position.set(b.x, 0, b.z);
  });
  $("feed").textContent = "";
  $("message").textContent = "";
}
function showOverlay(title, copy, label) {
  $("overlay-title").textContent = title;
  $("overlay-copy").textContent = copy;
  $("start").innerHTML = `${label} <span>→</span>`;
  $("overlay").classList.remove("hidden");
}
function finish(won, copy) {
  state.mode = won ? "won" : "lost";
  firing = false;
  keys.clear();
  document.exitPointerLock?.();
  showOverlay(won ? "DEPOT SECURED" : "MISSION FAILED", copy, "DEPLOY AGAIN");
}
function pause() {
  if (state.mode !== "playing") return;
  state.mode = "paused";
  keys.clear();
  firing = false;
  showOverlay(
    "MISSION PAUSED",
    "Take a breath. Your round will resume where you left it.",
    "RESUME MISSION",
  );
}
function start() {
  if (state.mode !== "paused") reset();
  state.mode = "playing";
  $("overlay").classList.add("hidden");
  renderer.domElement.requestPointerLock?.();
  sound("start");
}
$("start").addEventListener("click", start);
document.addEventListener("pointerlockchange", () => {
  if (document.pointerLockElement !== renderer.domElement) pause();
});
document.addEventListener("mousemove", (event) => {
  if (
    state.mode !== "playing" ||
    document.pointerLockElement !== renderer.domElement
  )
    return;
  state.yaw -= event.movementX * 0.002 * (scoped ? 0.35 : 1);
  state.pitch = THREE.MathUtils.clamp(
    state.pitch - event.movementY * 0.002 * (scoped ? 0.35 : 1),
    -1.35,
    1.35,
  );
});
document.addEventListener("keydown", (event) => {
  if (["Space", "ArrowUp", "ArrowDown"].includes(event.code))
    event.preventDefault();
  if (event.code === "Escape") {
    pause();
    document.exitPointerLock?.();
    return;
  }
  if (event.code === "Enter" && state.mode !== "playing") {
    start();
    return;
  }
  if (state.mode !== "playing") return;
  keys.add(event.code);
  if (event.code === "KeyR" && currentGun().reload()) sound("reload");
  if (event.code === "Digit1") selectWeapon("rifle");
  if (event.code === "Digit2") selectWeapon("sniper");
  if (event.code === "Space" && state.y <= 1.701) state.vy = 6;
});
document.addEventListener("keyup", (event) => keys.delete(event.code));
renderer.domElement.addEventListener("contextmenu", (event) =>
  event.preventDefault(),
);
renderer.domElement.addEventListener("mousedown", (event) => {
  if (state.mode !== "playing") return;
  if (event.button === 2) {
    if (activeWeapon === "sniper") {
      scoped = !scoped;
      sound("scope");
    }
    return;
  }
  if (event.button !== 0) return;
  if (document.pointerLockElement !== renderer.domElement) {
    renderer.domElement.requestPointerLock?.();
    return;
  }
  firing = true;
  shoot();
});
document.addEventListener("mouseup", () => (firing = false));
window.addEventListener("blur", () => {
  pause();
  document.exitPointerLock?.();
});
window.addEventListener("resize", () => {
  camera.aspect = innerWidth / innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(innerWidth, innerHeight);
});

function updateBots(dt) {
  for (const b of bots) {
    if (b.hp <= 0) continue;
    const dx = state.x - b.x,
      dz = state.z - b.z,
      dist = Math.hypot(dx, dz);
    b.phase += dt * 6;
    const visible = clearLine(
      new THREE.Vector3(b.x, 1.35, b.z),
      camera.position,
    );
    if (dist > 6) {
      let mx, mz;
      if (visible) {
        mx = (dx / dist) * 1.35 * dt;
        mz = (dz / dist) * 1.35 * dt;
      } else {
        mx = Math.sin(clock * 0.5 + b.start[0]) * 0.8 * dt;
        mz = Math.cos(clock * 0.4 + b.start[1]) * 0.8 * dt;
      }
      moveActor(b, mx, mz, colliders, 0.36);
    }
    b.group.position.x = b.x;
    b.group.position.z = b.z;
    b.group.rotation.y = Math.atan2(dx, dz);
    b.legs.forEach(
      (leg, i) =>
        (leg.rotation.x =
          dist > 6 ? Math.sin(b.phase + i * Math.PI) * 0.25 : 0),
    );
    b.cooldown -= dt;
    if (visible && dist < 23 && b.cooldown <= 0) {
      b.cooldown = 1.15 + random() * 0.6;
      state.health = Math.max(0, state.health - 9);
      hurtTime = 0.22;
      sound("hit");
      if (!state.health) {
        finish(
          false,
          "You were eliminated. Use crates and containers for cover.",
        );
        break;
      }
    }
  }
}
function radar() {
  const ctx = $("minimap").getContext("2d");
  ctx.clearRect(0, 0, 170, 170);
  ctx.save();
  ctx.translate(85, 85);
  ctx.fillStyle = "#61716a";
  ctx.strokeStyle = "#b7c1ab";
  ctx.lineWidth = 1;
  for (const b of colliders) {
    ctx.fillRect((b.x - b.w / 2) * 3, (b.z - b.d / 2) * 3, b.w * 3, b.d * 3);
  }
  for (const b of bots)
    if (b.hp > 0) {
      ctx.fillStyle = "#e9866b";
      ctx.beginPath();
      ctx.arc(b.x * 3, b.z * 3, 3, 0, Math.PI * 2);
      ctx.fill();
    }
  ctx.translate(state.x * 3, state.z * 3);
  ctx.rotate(-state.yaw);
  ctx.fillStyle = "#e7d39a";
  ctx.beginPath();
  ctx.moveTo(0, -6);
  ctx.lineTo(-4, 5);
  ctx.lineTo(4, 5);
  ctx.closePath();
  ctx.fill();
  ctx.restore();
}
function hud() {
  const gun = currentGun();
  const time = Math.max(0, Math.ceil(state.time));
  $("timer").textContent =
    `${String(Math.floor(time / 60)).padStart(2, "0")}:${String(time % 60).padStart(2, "0")}`;
  $("timer").classList.toggle("urgent", time < 30);
  $("health").textContent = state.health;
  $("healthbar").style.width = state.health + "%";
  $("ammo").textContent = gun.ammo;
  $("reserve").textContent = gun.reserve;
  $("weapon-label").textContent =
    activeWeapon === "sniper"
      ? "AW-9 / BOLT-ACTION SNIPER"
      : "AR-7 / ASSAULT RIFLE";
  $("remaining").textContent = 6 - state.kills;
  $("hitmarker").style.opacity = hitTime > 0 ? 1 : 0;
  $("damage").style.opacity = hurtTime > 0 ? 0.7 : 0;
  if (gun.reloadTime > 0) $("message").textContent = "RELOADING";
  else if (gun.ammo > 0) $("message").textContent = "";
  radar();
}
let previous = performance.now();
function frame(now) {
  const dt = Math.min((now - previous) / 1000, 0.04);
  previous = now;
  if (state.mode === "playing") {
    clock += dt;
    state.time -= dt;
    rifle.update(dt);
    sniper.update(dt);
    if (firing) shoot();
    let forward = (keys.has("KeyW") ? 1 : 0) - (keys.has("KeyS") ? 1 : 0),
      side = (keys.has("KeyD") ? 1 : 0) - (keys.has("KeyA") ? 1 : 0);
    const norm = Math.hypot(forward, side) || 1;
    forward /= norm;
    side /= norm;
    const speed = (keys.has("ShiftLeft") ? 6 : 4) * dt;
    moveActor(
      state,
      (-Math.sin(state.yaw) * forward + Math.cos(state.yaw) * side) * speed,
      (-Math.cos(state.yaw) * forward - Math.sin(state.yaw) * side) * speed,
      colliders,
    );
    state.vy -= 16 * dt;
    state.y = Math.max(1.7, state.y + state.vy * dt);
    if (state.y === 1.7) state.vy = 0;
    camera.position.set(state.x, state.y, state.z);
    camera.rotation.set(state.pitch, state.yaw, 0);
    updateBots(dt);
    if (state.time <= 0 && state.mode === "playing")
      finish(false, "Time expired. The patrol still controls the depot.");
    flashTime = Math.max(0, flashTime - dt);
    hitTime = Math.max(0, hitTime - dt);
    hurtTime = Math.max(0, hurtTime - dt);
    recoil = Math.max(0, recoil - dt * 0.4);
    feedTime = Math.max(0, feedTime - dt);
    if (!feedTime) $("feed").textContent = "";
    const bob = forward || side ? Math.sin(clock * 10) * 0.008 : 0;
    for (const model of [weapon, sniperModel]) {
      model.position.set(
        model === sniperModel ? 0.34 : 0.31,
        (model === sniperModel ? -0.32 : -0.29) + bob,
        (model === sniperModel ? -0.72 : -0.61) + recoil,
      );
      model.rotation.x = currentGun().reloadTime > 0 ? -0.45 : recoil * 0.8;
      model.rotation.z = currentGun().reloadTime > 0 ? -0.25 : 0;
    }
  } else if (state.mode === "ready") {
    camera.position.set(0, 1.7, 18);
    camera.rotation.set(-0.03, 0, 0);
  }
  scopeBlend = THREE.MathUtils.damp(
    scopeBlend,
    scoped && activeWeapon === "sniper" ? 1 : 0,
    14,
    dt,
  );
  camera.fov = THREE.MathUtils.lerp(76, 20, scopeBlend);
  camera.updateProjectionMatrix();
  $("scope").style.opacity = scopeBlend;
  $("scope").firstElementChild.style.transform =
    `scale(${1.08 - 0.08 * scopeBlend})`;
  $("crosshair").style.opacity = 1 - scopeBlend;
  weapon.visible = activeWeapon === "rifle";
  sniperModel.visible = activeWeapon === "sniper" && scopeBlend < 0.9;
  flash.visible = flashTime > 0 && activeWeapon === "rifle";
  sniperFlash.visible = flashTime > 0 && activeWeapon === "sniper";
  flashLight.intensity = flashTime > 0 ? 2 : 0;
  hud();
  renderer.render(scene, camera);
  requestAnimationFrame(frame);
}
requestAnimationFrame(frame);
// Explicit test mode exposes state/actions only to the local smoke runner.
if (new URLSearchParams(location.search).has("test"))
  window.__strike = {
    state,
    bots,
    rifle,
    sniper,
    colliders,
    camera,
    renderer,
    shoot,
    start,
    reset,
    pause,
    finish,
    selectWeapon,
    get activeWeapon() {
      return activeWeapon;
    },
    get scoped() {
      return scoped;
    },
    snapshot: () => ({
      mode: state.mode,
      health: state.health,
      ammo: currentGun().ammo,
      reserve: currentGun().reserve,
      weapon: activeWeapon,
      scoped,
      kills: state.kills,
      x: state.x,
      z: state.z,
      time: state.time,
    }),
    aimAt(index) {
      const b = bots[index],
        dx = b.x - state.x,
        dz = b.z - state.z;
      state.yaw = Math.atan2(-dx, -dz);
      state.pitch = Math.atan2(1.08 - state.y, Math.hypot(dx, dz));
      camera.position.set(state.x, state.y, state.z);
      camera.rotation.set(state.pitch, state.yaw, 0);
      scene.updateMatrixWorld(true);
    },
  };
