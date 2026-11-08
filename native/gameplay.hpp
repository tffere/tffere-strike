#pragma once
// Tffere Strike gameplay core: C++17, standard library only.
// Renderer integration:
//   strike::Game game; game.start();
//   strike::Input in; in.forward = ...; in.fire = heldMouseButton;
//   in.lookX = mouseDeltaX * sensitivity;  // positive = turn right
//   in.lookY = -mouseDeltaY * sensitivity; // positive = look up
//   game.step(frameSeconds, in);
//   Render game.player, game.bots, and game.obstacles. Consume game.events once
//   after each step for audio/FX. Movement/fire/sprint are held inputs; all other
//   input flags are edge inputs. Each step clears previous events, caps elapsed
//   time at 0.25 s, and preserves state while paused. Yaw 0 faces -Z; world +Y is
//   up. Player y is eye height; feet are y - EYE_HEIGHT. Bots stand at y=0 and
//   their heads occupy y=1.55..1.88. AABB obstacle x/z values are center points.
//   Weapon reload/cooldown state is preserved across switches. Reloading AW-9
//   exits scope. lastShot records the most recent fired ray for visual tracers;
//   muzzleFlash and hitMarker are short remaining durations in seconds.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <random>
#include <string_view>
#include <utility>
#include <vector>

namespace strike {

constexpr float PI = 3.14159265358979323846f;
constexpr float MAP_LIMIT = 21.5f;
constexpr float PLAYER_RADIUS = .35f;
constexpr float EYE_HEIGHT = 1.7f;
constexpr float ROUND_SECONDS = 180.f;

struct Vec2 { float x = 0, z = 0; };
struct Vec3 { float x = 0, y = 0, z = 0; };
enum class ObstacleKind { Wall, Building, Container, Crate, Barrel };
struct Obstacle { float x, z, w, d, h; ObstacleKind kind = ObstacleKind::Crate; };

enum class WeaponId { AR7 = 0, AW9 = 1 };
enum class Mode { Ready, Playing, Paused, Won, Lost };
enum class EventType {
    Restarted, PauseChanged, PlayerShot, EmptyClick, ReloadStarted,
    ReloadCompleted, WeaponSwitched, ScopeChanged, BotShot,
    PlayerHit, BotHit, BotKilled, Jumped, Won, Lost
};
struct Event {
    EventType type;
    int bot = -1;
    WeaponId weapon = WeaponId::AR7;
    Vec2 position{};
    float value = 0;
};

struct WeaponSpec {
    std::string_view name;
    int capacity, initialReserve, bodyDamage, headDamage;
    float shotInterval, reloadSeconds;
};
inline constexpr std::array<WeaponSpec, 2> WEAPON_SPECS{{
    {"AR-7", 30, 90, 36, 75, .115f, 1.65f},
    {"AW-9", 5, 20, 125, 220, 1.35f, 2.5f}
}};

struct WeaponState {
    WeaponSpec spec = WEAPON_SPECS[0];
    int ammo = 30, reserve = 90;
    float cooldown = 0, reloadRemaining = 0;

