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
//   up. Player y is eye height; feet are y - EYE_HEIGHT. Bot feet and crouch
//   shift their rendered stance, eye/muzzle positions and hitboxes. AABB obstacle x/z values are center points.
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
#include "tactical_policy.hpp"

namespace strike {

constexpr float PI = 3.14159265358979323846f;
constexpr float MAP_LIMIT = 31.5f;
constexpr float PLAYER_RADIUS = .35f;
constexpr float EYE_HEIGHT = 1.7f;
constexpr float ROUND_SECONDS = 180.f;
constexpr float WALK_SPEED=4.f,SPRINT_SPEED=6.f,CROUCH_DROP=.65f,AI_TURN_RATE=3.5f;

struct Vec2 { float x = 0, z = 0; };
struct Vec3 { float x = 0, y = 0, z = 0; };
enum class ObstacleKind { Wall, Building, Container, Crate, Barrel };
struct Obstacle { float x, z, w, d, h; ObstacleKind kind = ObstacleKind::Crate; };
#include "map_layout.hpp"

enum class WeaponId { AR7 = 0, AW9 = 1 };
enum class Mode { Ready, Playing, Paused, Won, Lost, Draw };
enum class EventType {
    Restarted, PauseChanged, PlayerShot, EmptyClick, ReloadStarted,
    ReloadCompleted, WeaponSwitched, ScopeChanged, BotShot,
    PlayerFootstep, BotFootstep, Landed, PlayerHit, BotHit, BotKilled, Jumped, Won, Lost, Draw
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
    float baseSpread=.008f,recoilKick=.018f,recoilSide=.004f;
};
inline constexpr std::array<WeaponSpec, 2> WEAPON_SPECS{{
    {"AR-7", 30, 90, 36, 75, .115f, 1.65f, .008f, .018f, .004f},
    {"AW-9", 5, 20, 125, 220, 1.35f, 2.5f, .0025f, .060f, .010f}
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
    float crouch = 0, speed=0,recoilPitch=0,recoilYaw=0,switchRemaining=0;
    float eyeHeight() const { return EYE_HEIGHT - CROUCH_DROP*crouch; }
};
enum class Awareness { Patrol, Investigate, Chase, Search };
struct Noise { Vec2 position; float radius; };
struct Impact { Vec3 point, normal, origin, direction; };
struct Bot {
    Vec2 pos{}, spawn{};
    int hp = 100;
    float phase = 0, cooldown = 0, shotFlash = 0;
    float yaw = 0, pitch=0,recoilPitch=0,recoilYaw=0;
    float feet = 0, verticalVelocity = 0, crouch = 0, speed = 0;
    float footstepDistance = 0;
    float actionCooldown = 2, crouchRemaining = 0;
    Awareness awareness = Awareness::Patrol;
    Vec2 interest{}, searchCenter{};
    float memory = 0, searchRemaining = 0, patrolRemaining = 0;
    unsigned patrolStep = 0, searchStep = 0;
    int tacticalAction=tactics::Search, forcedAction=-1;
    float decisionRemaining=0, switchRemaining=0,weaponCommitment=0;
    int decisionState=0;
    WeaponId selected=WeaponId::AR7;
    std::array<WeaponState,2> guns{};
    Bot(){for(int w=0;w<2;w++)guns[w].reset(WEAPON_SPECS[w]);}
    Vec2 navigationTarget{};
    float navigationCooldown = 0;
    bool alive() const { return hp > 0; }
};
struct Input {
    bool forward = false, backward = false, left = false, right = false;
    bool sprint = false, walk=false, crouch = false, jump = false, fire = false, reload = false;
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
    constexpr int extent = static_cast<int>(MAP_LIMIT), side = extent * 2 + 1, count = side * side;
    std::array<bool, count> free{};
    std::array<int, count> parent{};
    std::array<int, count> queue{};
    parent.fill(-2);
    int source = -1, destination = -1;
    float sourceDistance = std::numeric_limits<float>::infinity();
    float destinationDistance = sourceDistance;
    const auto point = [=](int cell) {
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
inline float accuracyMultiplier(float speed,float crouch,bool airborne){
    float movement=speed>5.f?3.f:speed>2.1f?1.5f:speed>.05f?.75f:1.f;
    return movement*(1.f-.5f*crouch)*(airborne?4.f:1.f);
}
inline Vec3 fireDirection(float yaw,float pitch,float& recoilYaw,float& recoilPitch,
                         const WeaponSpec& spec,float speed,float crouch,bool airborne,std::mt19937& rng){
    auto random=[&](){return float(rng()%10001)/5000.f-1.f;};
    const float spread=spec.baseSpread*accuracyMultiplier(speed,crouch,airborne);
    const auto ray=viewDirection(yaw+recoilYaw+random()*spread,pitch+recoilPitch+random()*spread);
    recoilPitch=std::min(.20f,recoilPitch+spec.recoilKick);
    recoilYaw=std::clamp(recoilYaw+random()*spec.recoilSide,-.08f,.08f);
    return ray;
}
inline float botHeadBase(const Bot& bot) { return bot.feet + 1.55f - CROUCH_DROP * bot.crouch; }
inline Vec3 botEye(const Bot& bot) { return {bot.pos.x, bot.feet + 1.7f - CROUCH_DROP * bot.crouch, bot.pos.z}; }
inline Vec3 botMuzzle(const Bot& bot) {
    const auto forward = viewDirection(bot.yaw, 0);
    const float reach=.87f;
    return {bot.pos.x + forward.x * reach, bot.feet + 1.32f - CROUCH_DROP * bot.crouch,
            bot.pos.z + forward.z * reach};
}
inline bool botSees(const Bot& bot, Vec3 target, const std::vector<Obstacle>& obstacles) {
    const auto eye = botEye(bot);
    const float dx = target.x - eye.x, dz = target.z - eye.z;
    const float horizontal = std::hypot(dx, dz);
    if (horizontal > 30.f || std::abs(target.y - eye.y) > std::max(1.f, horizontal)) return false;
    const auto forward = viewDirection(bot.yaw, 0);
    if (horizontal > .001f && (forward.x * dx + forward.z * dz) / horizontal < .5f) return false;
    return lineOfSight(eye, target, obstacles);
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
    std::vector<Impact> impacts;
    std::vector<Noise> noises; // Discrete stimuli consumed once by all bots.
    float footstepDistance = 0;
    unsigned impactSerial = 0;

    void makeNoise(Vec2 position, float radius) { noises.push_back({position, radius}); }
    void leaveImpact(Vec3 origin, Vec3 direction, float hitDistance) {
        if (!std::isfinite(hitDistance) || hitDistance > 120) return;
        const Vec3 point{origin.x + direction.x * hitDistance,
                         origin.y + direction.y * hitDistance,
                         origin.z + direction.z * hitDistance};
        Vec3 normal{};
        float nearest = std::numeric_limits<float>::infinity();
        for (const auto& box : obstacles) {
            if (std::abs(rayBox(origin, direction,
                {box.x-box.w/2,0,box.z-box.d/2},
                {box.x+box.w/2,box.h,box.z+box.d/2}) - hitDistance) > .002f) continue;
            const std::array<std::pair<float,Vec3>,6> faces{{
                {std::abs(point.x-(box.x-box.w/2)),{-1,0,0}},
                {std::abs(point.x-(box.x+box.w/2)),{1,0,0}},
                {std::abs(point.z-(box.z-box.d/2)),{0,0,-1}},
                {std::abs(point.z-(box.z+box.d/2)),{0,0,1}},
                {std::abs(point.y),{0,-1,0}}, {std::abs(point.y-box.h),{0,1,0}}}};
            for (const auto& face : faces) if(face.first < nearest) {
                nearest=face.first; normal=face.second;
            }
        }
        if(nearest > .01f) return;
        if(impacts.size() >= 256) impacts.erase(impacts.begin());
        impacts.push_back({point,normal,origin,direction});
        ++impactSerial;
    }

    bool trainedBots=false;
    tactics::Policy botPolicy{};
    void useBotPolicy(const tactics::Policy& policy) { botPolicy=policy;trainedBots=true; }
    tactics::Observation botObservation(std::size_t index) const {
        const auto& b=bots.at(index);
        const Vec3 target{player.pos.x,player.y,player.pos.z};
        const bool seen=botSees(b,target,obstacles);
        const bool known=seen||b.memory>0;
        const Vec2 evidence=seen?player.pos:b.searchCenter;
        const float range=known?distance(b.pos,evidence):40;
        const auto& gun=b.guns[static_cast<std::size_t>(b.selected)];
        const bool covered=known&&!lineOfSight(botEye(b),{evidence.x,EYE_HEIGHT,evidence.z},obstacles);
        return {seen,known,covered,b.hp<=40,gun.ammo<=int(gun.spec.capacity*.2f),
                range<9?0:range<18?1:2,int(b.selected)};
    }
    tactics::ActionMask botActionMask(std::size_t index) const {
        const auto& b=bots.at(index);auto o=botObservation(index);
        const auto& gun=b.guns[static_cast<std::size_t>(b.selected)];
        bool cover=o.covered||(o.known&&distance(botCoverGoal(b),b.pos)>.1f);
        bool clear=true;
        if(o.visible)clear=lineOfSight(botEye(b),{player.pos.x,player.y,player.pos.z},obstacles);
        return tactics::legalActions(o,{gun.ammo,b.guns[1-static_cast<std::size_t>(b.selected)].ammo,
            b.switchRemaining,b.weaponCommitment,gun.reloadRemaining,cover,clear});
    }
    Vec2 botCoverGoal(const Bot& bot) const {
        Vec2 best=bot.pos;float cost=std::numeric_limits<float>::infinity();
        for(const auto& box:obstacles){
            const std::array<Vec2,4> candidates{{{box.x-box.w/2-.8f,box.z},{box.x+box.w/2+.8f,box.z},
                {box.x,box.z-box.d/2-.8f},{box.x,box.z+box.d/2+.8f}}};
            for(auto p:candidates){
                if(circleBlocked(p,.36f,obstacles)||lineOfSight({p.x,1.2f,p.z},
                    {bot.searchCenter.x,EYE_HEIGHT,bot.searchCenter.z},obstacles))continue;
                const float d=distance(bot.pos,p);
                if(d<cost){cost=d;best=p;}
            }
        }
        return best;
    }
    bool desertMap = false;
    explicit Game(bool desert=false):desertMap(desert) {
        if(desertMap)obstacles=desertObstacles();
        resetState();mode=Mode::Ready;
    }
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
        player.switchRemaining=.35f;
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
        if (mode != Mode::Playing || player.switchRemaining>0) return false;
        if (!weapon().fire()) {
            if (weapon().ammo == 0 && weapon().cooldown <= 0 && weapon().reloadRemaining <= 0) {
                events.push_back({EventType::EmptyClick, -1, selectedWeapon});
                weapon().cooldown = .2f;
            }
            return false;
        }
        makeNoise(player.pos, selectedWeapon == WeaponId::AW9 ? 48.f : 38.f);
        muzzleFlash = selectedWeapon == WeaponId::AW9 ? .09f : .055f;
        events.push_back({EventType::PlayerShot, -1, selectedWeapon, player.pos});
        const Vec3 origin{player.pos.x, player.y, player.pos.z};
        const Vec3 direction = fireDirection(player.yaw,player.pitch,player.recoilYaw,player.recoilPitch,
            weapon().spec,player.speed,player.crouch,player.y>player.eyeHeight()+.001f,random_);
        float closest = std::min(120.f, obstacleRay(origin, direction, obstacles));
        int target = -1;
        bool head = false;
        for (std::size_t i = 0; i < bots.size(); ++i) {
            const auto& bot = bots[i];
            if (!bot.alive()) continue;
            const float headHit = rayBox(origin, direction,
                {bot.pos.x - .18f, botHeadBase(bot), bot.pos.z - .18f},
                {bot.pos.x + .18f, botHeadBase(bot) + .33f, bot.pos.z + .18f});
            const float bodyHit = rayBox(origin, direction,
                {bot.pos.x - .31f, bot.feet + .2f, bot.pos.z - .25f},
                {bot.pos.x + .31f, botHeadBase(bot), bot.pos.z + .25f});
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
        if (target < 0) { leaveImpact(origin, direction, closest); return true; }
        auto& bot = bots[static_cast<std::size_t>(target)];
        const int damage = head ? weapon().spec.headDamage : weapon().spec.bodyDamage;
        if(combatStep_)queuedBotDamage_[static_cast<std::size_t>(target)]+=damage;
        else bot.hp = std::max(0, bot.hp - damage);
        lastShot.damage = damage;
        lastShot.killed = !bot.alive();
        hitMarker = .15f;
        events.push_back({EventType::BotHit, target, selectedWeapon, bot.pos, static_cast<float>(damage)});
        if (!combatStep_&&!bot.alive()) {
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
        player.switchRemaining=std::max(0.f,player.switchRemaining-dt);
        player.recoilPitch*=std::exp(-dt*8);player.recoilYaw*=std::exp(-dt*8);
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
        const float feet=std::max(0.f,player.y-player.eyeHeight());
        const bool grounded=feet<=.001f;
        player.crouch+=std::clamp((input.crouch?1.f:0.f)-player.crouch,-dt*5,dt*5);
        player.y=feet+player.eyeHeight();
        const bool sprinting=input.sprint && !input.walk && player.crouch<.1f;
        const float speed = (sprinting ? SPRINT_SPEED : input.walk?2.f:WALK_SPEED-2.f*player.crouch) * dt;
        const Vec2 previousPlayer = player.pos;
        moveActor(player.pos,
            (-std::sin(player.yaw) * forward + std::cos(player.yaw) * side) * speed,
            (-std::cos(player.yaw) * forward - std::sin(player.yaw) * side) * speed,
            obstacles, PLAYER_RADIUS, std::max(0.f, player.y - player.eyeHeight()));
        player.speed=dt>0?distance(previousPlayer,player.pos)/dt:0;
        if (grounded) {
            footstepDistance += distance(previousPlayer, player.pos);
            const float stride = sprinting ? 1.6f : 1.35f-.4f*player.crouch;
            if (footstepDistance >= stride) {
                const float loudness=sprinting?1.25f:1.f-.75f*player.crouch;
                makeNoise(player.pos, sprinting ? 15.f : 8.f-5.5f*player.crouch);
                events.push_back({EventType::PlayerFootstep,-1,selectedWeapon,player.pos,loudness});
                footstepDistance = std::fmod(footstepDistance, stride);
            }
        }
        if (input.jump && grounded && player.crouch<.1f) {
            player.verticalVelocity = 6;
            events.push_back({EventType::Jumped});
        }
        player.verticalVelocity -= 16 * dt;
        player.y = std::max(player.eyeHeight(), player.y + player.verticalVelocity * dt);
        if (player.y <= player.eyeHeight()) {
            if (!grounded) {
                makeNoise(player.pos, 12.f);
                events.push_back({EventType::Landed,-1,selectedWeapon,player.pos,1.35f});
            }
            player.verticalVelocity = 0;
        }
        queuedBotDamage_.fill(0);queuedPlayerDamage_=0;combatStep_=true;
        if (input.fire) shoot();
        updateBots(dt);combatStep_=false;
        for(std::size_t i=0;i<bots.size();i++)if(queuedBotDamage_[i]>0&&bots[i].alive()){
            bots[i].hp=std::max(0,bots[i].hp-queuedBotDamage_[i]);
            if(!bots[i].alive()){
                ++kills;events.push_back({EventType::BotKilled,static_cast<int>(i),selectedWeapon,bots[i].pos});
                if(lastShot.bot==static_cast<int>(i))lastShot.killed=true;
            }
        }
        player.health=std::max(0,player.health-queuedPlayerDamage_);
        if(player.health<=0&&remaining()==0){mode=Mode::Draw;scoped=false;events.push_back({EventType::Draw});}
        else if(player.health<=0)finish(false);
        else if(remaining()==0)finish(true);
        if (mode == Mode::Playing && timeLeft <= 0) finish(false);
    }

private:
    bool combatStep_=false;
    std::array<int,6> queuedBotDamage_{};int queuedPlayerDamage_=0;
    std::mt19937 random_{711};
    void resetState() {
        player = Player{};
        if(desertMap)player.pos=DESERT_PLAYER_SPAWN;
        kills = 0;
        timeLeft = ROUND_SECONDS;
        elapsed = muzzleFlash = hitMarker = damageFlash = 0;
        selectedWeapon = WeaponId::AR7;
        scoped = false;
        lastShot = ShotResult{};
        events.clear();
        impacts.clear(); noises.clear(); footstepDistance=0; impactSerial=0;
        random_.seed(711);
        for (std::size_t i = 0; i < weapons.size(); ++i) weapons[i].reset(WEAPON_SPECS[i]);
        constexpr std::array<Vec2, 6> starts{{{0, 1}, {-3, -6}, {8, 10}, {4, -15}, {-16, 16}, {17, -4}}};
        for (std::size_t i = 0; i < bots.size(); ++i) {
            bots[i] = Bot{};
            bots[i].pos = bots[i].spawn = desertMap?DESERT_BOT_SPAWNS[i]:starts[i];
            bots[i].yaw = static_cast<float>(i) * 2.39996f;
            bots[i].interest = bots[i].spawn;
            bots[i].actionCooldown = 1.5f + static_cast<float>(i) * .55f;
            for(int w=0;w<2;w++)bots[i].guns[w].reset(WEAPON_SPECS[w]);
            bots[i].phase = static_cast<float>(i);
            bots[i].cooldown = 0;
        }
    }
    void finish(bool won) {
        mode = won ? Mode::Won : Mode::Lost;
        scoped = false;
        events.push_back({won ? EventType::Won : EventType::Lost});
    }
    Vec2 explorePoint(Vec2 center, float radius, unsigned step) const {
        // World-only deterministic samples: never use the hidden player position.
        for (unsigned n=0; n<24; ++n) {
            const float angle=(step+n)*2.39996f;
            const float r=radius*(.45f+.55f*static_cast<float>((step+n)%7)/6.f);
            Vec2 p{std::clamp(center.x+std::cos(angle)*r,-30.f,30.f),
                   std::clamp(center.z+std::sin(angle)*r,-30.f,30.f)};
            if(!circleBlocked(p,.4f,obstacles) && distance(center,p)>.5f
               && distance(navigationWaypoint(center,p,obstacles),center)>.1f) return p;
        }
        return center;
    }
    void updateBots(float dt) {
        for (std::size_t i = 0; i < bots.size(); ++i) {
            auto& bot = bots[i];
            bot.shotFlash = std::max(0.f, bot.shotFlash - dt);
            bot.recoilPitch*=std::exp(-dt*8);bot.recoilYaw*=std::exp(-dt*8);
            if (!bot.alive()) continue;
            // Live player coordinates are used only by the sight sensor and
            // the final, visibility-gated shot. Navigation uses remembered evidence.
            const Vec3 target{player.pos.x, player.y, player.pos.z};
            const bool visible = botSees(bot,target,obstacles);
            bot.memory = std::max(0.f,bot.memory-dt);
            bot.searchRemaining = std::max(0.f,bot.searchRemaining-dt);
            bot.patrolRemaining = std::max(0.f,bot.patrolRemaining-dt);
            if (visible) {
                bot.awareness=Awareness::Chase;
                bot.interest=bot.searchCenter=player.pos;
                bot.memory=6.f; bot.searchRemaining=0;
            } else {
                bool heard=false;
                for(const auto& noise:noises) {
                    const bool muffled=!lineOfSight(botEye(bot),
                        {noise.position.x,1.f,noise.position.z},obstacles);
                    if(distance(bot.pos,noise.position)>noise.radius*(muffled?.45f:1.f)) continue;
                    // Coarse sound localization, not an exact player locator.
                    bot.interest=bot.searchCenter={std::round(noise.position.x/3)*3,
                                                  std::round(noise.position.z/3)*3};
                    bot.memory=6.f;bot.awareness=Awareness::Investigate;
                    bot.searchRemaining=0;heard=true;
                }
                if(!heard && (bot.awareness==Awareness::Chase || bot.awareness==Awareness::Investigate)
                   && (bot.memory<=0 || distance(bot.pos,bot.interest)<1.f)) {
                    bot.awareness=Awareness::Search;bot.searchRemaining=8.f;
                    bot.interest=explorePoint(bot.searchCenter,5.f,++bot.searchStep);
                    bot.navigationCooldown=0;
                }
                if(bot.awareness==Awareness::Search) {
                    if(bot.searchRemaining<=0) {bot.awareness=Awareness::Patrol;bot.patrolRemaining=0;}
                    else if(distance(bot.pos,bot.interest)<.8f) {
                        bot.interest=explorePoint(bot.searchCenter,5.f,++bot.searchStep);
                        bot.navigationCooldown=0;
                    }
                }
                if(bot.awareness==Awareness::Patrol &&
                   (bot.patrolRemaining<=0 || distance(bot.pos,bot.interest)<1.f)) {
                    bot.interest=explorePoint(bot.spawn,26.f,++bot.patrolStep+static_cast<unsigned>(i)*7);
                    bot.patrolRemaining=12.f;bot.navigationCooldown=0;
                }
            }
            const float range=distance(bot.pos,bot.interest);
            bot.actionCooldown -= dt;
            bot.crouchRemaining = std::max(0.f, bot.crouchRemaining - dt);
            if (!trainedBots && bot.actionCooldown <= 0 && bot.feet <= 0) {
                // Stagger actions so patrols do not jump/crouch in unison.
                if (visible && range < 16 && (i + static_cast<int>(elapsed / 3)) % 2 == 0)
                    bot.crouchRemaining = 1.3f;
                else if (bot.awareness!=Awareness::Patrol && range > 7) bot.verticalVelocity = 6.f;
                bot.actionCooldown = 3.5f + static_cast<float>(i) * .27f;
            }
            if(trainedBots)bot.crouchRemaining=bot.tacticalAction==tactics::Hide&&distance(bot.pos,botCoverGoal(bot))<1?.1f:0;
            const float stance = bot.crouchRemaining > 0 && bot.feet <= 0 ? 1.f : 0.f;
            bot.crouch += std::clamp(stance - bot.crouch, -dt * 5, dt * 5);
            // Jump is a real vertical arc. Horizontal collision stays conservative:
            // bots jump on navigable ground rather than clipping through cover.
            const bool wasAirborne=bot.feet>0;
            bot.verticalVelocity -= 16 * dt;
            bot.feet = std::max(0.f, bot.feet + bot.verticalVelocity * dt);
            if (bot.feet <= 0){
                if(wasAirborne)events.push_back({EventType::BotFootstep,static_cast<int>(i),bot.selected,bot.pos,1.35f});
                bot.verticalVelocity = 0;
            }
            bot.navigationCooldown -= dt;
            bot.switchRemaining=std::max(0.f,bot.switchRemaining-dt);
            bot.weaponCommitment=std::max(0.f,bot.weaponCommitment-dt);
            for(auto& gun:bot.guns)gun.update(dt);
            Vec2 goal = bot.interest;
            bool advance=true,wantsFire=true;
            if(trainedBots){
                const auto observation=botObservation(i);
                bot.decisionRemaining-=dt;
                if(bot.decisionRemaining<=.0001f){
                    const auto allowed=botActionMask(i);const int oldAction=bot.tacticalAction;
                    bot.tacticalAction=bot.forcedAction>=0&&bot.forcedAction<tactics::ACTIONS&&allowed[bot.forcedAction]?
                        bot.forcedAction:botPolicy.greedyMasked(observation,allowed);
                    bot.decisionState=observation.index();
                    if(oldAction!=bot.tacticalAction)bot.navigationCooldown=0;
                    bot.decisionRemaining=.25f;
                }
                const int action=bot.tacticalAction;
                wantsFire=action==tactics::Shoot||action==tactics::Rifle||action==tactics::Sniper;
                advance=action==tactics::Chase||action==tactics::Hide||action==tactics::Left||
                        action==tactics::Right||action==tactics::Search;
                if(action==tactics::Rifle||action==tactics::Sniper){
                    const auto weapon=action==tactics::Sniper?WeaponId::AW9:WeaponId::AR7;
                    if(weapon!=bot.selected){bot.selected=weapon;bot.switchRemaining=.35f;bot.weaponCommitment=2.f;}
                }
                if(action==tactics::Reload)bot.guns[static_cast<std::size_t>(bot.selected)].reload();
                if(observation.known){
                    if(action==tactics::Hide)goal=botCoverGoal(bot);
                    else if(action==tactics::Left||action==tactics::Right){
                        const float dx=bot.searchCenter.x-bot.pos.x,dz=bot.searchCenter.z-bot.pos.z;
                        const float d=std::max(.01f,std::hypot(dx,dz)),side=action==tactics::Left?-1.f:1.f;
                        goal={std::clamp(bot.pos.x+side*dz/d*5,-30.f,30.f),std::clamp(bot.pos.z-side*dx/d*5,-30.f,30.f)};
                    }else if(action==tactics::Chase)goal=bot.searchCenter;
                }
                // An attack command cannot fire at an occluded target. Move
                // toward remembered evidence to reacquire instead of freezing.
                if(observation.known&&!observation.visible&&wantsFire){
                    advance=true;wantsFire=false;goal=bot.searchCenter;
                }
                // Return fire while moving/retreating when actually visible;
                // shooting is still gated by aim, muzzle clearance and readiness.
                if(observation.visible&&action!=tactics::Reload)wantsFire=true;
                if(!observation.known){advance=true;wantsFire=false;}
            }
            const bool routeClear=lineOfSight({bot.pos.x,.35f,bot.pos.z},
                                              {goal.x,.35f,goal.z},obstacles);
            if (!routeClear) {
                if (bot.navigationCooldown <= 0 || distance(bot.pos, bot.navigationTarget) < .2f) {
                    bot.navigationTarget = navigationWaypoint(bot.pos, goal, obstacles);
                    bot.navigationCooldown = .8f + static_cast<float>(i) * .04f;
                }
                goal = bot.navigationTarget;
            } else bot.navigationCooldown = 0;
            const float gx = goal.x - bot.pos.x, gz = goal.z - bot.pos.z;
            const Vec2 aim=trainedBots&&bot.memory>0?bot.searchCenter:goal;
            const float desiredYaw = std::atan2(bot.pos.x-aim.x,bot.pos.z-aim.z)-bot.recoilYaw;
            bot.yaw = std::remainder(bot.yaw + std::clamp(std::remainder(desiredYaw - bot.yaw, 2 * PI),
                                                         -dt * AI_TURN_RATE, dt * AI_TURN_RATE), 2 * PI);
            const Vec2 before = bot.pos;
            if (advance && distance(bot.pos,goal) > (trainedBots?.5f:(visible?6.f:.5f))) {
                const float length = std::hypot(gx, gz);
                const float pace = (bot.memory>0 && distance(bot.pos,bot.searchCenter)>12 && bot.crouch<.1f ? SPRINT_SPEED : trainedBots&&visible?2.f:WALK_SPEED-2.f*bot.crouch);
                const float amount = std::min(length, pace * dt);
                if (length > .001f) moveActor(bot.pos, gx / length * amount, gz / length * amount, obstacles, PLAYER_RADIUS);
            }
            bot.speed = dt > 0 ? distance(before, bot.pos) / dt : 0;
            bot.phase += distance(before, bot.pos) * 7;
            if(bot.feet<=.001f) {
                bot.footstepDistance+=distance(before,bot.pos);
                const bool sprinting=bot.speed>5.f;
                const float stride=sprinting?1.6f:1.35f-.4f*bot.crouch;
                if(bot.footstepDistance>=stride) {
                    events.push_back({EventType::BotFootstep,static_cast<int>(i),WeaponId::AR7,bot.pos,
                                      (sprinting?1.25f:1.f-.75f*bot.crouch)});
                    bot.footstepDistance=std::fmod(bot.footstepDistance,stride);
                }
            }
            bot.cooldown -= dt;
            // Check the final position and posture, both eyes and muzzle. The
            // muzzle must also be reachable from the body (no barrel through walls).
            const Vec3 origin=botEye(bot);
            const float targetFeet=std::max(0.f,player.y-player.eyeHeight());
            const float bodyY=targetFeet+1.1f-CROUCH_DROP*player.crouch;
            const float finalRange=distance(bot.pos,player.pos);
            const float desiredPitch=std::atan2(bodyY-origin.y,std::max(.001f,finalRange));
            bot.pitch=std::clamp(bot.pitch+std::clamp(desiredPitch-bot.recoilPitch-bot.pitch,-dt*AI_TURN_RATE,dt*AI_TURN_RATE),-1.35f,1.35f);
            const float shotYaw=std::atan2(bot.pos.x-player.pos.x,bot.pos.z-player.pos.z);
            const bool aimed=std::abs(std::remainder(shotYaw-bot.yaw-bot.recoilYaw,2*PI))<.15f&&std::abs(desiredPitch-bot.pitch-bot.recoilPitch)<.10f;
            auto& botGun=bot.guns[static_cast<std::size_t>(bot.selected)];
            if(wantsFire&&finalRange<120&&aimed&&bot.cooldown<=0&&bot.switchRemaining<=0&&
               botGun.cooldown<=0&&botGun.reloadRemaining<=0&&botGun.ammo>0&&botSees(bot,target,obstacles)){
                if(!botGun.fire())continue;
                bot.shotFlash=bot.selected==WeaponId::AW9?.09f:.055f;
                events.push_back({EventType::BotShot,static_cast<int>(i),bot.selected,bot.pos});
                const auto ray=fireDirection(bot.yaw,bot.pitch,bot.recoilYaw,bot.recoilPitch,botGun.spec,bot.speed,bot.crouch,bot.feet>.001f,random_);
                const float headBase=targetFeet+1.55f-CROUCH_DROP*player.crouch;
                const float head=rayBox(origin,ray,{player.pos.x-.18f,headBase,player.pos.z-.18f},
                    {player.pos.x+.18f,headBase+.33f,player.pos.z+.18f});
                const float body=rayBox(origin,ray,{player.pos.x-.31f,targetFeet+.2f,player.pos.z-.25f},
                    {player.pos.x+.31f,headBase,player.pos.z+.25f});
                if(std::min(head,body)<std::min(120.f,obstacleRay(origin,ray,obstacles))){
                    const int damage=head<body?botGun.spec.headDamage:botGun.spec.bodyDamage;
                    if(combatStep_)queuedPlayerDamage_+=damage;else player.health=std::max(0,player.health-damage);
                    damageFlash=.22f;events.push_back({EventType::PlayerHit,static_cast<int>(i),bot.selected,player.pos,static_cast<float>(damage)});
                }

            }
        }
        noises.clear();
    }
};

} // namespace strike
