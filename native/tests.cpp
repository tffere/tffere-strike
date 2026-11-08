#include "gameplay.hpp"
#include "bot_pose.hpp"
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
        {"trained policy switches weapons with delay and tracks sniper ammunition", [] {
            Game g;g.start();quiet(g);g.obstacles.clear();g.player.pos={0,-8};
            auto& b=g.bots[0];b=Bot{};b.pos={0,0};b.cooldown=0;
            tactics::Policy p;for(auto& row:p.q)row[tactics::Sniper]=1;g.useBotPolicy(p);
            g.step(.05f);check(b.selected==WeaponId::AW9,"policy did not select sniper");
            check(g.player.health==100&&b.guns[1].ammo==5,"switch fired early");
            for(int i=0;i<8;i++)g.step(.05f);
            check(b.guns[1].ammo==4&&g.player.health==69,"sniper did not obey ammo/damage");
            check(b.guns[0].ammo==30,"switch destroyed rifle ammunition");
        }},
        {"trained reload consumes reserve and policy survives restart", [] {
            Game g;g.start();quiet(g);g.obstacles.clear();g.player.pos={0,-8};
            auto& b=g.bots[0];b=Bot{};b.selected=WeaponId::AW9;b.guns[1].ammo=0;
            tactics::Policy p;for(auto& row:p.q)row[tactics::Reload]=1;g.useBotPolicy(p);
            for(int i=0;i<60;i++)g.step(.05f);
            check(b.guns[1].ammo==5&&b.guns[1].reserve==15,"trained reload failed");
            g.restart();check(g.trainedBots&&g.bots[0].guns[1].ammo==5,"restart lost policy/reset weapon state");
        }},
        {"trained hidden-player navigation uses evidence, not live coordinates", [] {
            Game a,b;a.start();b.start();quiet(a);quiet(b);
            tactics::Policy p;for(auto& row:p.q)row[tactics::Hide]=1;a.useBotPolicy(p);b.useBotPolicy(p);
            a.obstacles=b.obstacles={{0,5,66,1,10},{3,-2,2,3,3}};
            for(Game* g:{&a,&b}){
                g->bots[0]=Bot{};g->bots[0].memory=6;g->bots[0].searchCenter={0,10};
                g->bots[0].awareness=Awareness::Investigate;g->bots[0].interest={0,10};
            }
            a.player.pos={-20,20};b.player.pos={20,20};
            for(int i=0;i<30;i++){a.step(.05f);b.step(.05f);}
            check(distance(a.bots[0].pos,b.bots[0].pos)<.001f,"trained hide leaked target position");
            check(!circleBlocked(a.bots[0].pos,.35f,a.obstacles),"trained navigation entered cover");
        }},
        {"trained firing is blocked by walls and empty magazines", [] {
            Game g;g.start();quiet(g);g.player.pos={0,-8};g.obstacles={{0,-4,4,1,4}};
            auto& b=g.bots[0];b=Bot{};b.cooldown=0;
            tactics::Policy p;for(auto& row:p.q)row[tactics::Shoot]=1;g.useBotPolicy(p);
            g.step(.1f);check(g.player.health==100&&!hasEvent(g,EventType::BotShot),"trained shot through wall");
            g.obstacles.clear();b.guns[0].ammo=0;b.cooldown=0;b.decisionRemaining=0;
            g.step(.1f);check(g.player.health==100&&!hasEvent(g,EventType::BotShot),"empty trained gun fired");
        }},
        {"player crouch lowers eyes without moving feet, slows movement and disables sprint", [] {
            Game g;g.start();quiet(g);g.obstacles.clear();
            Input in;in.crouch=in.sprint=in.forward=true;
            Vec2 start=g.player.pos;g.step(.25f,in);
            check(near(g.player.crouch,1)&&near(g.player.y,1.05f),"crouch eye height incorrect");
            check(near(g.player.y-g.player.eyeHeight(),0),"crouching moved feet");
            check(near(distance(start,g.player.pos),.5f),"crouch did not slow/disable sprint");
            g.step(.1f);
            check(g.player.crouch>0&&g.player.crouch<1,"standing snapped instead of blending");
            g.step(.1f);check(near(g.player.y,EYE_HEIGHT),"standing left camera below floor");
        }},
        {"crouching avoids low-cover sight and produces quieter footsteps", [] {
            Bot b;b.pos={0,0};b.yaw=0;
            std::vector<Obstacle> cover{{0,-4.5f,4,1,1.3f}};
            check(botSees(b,{0,1.7f,-6},cover),"standing player should be visible");
            check(!botSees(b,{0,1.05f,-6},cover),"crouching player seen through low cover");
            Game g;g.start();quiet(g);g.obstacles.clear();Input in;in.crouch=in.forward=true;
            bool step=false;
            for(int i=0;i<20;i++){
                g.step(.05f,in);
                for(auto e:g.events)if(e.type==EventType::PlayerFootstep){step=true;check(e.value<.5f,"crouch footsteps too loud");}
            }
            check(step,"crouch movement never produced footsteps");
        }},
        {"footstep sounds follow grounded travel, not held movement keys", [] {
            Game g;g.start();quiet(g);g.player.pos={0,5};g.obstacles={{0,3.9f,4,1,4}};
            Input in;in.forward=true;
            for(int i=0;i<20;i++){g.step(.05f,in);check(!hasEvent(g,EventType::PlayerFootstep),"blocked movement made step sounds");}
            g.obstacles.clear();g.player.y=EYE_HEIGHT+3;g.player.verticalVelocity=0;
            for(int i=0;i<5;i++){g.step(.05f,in);check(!hasEvent(g,EventType::PlayerFootstep),"airborne movement made step sounds");}
        }},
        {"crouch during jump preserves landing height and pause freezes stance", [] {
            Game g;g.start();quiet(g);g.obstacles.clear();Input in;in.jump=true;g.step(.05f,in);
            check(g.player.y>EYE_HEIGHT,"initial jump failed");in={};in.crouch=true;
            for(int i=0;i<40;i++)g.step(.025f,in);
            check(near(g.player.y,g.player.eyeHeight()),"crouch jump failed to land");
            g.togglePause();float y=g.player.y,c=g.player.crouch;g.step(.25f);
            check(near(g.player.y,y)&&near(g.player.crouch,c),"pause changed crouch pose");
            g.restart();check(near(g.player.crouch,0)&&near(g.player.y,EYE_HEIGHT),"restart retained crouch");
        }},
        {"collapse bends knees before impact and settles in varied grounded poses", [] {
            auto start=collapsePose(0,.94f,1,0),early=collapsePose(.15f,.94f,1,0),end=collapsePose(1,.94f,1,0);
            check(near(start.hipHeight,.94f)&&near(start.torsoPitch,0),"collapse starts displaced");
            check(early.armRelease>0&&early.hipHeight<start.hipHeight,"collapse remains rigid");
            check(near(end.hipHeight,.24f)&&end.legFold>.99f&&end.armRelease>.99f,"body never settles");
            auto reverse=collapsePose(1,.39f,-1,1);
            check(reverse.torsoPitch<0&&end.torsoPitch>0&&reverse.torsoRoll!=end.torsoRoll,"death variations identical");
            for(int i=0;i<=100;i++){
                auto pose=collapsePose(i/100.f,.39f,-1,1);
                check(std::isfinite(pose.hipHeight)&&pose.hipHeight>=.22f,"collapse clips pelvis below floor");
            }
        }},
        {"silent hidden player positions cannot influence patrols", [] {
            Game a,b; a.start();b.start();quiet(a);quiet(b);
            a.obstacles=b.obstacles={{0,5,66,1,12}};
            for(Game* g:{&a,&b}) {
                auto& bot=g->bots[0];bot=Bot{};bot.pos=bot.spawn={0,0};bot.cooldown=100;
            }
            a.player.pos={-20,20};b.player.pos={20,20};
            for(int i=0;i<200;i++) {a.step(.02f);b.step(.02f);}
            const auto& x=a.bots[0];const auto& y=b.bots[0];
            check(near(x.pos.x,y.pos.x)&&near(x.pos.z,y.pos.z)&&near(x.yaw,y.yaw),"hidden location changed patrol");
            check(x.awareness==Awareness::Patrol && x.patrolStep>0,"idle bot did not explore");
            check(distance(x.pos,x.spawn)>.5f,"patrol never moved");
        }},
        {"last sight stays fixed until search and patrol replace stale memory", [] {
            Game g;g.start();g.obstacles.clear();quiet(g);
            auto& b=g.bots[0];b=Bot{};b.pos={0,0};b.hp=100;b.cooldown=100;
            g.player.pos={0,-8};g.step(.02f);
            check(b.awareness==Awareness::Chase,"visible target not acquired");
            Vec2 remembered=b.interest;
            g.obstacles={{0,-4,66,1,10}};g.player.pos={18,-12};
            g.step(.02f);
            check(near(b.interest.x,remembered.x)&&near(b.interest.z,remembered.z),"bot tracked hidden player");
            b.memory=.01f;g.step(.02f);
            check(b.awareness==Awareness::Search,"expired memory did not start search");
            check(near(b.searchCenter.x,remembered.x),"search center followed hidden target");
            b.searchRemaining=.01f;g.step(.02f);
            check(b.awareness==Awareness::Patrol,"search never returned to patrol");
        }},
        {"gunshots are localized once and walls attenuate hearing", [] {
            Game g;g.start();quiet(g);g.obstacles={{0,5,66,1,10}};
            auto& b=g.bots[0];b=Bot{};b.cooldown=100;g.player.pos={1,10};
            g.shoot();g.step(.02f);
            check(b.awareness==Awareness::Investigate,"nearby hidden gunshot ignored");
            check(near(b.interest.x,0)&&near(b.interest.z,9),"sound should have coarse location");
            Vec2 heard=b.interest;g.player.pos={-20,20};g.step(.02f);
            check(near(b.interest.x,heard.x)&&near(b.interest.z,heard.z),"consumed noise followed player");
            g.restart();quiet(g);b=Bot{};b.cooldown=100;
            g.player.pos={0,24};g.shoot();g.step(.02f);
            check(b.awareness==Awareness::Patrol,"wall did not attenuate distant gunshot");
        }},
        {"footsteps require actual grounded travel and do not reveal silent players", [] {
            Game g;g.start();quiet(g);g.obstacles.clear();
            auto& b=g.bots[0];b=Bot{};b.interest={0,-20};b.patrolRemaining=100;b.cooldown=100;
            g.player.pos={0,5};g.player.yaw=PI;
            g.step(.25f);
            check(b.awareness==Awareness::Patrol,"standing player made footsteps");
            Input in;in.forward=true;g.step(.25f,in);g.step(.1f,in);
            check(b.awareness==Awareness::Investigate,"nearby actual footsteps ignored");
            g.restart();quiet(g);b=Bot{};b.interest={0,-20};b.patrolRemaining=100;b.cooldown=100;
            g.player.pos={0,5};g.player.yaw=PI;g.obstacles={{0,6,4,1,3}};
            for(int i=0;i<8;i++)g.step(.1f,in);
            check(b.awareness==Awareness::Patrol,"walking against wall made footsteps");
        }},
        {"bullet marks sit on the nearest hit face and reset each round", [] {
            Game g;g.start();quiet(g);g.player.pos={0,5};
            g.obstacles={{0,0,4,2,4},{0,-5,4,2,4}};
            g.shoot();check(g.impacts.size()==1,"wall shot left no mark");
            const auto& m=g.impacts.back();
            check(near(m.point.z,1)&&near(m.normal.z,1)&&near(m.point.y,EYE_HEIGHT),"mark on wrong face");
            for(int i=0;i<270;i++){g.weapon().ammo=30;g.weapon().cooldown=0;g.shoot();}
            check(g.impacts.size()==256,"impact limit not enforced");
            g.restart();check(g.impacts.empty(),"marks survived restart");
        }},
        {"body hits and sky misses do not create wall marks", [] {
            Game g;g.start();quiet(g);g.obstacles.clear();
            auto& b=g.bots[0];b.hp=100;b.pos={0,5};aim(g,b.pos);g.shoot();
            check(g.lastShot.hit&&g.impacts.empty(),"body hit left a floating mark");
            b.hp=0;g.weapon().cooldown=0;g.player.pitch=1.2f;g.shoot();
            check(g.impacts.empty(),"sky miss left a floating mark");
        }},
        {"vision rejects targets behind, outside cone, or behind cover", [] {
            Bot bot; bot.pos={0,0}; bot.yaw=0;
            check(botSees(bot,{0,1.7f,-8},{}), "front target unseen");
            check(!botSees(bot,{0,1.7f,8},{}), "target behind seen");
            check(!botSees(bot,{8,1.7f,0},{}), "side target seen");
            check(!botSees(bot,{0,1.7f,-8},{{0,-4,3,1,3}}), "wall did not occlude sight");
            check(!botSees(bot,{0,1.7f,-31},{}), "vision range unlimited");
        }},
        {"bots turn gradually and cannot fire behind themselves", [] {
            Game game; game.start(); game.obstacles.clear(); quiet(game);
            auto& b=game.bots[0]; b.hp=100; b.pos={0,10}; b.yaw=0; b.cooldown=0;
            game.step(.1f);
            check(std::abs(b.yaw)<=.351f,"bot snapped to target");
            check(!hasEvent(game,EventType::BotShot),"bot fired outside vision");
        }},
        {"muzzle cover blocks firing even when eyes see over it", [] {
            Game game; game.start(); quiet(game); game.player.pos={0,-5};
            auto& b=game.bots[0]; b.hp=100; b.pos={0,0}; b.yaw=0; b.cooldown=0;
            game.obstacles={{0,-.5f,3,.2f,1.5f}};
            check(botSees(b,{0,1.7f,-5},game.obstacles),"eyes should see over cover");
            game.step(.02f);
            check(!hasEvent(game,EventType::BotShot),"barrel fired through cover");
        }},
        {"bots run, jump, land and crouch on staggered action timers", [] {
            Game game; game.start(); game.obstacles.clear(); quiet(game);
            auto& b=game.bots[0]; b.hp=100; b.pos={0,-5}; b.yaw=PI; b.cooldown=100;
            b.actionCooldown=0;
            game.step(.05f);
            check(b.speed>3,"bot did not run");
            check(b.feet>0 && b.verticalVelocity>0,"bot did not jump");
            for(int i=0;i<60;i++) game.step(.02f);
            check(near(b.feet,0),"bot failed to land");
            b.pos={0,10}; b.actionCooldown=100; b.crouchRemaining=1;
            for(int i=0;i<20;i++) game.step(.02f);
            check(b.crouch>.95f && botHeadBase(b)<1.05f,"crouch did not lower hitbox");
            for(int i=0;i<65;i++) game.step(.02f);
            check(near(b.crouch,0),"bot failed to stand");
        }},
        {"crouching and airborne hitboxes follow rendered stance", [] {
            Game game; game.start(); game.obstacles.clear(); quiet(game);
            auto& b=game.bots[0]; b.hp=100; b.pos={0,5}; b.crouch=1;
            aim(game,b.pos,1.7f); game.shoot();
            check(!game.lastShot.hit,"standing head ray hit crouching bot");
            game.weapon().cooldown=0; aim(game,b.pos,1.15f); game.shoot();
            check(game.lastShot.hit && game.lastShot.headshot,"crouching head missed");
            b.hp=100; b.crouch=0; b.feet=1; game.weapon().cooldown=0;
            aim(game,b.pos,.6f); game.shoot();
            check(!game.lastShot.hit,"ground ray hit airborne bot");
        }},
        {"circle collision, corners, clearance and map limits", [] {
            const std::vector<Obstacle> obstacles{{0, 0, 2, 2, 2}};
            check(circleBlocked({0, 0}, .35f, obstacles), "box interior must collide");
            check(circleBlocked({1.2f, 1.2f}, .35f, obstacles), "round corner must collide");
            check(!circleBlocked({1.4f, 1.4f}, .35f, obstacles), "clear corner must pass");
            check(circleBlocked({MAP_LIMIT-.2f, 0}, .35f, {}), "map edge must collide");
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
            auto& bot = game.bots[0]; bot.hp = 100; bot.pos = {0, 5}; bot.yaw=PI; bot.cooldown = 0;
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
        {"heard gunshot routes a patrol around cover into line of sight", [] {
            Game game; game.start();
            game.makeNoise(game.player.pos,48);
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
        {"desert spawn and restart preserve the expanded map", [] {
            Game g(true);g.start();
            check(near(g.player.pos.x,DESERT_PLAYER_SPAWN.x)&&near(g.player.pos.z,DESERT_PLAYER_SPAWN.z),"wrong desert spawn");
            check(!circleBlocked(g.player.pos,.36f,g.obstacles),"player spawned in a wall");
            for(auto b:g.bots)check(!circleBlocked(b.pos,.36f,g.obstacles),"bot spawned in cover");
            g.player.pos={25,20};g.restart();
            check(near(g.player.pos.z,26)&&g.obstacles.size()==desertObstacles().size(),"restart changed map");
        }},
        {"all desert patrol spawns have a continuous walkable route", [] {
            Game g(true);
            for(auto b:g.bots){
                Vec2 p=b.pos;
                for(int n=0;n<300&&distance(p,g.player.pos)>.7f;n++){
                    Vec2 next=navigationWaypoint(p,g.player.pos,g.obstacles);
                    float d=distance(next,p);
                    check(d>.001f,"navigation found no desert route");
                    moveActor(p,next.x-p.x,next.z-p.z,g.obstacles,.36f);
                    check(!circleBlocked(p,.35f,g.obstacles),"route crossed solid cover");
                }
                check(distance(p,g.player.pos)<.7f,"desert route did not reach spawn");
            }
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