    void reset(WeaponSpec value) {
        spec = value;
        ammo = spec.capacity;
        reserve = spec.initialReserve;
        cooldown = reloadRemaining = 0;
    }
    bool fire() {
        if (cooldown > 0 || reloadRemaining > 0 || ammo <= 0) return false;
        --ammo;
        cooldown = spec.shotInterval;
        return true;
    }
    bool reload() {
        if (reloadRemaining > 0 || ammo >= spec.capacity || reserve <= 0) return false;
        reloadRemaining = spec.reloadSeconds;
        return true;
    }
    // Returns true only on the frame on which reload finishes.
    bool update(float dt) {
        cooldown = std::max(0.f, cooldown - dt);
        if (reloadRemaining <= 0) return false;
        reloadRemaining = std::max(0.f, reloadRemaining - dt);
        if (reloadRemaining > 0) return false;
        const int amount = std::min(spec.capacity - ammo, reserve);
        ammo += amount;
        reserve -= amount;
        return true;
    }
};

struct Player {
    Vec2 pos{0, 18};
    float y = EYE_HEIGHT, yaw = 0, pitch = 0, verticalVelocity = 0;
    int health = 100;
};
struct Bot {
    Vec2 pos{}, spawn{};
    int hp = 100;
    float phase = 0, cooldown = 1, shotFlash = 0;
    float yaw = 0;
    Vec2 navigationTarget{};
    float navigationCooldown = 0;
    bool alive() const { return hp > 0; }
};
struct Input {
    bool forward = false, backward = false, left = false, right = false;
    bool sprint = false, jump = false, fire = false, reload = false;
    bool toggleScope = false, selectAR7 = false, selectAW9 = false;
    bool pause = false, restart = false, start = false;
    float lookX = 0, lookY = 0;
};
struct ShotResult {
    bool fired = false, hit = false, killed = false, headshot = false;
    int bot = -1, damage = 0;
    Vec3 origin{}, point{};
};

inline float distance(Vec2 a, Vec2 b) {
    return std::hypot(a.x - b.x, a.z - b.z);
}
inline bool circleBlocked(Vec2 p, float radius,
                          const std::vector<Obstacle>& obstacles,
                          float feet = 0, float limit = MAP_LIMIT) {
    if (std::abs(p.x) + radius > limit || std::abs(p.z) + radius > limit) return true;
    for (const auto& b : obstacles) {
        if (feet >= b.h) continue;
        const float x = std::clamp(p.x, b.x - b.w / 2, b.x + b.w / 2);
        const float z = std::clamp(p.z, b.z - b.d / 2, b.z + b.d / 2);
        const float dx = p.x - x, dz = p.z - z;
        if (dx * dx + dz * dz < radius * radius) return true;
    }
    return false;
}
inline void moveActor(Vec2& pos, float dx, float dz,
                      const std::vector<Obstacle>& obstacles,
                      float radius = PLAYER_RADIUS, float feet = 0) {
    const int count = std::max(1, static_cast<int>(std::ceil(std::hypot(dx, dz) / .12f)));
    dx /= count;
    dz /= count;
    for (int i = 0; i < count; ++i) {
        const Vec2 px{pos.x + dx, pos.z};
        if (!circleBlocked(px, radius, obstacles, feet)) pos.x = px.x;
        const Vec2 pz{pos.x, pos.z + dz};
        if (!circleBlocked(pz, radius, obstacles, feet)) pos.z = pz.z;
    }
}

// A small, deterministic four-neighbour navigation grid is used only when an
// obstacle blocks sight. Direct movement remains continuous and collision uses
// the exact AABBs. Return the next reachable cell, not a teleport destination.
inline Vec2 navigationWaypoint(Vec2 from, Vec2 goal,
                                const std::vector<Obstacle>& obstacles) {
    constexpr int extent = 21, side = extent * 2 + 1, count = side * side;
    std::array<bool, count> free{};
    std::array<int, count> parent{};
    std::array<int, count> queue{};
    parent.fill(-2);
    int source = -1, destination = -1;
    float sourceDistance = std::numeric_limits<float>::infinity();
    float destinationDistance = sourceDistance;
    const auto point = [](int cell) {
        return Vec2{static_cast<float>(cell % side - extent),
                    static_cast<float>(cell / side - extent)};
    };
    for (int cell = 0; cell < count; ++cell) {
        const Vec2 p = point(cell);
        free[cell] = !circleBlocked(p, .38f, obstacles);
        if (!free[cell]) continue;
        const float ds = (p.x - from.x) * (p.x - from.x) + (p.z - from.z) * (p.z - from.z);
        const float dg = (p.x - goal.x) * (p.x - goal.x) + (p.z - goal.z) * (p.z - goal.z);
        if (ds < sourceDistance) { source = cell; sourceDistance = ds; }
        if (dg < destinationDistance) { destination = cell; destinationDistance = dg; }
    }
    if (source < 0 || destination < 0) return from;
    if (source == destination) return goal;
    int head = 0, tail = 0;
    queue[tail++] = source;
    parent[source] = -1;
    while (head < tail && parent[destination] == -2) {
        const int cell = queue[head++], x = cell % side, z = cell / side;
        const std::array<std::pair<int, int>, 4> offsets{{{0, 1}, {1, 0}, {0, -1}, {-1, 0}}};
        for (const auto& offset : offsets) {
            const int xx = x + offset.first, zz = z + offset.second;
            if (xx < 0 || xx >= side || zz < 0 || zz >= side) continue;
            const int next = zz * side + xx;
            if (!free[next] || parent[next] != -2) continue;
            parent[next] = cell;
            queue[tail++] = next;
        }
    }
    if (parent[destination] == -2) return from;
    int next = destination;
    while (parent[next] != source) {
        next = parent[next];
        if (next < 0) return from;
    }
    return point(next);
}

// Slab intersection. Infinity means miss; an origin inside a box hits at t=0.
inline float rayBox(Vec3 origin, Vec3 direction, Vec3 minimum, Vec3 maximum) {
    float near = 0, far = std::numeric_limits<float>::infinity();
    const std::array<float, 3> o{{origin.x, origin.y, origin.z}};
    const std::array<float, 3> d{{direction.x, direction.y, direction.z}};
    const std::array<float, 3> lo{{minimum.x, minimum.y, minimum.z}};
    const std::array<float, 3> hi{{maximum.x, maximum.y, maximum.z}};
    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(d[axis]) < .000001f) {
            if (o[axis] < lo[axis] || o[axis] > hi[axis])
                return std::numeric_limits<float>::infinity();
            continue;
        }
        float a = (lo[axis] - o[axis]) / d[axis];
        float b = (hi[axis] - o[axis]) / d[axis];
        if (a > b) std::swap(a, b);
        near = std::max(near, a);
        far = std::min(far, b);
        if (near > far) return std::numeric_limits<float>::infinity();
    }
    return near;
}
inline float obstacleRay(Vec3 origin, Vec3 direction,
                          const std::vector<Obstacle>& obstacles) {
    float closest = std::numeric_limits<float>::infinity();
    for (const auto& b : obstacles) {
        closest = std::min(closest, rayBox(origin, direction,
            {b.x - b.w / 2, 0, b.z - b.d / 2},
            {b.x + b.w / 2, b.h, b.z + b.d / 2}));
    }
    return closest;
}
inline bool lineOfSight(Vec3 a, Vec3 b, const std::vector<Obstacle>& obstacles) {
    const Vec3 delta{b.x - a.x, b.y - a.y, b.z - a.z};
    const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
    if (length < .0001f) return true;
    const Vec3 direction{delta.x / length, delta.y / length, delta.z / length};
    return obstacleRay(a, direction, obstacles) >= length - .01f;
}
inline Vec3 viewDirection(float yaw, float pitch) {
    const float cp = std::cos(pitch);
    return {-std::sin(yaw) * cp, std::sin(pitch), -std::cos(yaw) * cp};
}
inline std::vector<Obstacle> depotObstacles() {
    return {
        {-16, -10, 12, 14, 8, ObstacleKind::Building},
        {17, -15, 10, 12, 8, ObstacleKind::Building},
        {-11, 7, 4.4f, 8, 3.2f, ObstacleKind::Container},
        {13, 2, 4.4f, 8, 3.2f, ObstacleKind::Container},
        {-4, 8, 2.2f, 2.2f, 2.2f}, {-5.9f, 6.6f, 1.5f, 1.5f, 1.5f},
        {5, -2, 2, 2, 2}, {7.1f, -3, 1.7f, 1.7f, 1.7f},
        {-4, -12, 2.2f, 2.2f, 3.8f}, {17, 14, 2.2f, 2.2f, 2.2f},
        {18, -6, 1.8f, 1.8f, 1.8f},
        {0, -23, 48, 2, 6, ObstacleKind::Wall},
        {-23, 0, 2, 48, 6, ObstacleKind::Wall},
        {23, 0, 2, 48, 6, ObstacleKind::Wall},
        {0, 23, 48, 2, 6, ObstacleKind::Wall}
    };
}

