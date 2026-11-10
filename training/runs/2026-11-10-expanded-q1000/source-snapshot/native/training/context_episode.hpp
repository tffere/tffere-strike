#pragma once
#include "campaign_metrics.hpp"
struct ContextRunConfig {
    int games=100,evaluationGames=400;
    unsigned trainSeedBase=3700000,evalSeedBase=3800000;
    double epsilonStart=.25,epsilonEnd=.02,alpha=.12,parentAlpha=.02;
};
struct FineResult {CampaignResult round;int choiceContexts=0,routeDecisions=0,routeOptions=0;};
inline FineResult contextEpisode(strike::tactics::ContextPolicy* contextual,const strike::tactics::Policy* legacy,
    const strike::tactics::Policy& frozen,unsigned seed,bool learning,double epsilon,double alpha,double parentAlpha){
    using namespace strike;constexpr float dt=1.f/120;
    if(learning&&!contextual)throw std::runtime_error("Learning requires V3");
    Game g(true);g.aggressiveSearch=true;if(contextual)g.useContextPolicy(*contextual);else g.useBotPolicy(*legacy);
    g.start();g.bots[0]=g.bots[seed%6];for(int i=1;i<6;i++)g.bots[i].hp=0;g.kills=5;
    std::mt19937 positions(seed);positions();positions();static const auto pool=campaignSpawnPool(g.obstacles);
    g.player.pos=pool[positions()%pool.size()];do{g.bots[0].pos=pool[positions()%pool.size()];}while(distance(g.player.pos,g.bots[0].pos)<8);
    g.bots[0].spawn=g.bots[0].pos;g.player.yaw=float(positions()%100000)/100000.f*2*PI-PI;g.bots[0].yaw=float(positions()%100000)/100000.f*2*PI-PI;
    g.seedReaction(seed^0x75a13u);g.seedContext(seed^0x491dc7u);g.contextExploration=learning?epsilon:0;
    training::PlayerOpponent opponent(false);opponent.policy=&frozen;opponent.reactionRandom.seed(seed^0xc129bu);
    FineResult result;auto& r=result.round;r.stepSeconds=dt;r.botStart=g.bots[0].pos;r.playerStart=g.player.pos;r.botYaw=g.bots[0].yaw;r.playerYaw=g.player.yaw;
    std::set<std::pair<int,int>> visited;std::set<int> contexts;std::set<std::pair<int,int>> cells;bool previousSeen=false;
    while(g.mode==Mode::Playing){
        opponent.observe(g,dt);const bool seen=g.botObservation(0).visible;
        const auto& b=g.bots[0];const auto& gun=b.guns[static_cast<int>(b.selected)];
        bool ready=(!g.reactionDelays||b.reaction.ready())&&b.cooldown<=0&&b.switchRemaining<=0&&gun.cooldown<=0&&gun.reloadRemaining<=0&&gun.ammo>0;
        if(seen&&!previousSeen)r.encounters++;if(seen&&r.firstVisibleSeconds<0)r.firstVisibleSeconds=r.ticks*double(dt);previousSeen=seen;
        int hp=b.hp,health=g.player.health;auto oldGun=b.selected,playerGun=g.selectedWeapon;unsigned serial=b.decisionsTaken;
        auto input=opponent.input(g,dt);g.step(dt,input);r.ticks++;
        if(b.decisionsTaken!=serial){
            r.decisions[0]++;if(contextual){visited.insert({b.contextState,b.tacticalAction});contexts.insert(b.contextState);result.routeDecisions+=b.contextRouteChoice&&b.tacticalAction>=tactics::EastRoute;
                result.routeOptions+=b.contextRouteChoice;
            }
        }
        r.damage[0]+=health-g.player.health;r.damage[1]+=hp-b.hp;r.switches[0]+=oldGun!=b.selected;r.switches[1]+=playerGun!=g.selectedWeapon;
        bool fired=false;
        for(auto e:g.events){
            if(e.type==EventType::BotShot&&e.bot==0){r.shots[0]++;fired=true;if(r.firstShotSeconds<0)r.firstShotSeconds=r.ticks*double(dt);}
            if(e.type==EventType::PlayerShot)r.shots[1]++;
            if(e.type==EventType::PlayerHit&&e.bot==0){r.botHits++;if(health>0&&g.player.health==0)r.botKillsByWeapon[static_cast<int>(e.weapon)]++;}
            if(e.type==EventType::BotHit&&e.bot==0&&hp>0&&b.hp==0)r.playerKillsByWeapon[static_cast<int>(e.weapon)]++;
        }
        if(seen){r.visibleSeconds+=dt;if(!fired){r.visibleWithoutShotSeconds+=dt;if(ready)r.readyVisibleWithoutShotSeconds+=dt;}}
        cells.insert({int(std::floor(b.pos.x/4)),int(std::floor(b.pos.z/4))});
    }
    r.winner=g.mode==Mode::Draw?-1:g.mode==Mode::Won?1:g.player.health<=0?0:-1;
    r.endReason=g.mode==Mode::Draw?"mutual_elimination":r.winner==-1?"timeout":"elimination";
    r.reward[0]=r.winner==0?1.:0.;r.reward[1]=r.winner==1?1.:0.;r.visitedCells=int(cells.size());result.choiceContexts=int(contexts.size());
    if(learning)contextual->learnEpisode(visited,r.winner==0,alpha,parentAlpha);
    return result;
}
