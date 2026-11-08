#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <random>
#include <stdexcept>
#include <string>
namespace strike::tactics {
constexpr int ACTIONS=9, STATES=192;
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
using ActionMask=std::array<bool,ACTIONS>;
struct Readiness {
    int ammo=30,otherAmmo=5;
    float switching=0,commitment=0,reloading=0;
    bool coverAvailable=false,muzzleClear=true;
};
inline ActionMask legalActions(const Observation& o,const Readiness& r){
    ActionMask mask{};
    if(!o.known){mask[Search]=true;return mask;}
    mask[Chase]=mask[Left]=mask[Right]=true;
    mask[Hide]=r.coverAvailable;
    mask[Search]=!o.visible;
    if(r.switching>0){mask.fill(false);mask[o.weapon?Sniper:Rifle]=true;return mask;}
    if(r.reloading>0)return mask;
    mask[Reload]=o.lowAmmo;
    mask[Shoot]=o.visible&&r.ammo>0&&r.muzzleClear;
    mask[o.weapon?Sniper:Rifle]=mask[Shoot];
    bool usefulRange=o.weapon?o.range==0:o.range==2;
    bool empty=r.ammo==0&&r.otherAmmo>0;
    mask[o.weapon?Rifle:Sniper]=r.otherAmmo>0&&r.commitment<=0&&(usefulRange||empty);
    return mask;
}
struct Policy {
    std::array<std::array<double,ACTIONS>,STATES> q{};
    std::array<std::array<unsigned,ACTIONS>,STATES> visits{};
    int greedy(const Observation& o) const {
        const auto& row=q[o.index()];return int(std::max_element(row.begin(),row.end())-row.begin());
    }
    int choose(const Observation& o,double epsilon,std::mt19937& rng) const {
        return double(rng()%100000)/100000.<epsilon?int(rng()%ACTIONS):greedy(o);
    }
    int greedyMasked(const Observation& o,const ActionMask& allowed) const {
        int best=-1;for(int a=0;a<ACTIONS;a++)if(allowed[a]&&(best<0||q[o.index()][a]>q[o.index()][best]))best=a;
        return best<0?Search:best;
    }
    int chooseMasked(const Observation& o,const ActionMask& allowed,double epsilon,std::mt19937& rng) const {
        if(double(rng()%100000)/100000.>=epsilon)return greedyMasked(o,allowed);
        std::array<int,ACTIONS> options{};int count=0;
        for(int a=0;a<ACTIONS;a++)if(allowed[a])options[count++]=a;
        return count?options[rng()%count]:Search;
    }
    void learnMasked(int state,int action,double reward,int next,bool terminal,const ActionMask& allowed,
                     double alpha=.10,double gamma=.995){
        double future=0;bool found=false;
        if(!terminal)for(int a=0;a<ACTIONS;a++)if(allowed[a]&&(!found||q[next][a]>future)){future=q[next][a];found=true;}
        q[state][action]+=alpha*(reward+gamma*future-q[state][action]);visits[state][action]++;
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

}