class Game {
public:
    Player player{};
    std::array<Bot, 6> bots{};
    std::vector<Obstacle> obstacles = depotObstacles();
    std::array<WeaponState, 2> weapons{};
    WeaponId selectedWeapon = WeaponId::AR7;
    bool scoped = false;
    Mode mode = Mode::Ready;
    float timeLeft = ROUND_SECONDS, elapsed = 0, muzzleFlash = 0, hitMarker = 0, damageFlash = 0;
    int kills = 0;
    ShotResult lastShot{};
    std::vector<Event> events;

    Game() { resetState(); mode = Mode::Ready; }
    WeaponState& weapon() { return weapons[static_cast<std::size_t>(selectedWeapon)]; }
    const WeaponState& weapon() const { return weapons[static_cast<std::size_t>(selectedWeapon)]; }
    int remaining() const { return static_cast<int>(bots.size()) - kills; }

    void restart() {
        resetState();
        mode = Mode::Playing;
        events.push_back({EventType::Restarted});
    }
    void start() {
        if (mode == Mode::Paused) {
            mode = Mode::Playing;
            events.push_back({EventType::PauseChanged});
        } else if (mode != Mode::Playing) restart();
    }
    void togglePause() {
        if (mode == Mode::Playing) mode = Mode::Paused;
        else if (mode == Mode::Paused) mode = Mode::Playing;
        else return;
        events.push_back({EventType::PauseChanged});
    }
    void selectWeapon(WeaponId id) {
        if (selectedWeapon == id) return;
        if (scoped) {
            scoped = false;
            events.push_back({EventType::ScopeChanged});
        }
        selectedWeapon = id;
        events.push_back({EventType::WeaponSwitched, -1, id});
    }
    bool reload() {
        if (mode != Mode::Playing || !weapon().reload()) return false;
        if (selectedWeapon == WeaponId::AW9 && scoped) {
            scoped = false;
            events.push_back({EventType::ScopeChanged});
        }
        events.push_back({EventType::ReloadStarted, -1, selectedWeapon});
        return true;
    }
    bool shoot() {
        if (mode != Mode::Playing) return false;
        if (!weapon().fire()) {
            if (weapon().ammo == 0 && weapon().cooldown <= 0 && weapon().reloadRemaining <= 0) {
                events.push_back({EventType::EmptyClick, -1, selectedWeapon});
                weapon().cooldown = .2f;
            }
            return false;
        }
        muzzleFlash = selectedWeapon == WeaponId::AW9 ? .09f : .055f;
        events.push_back({EventType::PlayerShot, -1, selectedWeapon, player.pos});
        const Vec3 origin{player.pos.x, player.y, player.pos.z};
        const Vec3 direction = viewDirection(player.yaw, player.pitch);
        float closest = std::min(120.f, obstacleRay(origin, direction, obstacles));
        int target = -1;
        bool head = false;
        for (std::size_t i = 0; i < bots.size(); ++i) {
            const auto& bot = bots[i];
            if (!bot.alive()) continue;
            const float headHit = rayBox(origin, direction,
                {bot.pos.x - .18f, 1.55f, bot.pos.z - .18f},
                {bot.pos.x + .18f, 1.88f, bot.pos.z + .18f});
            const float bodyHit = rayBox(origin, direction,
                {bot.pos.x - .31f, .2f, bot.pos.z - .25f},
                {bot.pos.x + .31f, 1.55f, bot.pos.z + .25f});
            if (headHit < closest) {
                closest = headHit;
                target = static_cast<int>(i);
                head = true;
            }
            if (bodyHit < closest) {
                closest = bodyHit;
                target = static_cast<int>(i);
                head = false;
            }
        }
        lastShot = {true, target >= 0, false, head, target, 0, origin,
                    {origin.x + direction.x * closest, origin.y + direction.y * closest,
                     origin.z + direction.z * closest}};
        if (target < 0) return true;
        auto& bot = bots[static_cast<std::size_t>(target)];
        const int damage = head ? weapon().spec.headDamage : weapon().spec.bodyDamage;
        bot.hp = std::max(0, bot.hp - damage);
        lastShot.damage = damage;
        lastShot.killed = !bot.alive();
        hitMarker = .15f;
        events.push_back({EventType::BotHit, target, selectedWeapon, bot.pos, static_cast<float>(damage)});
        if (!bot.alive()) {
            ++kills;
            events.push_back({EventType::BotKilled, target, selectedWeapon, bot.pos});
            if (remaining() == 0) finish(true);
        }
        return true;
    }

