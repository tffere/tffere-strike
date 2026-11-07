export const MAP_LIMIT = 21.5;
export function circleBlocked(x, z, radius, obstacles, limit = MAP_LIMIT) {
  if (Math.abs(x) + radius > limit || Math.abs(z) + radius > limit) return true;
  return obstacles.some((b) => {
    const px = Math.max(b.x - b.w / 2, Math.min(x, b.x + b.w / 2));
    const pz = Math.max(b.z - b.d / 2, Math.min(z, b.z + b.d / 2));
    return (x - px) ** 2 + (z - pz) ** 2 < radius ** 2;
  });
}
export function moveActor(actor, dx, dz, obstacles, radius = 0.35) {
  // Sweep in small steps so a fast frame cannot tunnel through a wall.
  const steps = Math.max(1, Math.ceil(Math.hypot(dx, dz) / 0.15));
  for (let i = 0; i < steps; i++) {
    if (!circleBlocked(actor.x + dx / steps, actor.z, radius, obstacles))
      actor.x += dx / steps;
    if (!circleBlocked(actor.x, actor.z + dz / steps, radius, obstacles))
      actor.z += dz / steps;
  }
}
export class Rifle {
  constructor({
    capacity = 30,
    reserve = 90,
    interval = 0.115,
    reloadDuration = 1.65,
  } = {}) {
    this.capacity = capacity;
    this.initialReserve = reserve;
    this.interval = interval;
    this.reloadDuration = reloadDuration;
    this.reset();
  }
  reset() {
    this.ammo = this.capacity;
    this.reserve = this.initialReserve;
    this.cooldown = 0;
    this.reloadTime = 0;
  }
  fire() {
    if (this.cooldown > 0 || this.reloadTime > 0 || this.ammo === 0)
      return false;
    this.ammo--;
    this.cooldown = this.interval;
    return true;
  }
  reload() {
    if (this.reloadTime || this.ammo === this.capacity || this.reserve === 0)
      return false;
    this.reloadTime = this.reloadDuration;
    return true;
  }
  update(dt) {
    this.cooldown = Math.max(0, this.cooldown - dt);
    if (this.reloadTime > 0) {
      this.reloadTime = Math.max(0, this.reloadTime - dt);
      if (this.reloadTime === 0) {
        const take = Math.min(this.capacity - this.ammo, this.reserve);
        this.ammo += take;
        this.reserve -= take;
      }
    }
  }
}

export class Sniper extends Rifle {
  constructor() {
    super({ capacity: 5, reserve: 20, interval: 1.35, reloadDuration: 2.5 });
  }
}
