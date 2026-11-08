#include "gameplay.hpp"
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace strike;
void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
bool near(float a, float b, float tolerance = .001f) {
    return std::abs(a - b) < tolerance;
}
void quiet(Game& game) {
    for (auto& bot : game.bots) bot.hp = 0;
}
void aim(Game& game, Vec2 position, float height = 1.05f) {
    const float dx = position.x - game.player.pos.x;
    const float dz = position.z - game.player.pos.z;
    game.player.yaw = std::atan2(-dx, -dz);
    game.player.pitch = std::atan2(height - game.player.y, std::hypot(dx, dz));
}
bool hasEvent(const Game& game, EventType type) {
    return std::any_of(game.events.begin(), game.events.end(),
                       [type](const Event& event) { return event.type == type; });
}
}

int main() {
    using namespace strike;
    using Test = std::pair<const char*, std::function<void()>>;
    const std::vector<Test> tests{
        {"circle collision, corners, clearance and map limits", [] {
            const std::vector<Obstacle> obstacles{{0, 0, 2, 2, 2}};
            check(circleBlocked({0, 0}, .35f, obstacles), "box interior must collide");
            check(circleBlocked({1.2f, 1.2f}, .35f, obstacles), "round corner must collide");
            check(!circleBlocked({1.4f, 1.4f}, .35f, obstacles), "clear corner must pass");
            check(circleBlocked({21.3f, 0}, .35f, {}), "map edge must collide");
            check(!circleBlocked({0, 0}, .35f, obstacles, 2.01f), "feet above box must clear it");
        }},
        {"swept movement blocks tunneling and slides along cover", [] {
            const std::vector<Obstacle> obstacles{{0, 0, 2, 2, 2}};
            Vec2 p{-5, 0};
            moveActor(p, 10, 0, obstacles);
            check(p.x <= -1.35f && p.x > -1.5f, "large movement tunneled through wall");
            p = {-1.4f, 0};
            moveActor(p, 1, 1, obstacles);
            check(near(p.x, -1.4f) && p.z > .9f, "wall sliding failed");
        }},
        {"3D ray slabs and line of sight account for height", [] {
            const std::vector<Obstacle> obstacles{{0, 0, 2, 2, 2}};
            check(near(rayBox({0, 1, 5}, {0, 0, -1}, {-1, 0, -1}, {1, 2, 1}), 4), "front ray intersection wrong");
            check(std::isinf(rayBox({3, 1, 5}, {0, 0, -1}, {-1, 0, -1}, {1, 2, 1})), "parallel ray should miss");
            check(!lineOfSight({0, 1, 5}, {0, 1, -5}, obstacles), "wall must block line of sight");
            check(lineOfSight({0, 3, 5}, {0, 3, -5}, obstacles), "ray above wall should pass");
        }},
        {"rifle capacity and cadence are enforced", [] {
            WeaponState gun;
            gun.reset(WEAPON_SPECS[0]);
            check(gun.fire() && gun.ammo == 29, "first shot failed");
            check(!gun.fire(), "rifle fired during cooldown");
            for (int i = 0; i < 29; ++i) {
                gun.update(.2f);
                check(gun.fire(), "rifle failed after cooldown");
            }
            gun.update(.2f);
            check(gun.ammo == 0 && !gun.fire(), "empty rifle fired");
        }},
        {"reload consumes finite reserve only when completed", [] {
            WeaponState gun;
            gun.reset(WEAPON_SPECS[0]);
            gun.ammo = 12;
            check(gun.reload(), "reload failed");
            check(!gun.fire(), "weapon fired during reload");
            check(!gun.update(1) && gun.ammo == 12, "reload completed too soon");
            check(gun.update(1) && gun.ammo == 30 && gun.reserve == 72, "reload accounting wrong");
            gun.ammo = 0;
            gun.reserve = 5;
            gun.reload();
            gun.update(2);
            check(gun.ammo == 5 && gun.reserve == 0, "partial reserve reload wrong");
            check(!gun.reload(), "reload succeeded with no reserve");
        }},
        {"sniper has independent ammo and slow cadence", [] {
            WeaponState gun;
            gun.reset(WEAPON_SPECS[1]);
            check(gun.ammo == 5 && gun.reserve == 20, "sniper loadout wrong");
            gun.fire();
            gun.update(.5f);
            check(!gun.fire(), "sniper fired faster than bolt cadence");
            gun.update(.9f);
            check(gun.fire() && gun.ammo == 3, "sniper failed after bolt cycle");
            gun.reload();
            check(!gun.update(2), "sniper reload too fast");
            check(gun.update(.6f) && gun.ammo == 5 && gun.reserve == 18, "sniper reload wrong");
        }},
        {"movement conventions, diagonal speed and jumping", [] {
            Game straight, diagonal;
            straight.start(); diagonal.start();
            straight.obstacles.clear(); diagonal.obstacles.clear();
            quiet(straight); quiet(diagonal);
            Input forward; forward.forward = true;
            straight.step(.2f, forward);
            check(near(straight.player.pos.z, 17.2f), "yaw zero does not face -Z");
            Input both = forward; both.right = true;
            diagonal.step(.2f, both);
            check(near(distance(diagonal.player.pos, {0, 18}), .8f), "diagonal movement is faster");
            Input turn; turn.lookX = PI / 2;
            straight.step(0, turn);
            const float x = straight.player.pos.x;
            straight.step(.1f, forward);
            check(straight.player.pos.x > x + .39f, "positive lookX should turn right");
            Input jump; jump.jump = true;
            straight.step(.1f, jump);
            check(straight.player.y > EYE_HEIGHT && hasEvent(straight, EventType::Jumped), "jump failed");
            for (int i = 0; i < 20; ++i) straight.step(.1f);
            check(near(straight.player.y, EYE_HEIGHT), "player did not return to ground");
        }},
        {"body hits eliminate a bot and emit hit events", [] {
            Game game; game.start();
            game.obstacles.clear(); quiet(game);
            auto& bot = game.bots[0]; bot.hp = 100; bot.pos = {0, 5};
            aim(game, bot.pos);
            for (int i = 0; i < 3; ++i) {
                game.weapon().cooldown = 0;
                check(game.shoot(), "shot did not fire");
            }
            check(bot.hp == 0 && game.kills == 1 && game.weapon().ammo == 27, "body-hit damage wrong");
            check(game.lastShot.killed && !game.lastShot.headshot, "shot result wrong");
            check(hasEvent(game, EventType::BotHit) && hasEvent(game, EventType::BotKilled), "hit events absent");
        }},
        {"headshots and sniper damage differ from body hits", [] {
            Game game; game.start(); game.obstacles.clear(); quiet(game);
            auto& bot = game.bots[0]; bot.hp = 100; bot.pos = {0, 5};
            aim(game, bot.pos, 1.7f);
            game.shoot();
            check(bot.hp == 25 && game.lastShot.headshot, "rifle head damage wrong");
            game.selectWeapon(WeaponId::AW9);
            aim(game, bot.pos);
            game.shoot();
            check(bot.hp == 0 && game.lastShot.damage == 125, "sniper body damage wrong");
        }},
        {"cover blocks both player bullets and bot attacks", [] {
            Game game; game.start(); quiet(game);
            game.obstacles = {{0, 10, 4, 1, 3}};
            auto& bot = game.bots[0]; bot.hp = 100; bot.pos = {0, 5}; bot.cooldown = 0;
            aim(game, bot.pos);
            game.shoot();
            check(bot.hp == 100 && !game.lastShot.hit, "player shot through cover");
            game.step(.2f);
            check(game.player.health == 100 && !hasEvent(game, EventType::BotShot), "bot shot through cover");
        }},
        {"nearest bot shields a farther bot from a bullet", [] {
            Game game; game.start(); game.obstacles.clear(); quiet(game);
            game.bots[0].hp = game.bots[1].hp = 100;
            game.bots[0].pos = {0, 12}; game.bots[1].pos = {0, 5};
            aim(game, game.bots[0].pos);
            game.shoot();
            check(game.bots[0].hp < 100 && game.bots[1].hp == 100, "bullet hit farther bot first");
        }},
        {"scope and switching preserve separate weapon state", [] {
            Game game; game.start(); quiet(game);
            game.shoot();
            game.selectWeapon(WeaponId::AW9);
            Input scope; scope.toggleScope = true;
            game.step(0, scope);
            check(game.scoped, "sniper scope did not activate");
            game.shoot();
            check(game.weapon().ammo == 4, "sniper ammo wrong");
            check(game.reload() && !game.scoped, "sniper reload must exit scope");
            game.selectWeapon(WeaponId::AR7);
            check(game.weapon().ammo == 29 && !game.scoped, "switch lost rifle ammo or retained scope");
            for (int i = 0; i < 11; ++i) game.step(.25f);
            check(game.weapons[1].ammo == 5 && game.weapons[1].reserve == 19, "holstered reload did not complete correctly");
        }},
        {"pause freezes movement, bots, reload and round clock", [] {
            Game game; game.start();
            game.weapon().ammo = 20; game.reload();
            game.togglePause();
            const auto player = game.player;
            const float phase = game.bots[0].phase, timer = game.timeLeft, reload = game.weapon().reloadRemaining;
            Input input; input.forward = input.fire = input.jump = true; input.lookX = 1;
            game.step(.25f, input);
            check(near(game.timeLeft, timer) && near(game.weapon().reloadRemaining, reload), "pause advanced timers");
            check(near(game.player.pos.z, player.pos.z) && near(game.player.y, player.y), "pause moved player");
            check(near(game.bots[0].phase, phase) && game.player.health == 100, "pause advanced bot AI");
            game.start(); check(game.mode == Mode::Playing, "resume failed");
        }},
        {"bots approach, attack with line of sight and cause defeat", [] {
            Game game; game.start(); game.obstacles.clear(); quiet(game);
            auto& bot = game.bots[0]; bot.hp = 100; bot.pos = {0, 5}; bot.cooldown = 0;
            const float before = distance(bot.pos, game.player.pos);
            game.step(.1f);
            check(distance(bot.pos, game.player.pos) < before, "bot did not approach");
            check(game.player.health == 91 && hasEvent(game, EventType::BotShot), "bot failed to attack");
            game.player.health = 9; bot.cooldown = 0;
            game.step(.1f);
            check(game.mode == Mode::Lost && game.player.health == 0, "bot did not cause defeat");
        }},
        {"six eliminations win and restart restores all round state", [] {
            Game game; game.start(); game.obstacles.clear();
            game.selectWeapon(WeaponId::AW9);
            for (std::size_t i = 0; i < game.bots.size(); ++i) game.bots[i].pos = {static_cast<float>(i) * 2 - 5, 5};
            for (auto& bot : game.bots) {
                game.weapon().ammo = 5; game.weapon().cooldown = 0;
                aim(game, bot.pos); game.shoot();
            }
            check(game.kills == 6 && game.mode == Mode::Won && hasEvent(game, EventType::Won), "round did not end in victory");
            game.restart();
            check(game.mode == Mode::Playing && game.kills == 0 && near(game.timeLeft, 180), "restart did not reset round");
            check(game.player.health == 100 && game.weapon().ammo == 30 && !game.scoped, "restart did not reset loadout");
            for (const auto& bot : game.bots) check(bot.hp == 100, "restart did not respawn bots");
        }},
        {"round expiry loses and large frame times are capped", [] {
            Game game; game.start(); quiet(game);
            game.step(50);
            check(near(game.timeLeft, 179.75f), "stall catch-up was not capped");
            game.timeLeft = .1f;
            game.step(.2f);
            check(game.mode == Mode::Lost && game.timeLeft == 0 && hasEvent(game, EventType::Lost), "round timeout failed");
        }},
        {"blocked patrol routes around the container into line of sight", [] {
            Game game; game.start();
            for (int i = 0; i < 1200; ++i) {
                game.player.health = 100; // isolate navigation from round defeat
                game.step(.05f);
            }
            const auto& bot = game.bots[5];
            check(distance(bot.pos, game.player.pos) < 7, "blocked bot never reached player range");
            check(lineOfSight({bot.pos.x, 1.4f, bot.pos.z},
                              {game.player.pos.x, game.player.y, game.player.pos.z}, game.obstacles),
                  "blocked bot never routed around the container");
            check(!circleBlocked(bot.pos, .35f, game.obstacles), "navigation entered solid cover");
        }},
        {"identical seeds and inputs reproduce bots and combat", [] {
            Game a, b; a.start(); b.start();
            for (int i = 0; i < 300; ++i) {
                Input input; input.right = i < 50; input.forward = i >= 50 && i < 100;
                a.step(.02f, input); b.step(.02f, input);
            }
            check(a.player.health == b.player.health && near(a.timeLeft, b.timeLeft), "round is nondeterministic");
            for (std::size_t i = 0; i < a.bots.size(); ++i)
                check(near(a.bots[i].pos.x, b.bots[i].pos.x) && near(a.bots[i].pos.z, b.bots[i].pos.z)
                      && near(a.bots[i].cooldown, b.bots[i].cooldown), "bot simulation is nondeterministic");
        }}
    };
    int failed = 0;
    for (const auto& test : tests) {
        try { test.second(); std::cout << "PASS " << test.first << '\n'; }
        catch (const std::exception& error) {
            ++failed;
            std::cerr << "FAIL " << test.first << ": " << error.what() << '\n';
        }
    }
    std::cout << tests.size() - failed << '/' << tests.size() << " tests passed\n";
    return failed ? 1 : 0;
}