    void step(float seconds, const Input& input = {}) {
        events.clear();
        if (input.restart) restart();
        if (input.start) start();
        if (input.pause) togglePause();
        if (mode != Mode::Playing) return;
        const float dt = std::clamp(seconds, 0.f, .25f);
        elapsed += dt;
        timeLeft = std::max(0.f, timeLeft - dt);
        muzzleFlash = std::max(0.f, muzzleFlash - dt);
        hitMarker = std::max(0.f, hitMarker - dt);
        damageFlash = std::max(0.f, damageFlash - dt);
        for (std::size_t i = 0; i < weapons.size(); ++i) {
            if (weapons[i].update(dt))
                events.push_back({EventType::ReloadCompleted, -1, static_cast<WeaponId>(i)});
        }
        if (input.selectAR7) selectWeapon(WeaponId::AR7);
        if (input.selectAW9) selectWeapon(WeaponId::AW9);
        if (input.toggleScope && selectedWeapon == WeaponId::AW9 && weapon().reloadRemaining <= 0) {
            scoped = !scoped;
            events.push_back({EventType::ScopeChanged, -1, selectedWeapon});
        }
        player.yaw = std::remainder(player.yaw - input.lookX, 2 * PI);
        player.pitch = std::clamp(player.pitch + input.lookY, -1.35f, 1.35f);
        if (input.reload) reload();
        float forward = (input.forward ? 1.f : 0.f) - (input.backward ? 1.f : 0.f);
        float side = (input.right ? 1.f : 0.f) - (input.left ? 1.f : 0.f);
        const float norm = std::hypot(forward, side);
        if (norm > 0) {
            forward /= norm;
            side /= norm;
        }
        const float speed = (input.sprint ? 6.f : 4.f) * dt;
        moveActor(player.pos,
            (-std::sin(player.yaw) * forward + std::cos(player.yaw) * side) * speed,
            (-std::cos(player.yaw) * forward - std::sin(player.yaw) * side) * speed,
            obstacles, PLAYER_RADIUS, std::max(0.f, player.y - EYE_HEIGHT));
        if (input.jump && player.y <= EYE_HEIGHT + .001f) {
            player.verticalVelocity = 6;
            events.push_back({EventType::Jumped});
        }
        player.verticalVelocity -= 16 * dt;
        player.y = std::max(EYE_HEIGHT, player.y + player.verticalVelocity * dt);
        if (player.y <= EYE_HEIGHT) player.verticalVelocity = 0;
        if (input.fire) shoot();
        if (mode != Mode::Playing) return;
        updateBots(dt);
        if (mode == Mode::Playing && timeLeft <= 0) finish(false);
    }

private:
    std::mt19937 random_{711};
    void resetState() {
        player = Player{};
        kills = 0;
        timeLeft = ROUND_SECONDS;
        elapsed = muzzleFlash = hitMarker = damageFlash = 0;
        selectedWeapon = WeaponId::AR7;
        scoped = false;
        lastShot = ShotResult{};
        events.clear();
        random_.seed(711);
        for (std::size_t i = 0; i < weapons.size(); ++i) weapons[i].reset(WEAPON_SPECS[i]);
        constexpr std::array<Vec2, 6> starts{{{0, 1}, {-3, -6}, {8, 10}, {4, -15}, {-16, 16}, {17, -4}}};
        for (std::size_t i = 0; i < bots.size(); ++i) {
            bots[i] = Bot{};
            bots[i].pos = bots[i].spawn = starts[i];
            bots[i].phase = static_cast<float>(i);
            bots[i].cooldown = 1 + static_cast<float>(i) * .35f;
        }
    }
    void finish(bool won) {
        mode = won ? Mode::Won : Mode::Lost;
        scoped = false;
        events.push_back({won ? EventType::Won : EventType::Lost});
    }
    void updateBots(float dt) {
        for (std::size_t i = 0; i < bots.size(); ++i) {
            auto& bot = bots[i];
            bot.shotFlash = std::max(0.f, bot.shotFlash - dt);
            if (!bot.alive()) continue;
            const float dx = player.pos.x - bot.pos.x, dz = player.pos.z - bot.pos.z;
            const float range = std::hypot(dx, dz);
            bot.yaw = std::atan2(-dx, -dz);
            const bool visible = lineOfSight({bot.pos.x, 1.4f, bot.pos.z},
                                            {player.pos.x, player.y, player.pos.z}, obstacles);
            bot.phase += dt * 6;
            bot.navigationCooldown -= dt;
            if (range > 6 || !visible) {
                float mx = 0, mz = 0;
                if (visible) {
                    mx = dx / range * 1.35f * dt;
                    mz = dz / range * 1.35f * dt;
                    bot.navigationCooldown = 0;
                } else {
                    if (bot.navigationCooldown <= 0 || distance(bot.pos, bot.navigationTarget) < .15f) {
                        bot.navigationTarget = navigationWaypoint(bot.pos, player.pos, obstacles);
                        bot.navigationCooldown = .8f + static_cast<float>(i) * .04f;
                    }
                    const float dist = distance(bot.pos, bot.navigationTarget);
                    if (dist > .01f) {
                        const float amount = std::min(dist, 1.2f * dt);
                        mx = (bot.navigationTarget.x - bot.pos.x) / dist * amount;
                        mz = (bot.navigationTarget.z - bot.pos.z) / dist * amount;
                    }
                }
                moveActor(bot.pos, mx, mz, obstacles, .36f);
            }
            bot.cooldown -= dt;
            if (visible && range < 23 && bot.cooldown <= 0) {
                // Direct mt19937 integer conversion avoids platform-dependent
                // uniform_real_distribution results in reproducibility tests.
                bot.cooldown = 1.15f + static_cast<float>(random_() % 6000) / 10000.f;
                bot.shotFlash = .08f;
                player.health = std::max(0, player.health - 9);
                damageFlash = .22f;
                events.push_back({EventType::BotShot, static_cast<int>(i), WeaponId::AR7, bot.pos});
                events.push_back({EventType::PlayerHit, static_cast<int>(i), WeaponId::AR7, player.pos, 9});
                if (player.health <= 0) {
                    finish(false);
                    return;
                }
            }
        }
    }
};

} // namespace strike
