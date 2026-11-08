#pragma once
#include "../gameplay.hpp"
#include <array>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>

namespace strike::training {
constexpr int ACTIONS=9, STATES=192;
constexpr float DT=.05f;
enum Action { Shoot, Rifle, Sniper, Hide, Chase, Left, Right, Reload, Search };
inline constexpr std::array<const char*,ACTIONS> ACTION_NAMES{{"shoot_current","engage_rifle","engage_sniper","hide","chase","flank_left","flank_right","reload","search"}};
struct Observation {
    bool visible=false, known=false, covered=false, lowHealth=false, lowAmmo=false;
    int range=2, weapon=0;
    int index() const {
        int n=int(visible);n=n*3+range;n=n*2+int(lowHealth);n=n*2+int(lowAmmo);
        n=n*2+int(covered);n=n*2+int(known);return n*2+weapon;
    }
};
struct Policy {
    std::array<std::array<double,ACTIONS>,STATES> q{};
    std::array<std::array<unsigned,ACTIONS>,STATES> visits{};
    int greedy(const Observation& o) const {
        const auto& row=q[o.index()];return int(std::max_element(row.begin(),row.end())-row.begin());
    }
    int choose(const Observation& o,double epsilon,std::mt19937& rng) const {
        return double(rng()%100000)/100000.<epsilon?int(rng()%ACTIONS):greedy(o);
    }
    void learn(int state,int action,double reward,int next,bool terminal) {
        auto& value=q[state][action];visits[state][action]++;
        double future=terminal?0:*std::max_element(q[next].begin(),q[next].end());
        value+=.12*(reward+.96*future-value);
    }
    void save(const std::string& path) const {
        std::ofstream out(path);if(!out)throw std::runtime_error("Cannot write policy");
        out<<"STRIKE_Q_V2 "<<STATES<<' '<<ACTIONS<<'\n'<<std::setprecision(17);
        for(int s=0;s<STATES;s++){for(double x:q[s])out<<x<<' ';out<<'\n';}
        if(!out)throw std::runtime_error("Policy write failed");
    }
    static Policy load(const std::string& path) {
        std::ifstream in(path);std::string magic;int states=0,actions=0;
        if(!(in>>magic>>states>>actions)||magic!="STRIKE_Q_V2"||states!=STATES||actions!=ACTIONS)
            throw std::runtime_error("Invalid policy header");
        Policy p;for(auto& row:p.q)for(double& x:row)if(!(in>>x)||!std::isfinite(x))throw std::runtime_error("Invalid policy values");
        return p;
    }
};
inline int fixedAction(const Observation& o,bool advanced=false) {
    if(o.lowAmmo)return Reload;
    if(!o.known)return Search;
    if(advanced&&o.lowHealth&&!o.covered)return Hide;
    if(o.visible){
        if(advanced&&o.range==2&&o.weapon==0)return Sniper;
        if(advanced&&o.range==0&&o.weapon==1)return Rifle;
        return Shoot;
    }
    return Chase;
}
inline Policy initialPolicy() {
    Policy p;
    for(int visible=0;visible<2;visible++)for(int range=0;range<3;range++)
    for(int health=0;health<2;health++)for(int ammo=0;ammo<2;ammo++)
    for(int cover=0;cover<2;cover++)for(int known=0;known<2;known++)for(int weapon=0;weapon<2;weapon++) {
        Observation o{bool(visible),bool(known),bool(cover),bool(health),bool(ammo),range,weapon};
        p.q[o.index()][fixedAction(o)]=.25; // Modest rifle-chaser warm start.
    }
    return p;
}
struct Fighter {
    Vec2 pos{}, lastKnown{}, goal{}, waypoint{};
    float yaw=0, memory=0, navTimer=0, moveDistance=0, switchDelay=0, lastSeenAge=100;
    int hp=100, selected=0;bool known=false, goalValid=false;
    unsigned heardSerial=0;
    std::array<WeaponState,2> guns{};
    Vec2 noisePosition{};float noiseRadius=0;unsigned noiseSerial=0;
};
struct Statistics {
    std::array<int,ACTIONS> decisions{};
    int shots=0,hits=0,damage=0,switches=0,hiddenTicks=0,coverEntries=0,visibleTicks=0;
    float distance=0;
};
struct Duel {
    std::array<Fighter,2> fighters{};
    std::array<Statistics,2> stats{};
    std::vector<Obstacle> obstacles;
    std::mt19937 rng;
    int ticks=0,winner=-2; // -2 ongoing, -1 draw, 0/1 winner.
    unsigned seed;
    explicit Duel(unsigned value):rng(value),seed(value) {
        float jitter=float(int(rng()%401)-200)/100.f;
        switch(value%4){
            case 0:obstacles={{0,0,3,7,3},{-8,-4,3,3,2},{8,4,3,3,2}};break;
            case 1:obstacles={{-4,0,3,12,3},{5,2,3,8,3},{0,-10,3,2,1.2f}};break;
            case 2:obstacles={{0,-5,8,2,2},{0,5,8,2,2},{-9,0,2,3,3},{9,0,2,3,3}};break;
            default:obstacles={{-5,-5,3,3,2},{5,5,3,3,2}};break;
        }
        for(auto& b:obstacles)b.x+=jitter;
        float offset=float(int(rng()%601)-300)/100.f;
        fighters[0].pos={-12,8+offset};fighters[1].pos={12,-8+offset};
        if(value%2)for(auto& f:fighters){std::swap(f.pos.x,f.pos.z);}
        for(auto& f:fighters){f.yaw=std::atan2(f.pos.x,f.pos.z);for(int w=0;w<2;w++)f.guns[w].reset(WEAPON_SPECS[w]);}
    }
    bool visible(int i) const {
        Bot sensor;sensor.pos=fighters[i].pos;sensor.yaw=fighters[i].yaw;
        const auto& e=fighters[1-i];return botSees(sensor,{e.pos.x,1.55f,e.pos.z},obstacles);
    }
    bool coverAt(Vec2 p,Vec2 threat) const {
        return !lineOfSight({p.x,1.2f,p.z},{threat.x,1.55f,threat.z},obstacles);
    }
    void sense(int i) {
        auto& f=fighters[i];const auto& e=fighters[1-i];
        const bool newNoise=e.noiseSerial!=f.heardSerial;
        f.heardSerial=e.noiseSerial;
        if(visible(i)){f.lastKnown=e.pos;f.known=true;f.memory=6;f.lastSeenAge=0;}
        else if(newNoise){
            const bool wall=!lineOfSight({f.pos.x,1.7f,f.pos.z},{e.noisePosition.x,1.f,e.noisePosition.z},obstacles);
            if(distance(f.pos,e.noisePosition)<=e.noiseRadius*(wall?.45f:1.f)){
                f.lastKnown={std::round(e.noisePosition.x/3)*3,std::round(e.noisePosition.z/3)*3};
                f.known=true;f.memory=6;
            }
        }
    }
    Observation observe(int i) const {
        const auto& f=fighters[i];float d=f.known?distance(f.pos,f.lastKnown):40;
        const auto& w=f.guns[f.selected];
        return {visible(i),f.known,f.known&&coverAt(f.pos,f.lastKnown),f.hp<=40,
                w.ammo<=int(w.spec.capacity*.2f),d<9?0:d<18?1:2,f.selected};
    }
    Vec2 hidingPlace(const Fighter& f) const {
        if(!f.known)return f.pos;
        Vec2 best=f.pos;float score=1e9;
        for(const auto& b:obstacles){
            std::array<Vec2,4> candidates{{{b.x-b.w/2-.8f,b.z},{b.x+b.w/2+.8f,b.z},
                                         {b.x,b.z-b.d/2-.8f},{b.x,b.z+b.d/2+.8f}}};
            for(auto p:candidates){
                if(circleBlocked(p,.36f,obstacles)||!coverAt(p,f.lastKnown))continue;
                float cost=distance(f.pos,p);
                if(cost<score){score=cost;best=p;}
            }
        }
        return best;
    }
    void emit(int i,float radius){auto& f=fighters[i];f.noisePosition=f.pos;f.noiseRadius=radius;++f.noiseSerial;}
    std::array<double,2> step(const std::array<int,2>& actions) {
        std::array<double,2> reward{{-.01,-.01}};
        if(winner!=-2)return {0,0};
        ++ticks;
        std::array<bool,2> oldCover{};
        for(int i=0;i<2;i++){
            auto& f=fighters[i];oldCover[i]=f.known&&coverAt(f.pos,f.lastKnown);
            f.memory=std::max(0.f,f.memory-DT);f.lastSeenAge+=DT;if(f.memory<=0)f.known=false;
            f.switchDelay=std::max(0.f,f.switchDelay-DT);f.navTimer-=DT;
            for(auto& w:f.guns)w.update(DT);
        }
        // Both sensors use the same pre-movement world; neither policy receives
        // the enemy's hidden live position, HP, weapon, goal or future action.
        sense(0);sense(1);
        for(int i=0;i<2;i++) {
            auto& f=fighters[i];int action=actions[i];
            if(action<0||action>=ACTIONS)throw std::runtime_error("Invalid action");
            if(action==Rifle||action==Sniper){
                int selected=action==Sniper?1:0;
                if(selected!=f.selected){f.selected=selected;f.switchDelay=.35f;stats[i].switches++;reward[i]-=.03;}
            }
            if(action==Reload){if(!f.guns[f.selected].reload())reward[i]-=.015;}
            bool moving=action==Hide||action==Chase||action==Left||action==Right||action==Search;
            Vec2 goal=f.pos;
            if(action==Hide)goal=hidingPlace(f);
            else if((action==Chase||action==Left||action==Right)&&f.known){
                goal=f.lastKnown;
                if(action==Left||action==Right){
                    float dx=goal.x-f.pos.x,dz=goal.z-f.pos.z,len=std::max(.01f,std::hypot(dx,dz));
                    float side=action==Left?-1.f:1.f;
                    goal={std::clamp(f.pos.x+side*dz/len*5,-28.f,28.f),std::clamp(f.pos.z-side*dx/len*5,-28.f,28.f)};
                }
            } else if(moving) {
                if(!f.goalValid||distance(f.pos,f.goal)<1){
                    // Exploration uses world coordinates, not hidden enemy data.
                    float angle=float((seed+i*7+ticks/50)%16)*2*PI/16;
                    f.goal={std::cos(angle)*8,std::sin(angle)*8};f.goalValid=true;
                }
                goal=f.goal;
            }
            if(moving&&distance(goal,f.pos)>.2f){
                if(!lineOfSight({f.pos.x,.35f,f.pos.z},{goal.x,.35f,goal.z},obstacles)){
                    if(f.navTimer<=0||distance(f.pos,f.waypoint)<.2f){f.waypoint=navigationWaypoint(f.pos,goal,obstacles);f.navTimer=.7f;}
                    goal=f.waypoint;
                }
                Vec2 before=f.pos;float d=distance(f.pos,goal);
                if(d>.001f){float pace=action==Hide?3.8f:3.2f;float amount=std::min(d,pace*DT);
                    moveActor(f.pos,(goal.x-f.pos.x)/d*amount,(goal.z-f.pos.z)/d*amount,obstacles,.35f);}
                float travel=distance(before,f.pos);stats[i].distance+=travel;f.moveDistance+=travel;
                if(f.moveDistance>1.4f){emit(i,action==Hide?8.f:15.f);f.moveDistance=0;}
                if(travel<.001f)reward[i]-=.015;
            }
            Vec2 aim=f.known?f.lastKnown:goal;float dx=aim.x-f.pos.x,dz=aim.z-f.pos.z;
            if(std::hypot(dx,dz)>.01f){float target=std::atan2(-dx,-dz);
                f.yaw=std::remainder(f.yaw+std::clamp(std::remainder(target-f.yaw,2*PI),-DT*3.5f,DT*3.5f),2*PI);}
        }
        std::array<int,2> damage{};
        for(int i=0;i<2;i++){
            auto& f=fighters[i];auto& stat=stats[i];const auto& e=fighters[1-i];
            bool seen=visible(i);if(seen)stat.visibleTicks++;else stat.hiddenTicks++;
            if(f.known&&coverAt(f.pos,f.lastKnown)&&!oldCover[i]){stat.coverEntries++;if(f.hp<=40)reward[i]+=.12;}
            if(actions[i]!=Shoot&&actions[i]!=Rifle&&actions[i]!=Sniper)continue;
            if(!seen){reward[i]-=.02;continue;}
            float dx=e.pos.x-f.pos.x,dz=e.pos.z-f.pos.z;
            float targetYaw=std::atan2(-dx,-dz);
            if(std::abs(std::remainder(targetYaw-f.yaw,2*PI))>.15f||f.switchDelay>0)continue;
            auto& w=f.guns[f.selected];if(!w.fire())continue;
            stat.shots++;emit(i,f.selected?48.f:38.f);
            float spread=f.selected?.009f:.035f;
            float perturb=(float(rng()%10001)/5000.f-1)*spread;
            Vec3 origin{f.pos.x,1.32f,f.pos.z};float range=std::max(.1f,std::hypot(dx,dz));
            float pitch=std::atan2(1.1f-origin.y,range);
            Vec3 ray=viewDirection(f.yaw+perturb,pitch);
            float hit=rayBox(origin,ray,{e.pos.x-.31f,.2f,e.pos.z-.25f},{e.pos.x+.31f,1.55f,e.pos.z+.25f});
            if(hit<obstacleRay(origin,ray,obstacles)&&hit<60){damage[1-i]+=w.spec.bodyDamage;stat.hits++;}
            else reward[i]-=.04;
        }
        for(int i=0;i<2;i++){
            int actual=std::min(fighters[i].hp,damage[i]);fighters[i].hp-=actual;
            stats[1-i].damage+=actual;reward[1-i]+=.04*actual;reward[i]-=.05*actual;
        }
        if(fighters[0].hp<=0||fighters[1].hp<=0){
            winner=fighters[0].hp==fighters[1].hp?-1:(fighters[0].hp>0?0:1);
        }else if(ticks>=900){winner=-1;for(int i=0;i<2;i++)reward[i]+=.02*(fighters[i].hp-fighters[1-i].hp);}
        if(winner>=0){reward[winner]+=12;reward[1-winner]-=12;}
        return reward;
    }
};
}
