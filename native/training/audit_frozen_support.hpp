#pragma once
#include "campaign_episode.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
using namespace strike;
struct Start {Vec2 player,bot;float pyaw=0,byaw=0;};
Start startFor(unsigned seed,int band,const std::vector<Vec2>& pool){
    std::mt19937 rng(seed);Start s;
    // Consume historical player jitter draws before random connected starts.
    rng();rng();s.player=pool[rng()%pool.size()];
    if(band==0){do{s.bot=pool[rng()%pool.size()];}while(distance(s.player,s.bot)<8);}
    else{
        const float lo=band==1?8:band==2?20:40,hi=band==1?12:band==2?30:80;
        int tries=0;
        do{s.player=pool[rng()%pool.size()];s.bot=pool[rng()%pool.size()];if(++tries>100000)throw std::runtime_error("No band spawn found");}
        while(distance(s.player,s.bot)<lo||distance(s.player,s.bot)>=hi);
    }
    s.pyaw=float(rng()%100000)/100000.f*2*PI-PI;s.byaw=float(rng()%100000)/100000.f*2*PI-PI;return s;
}
struct Result {int winner=-1,shots=0,playerShots=0,damage=0,taken=0,ticks=0;double firstBotSight=-1,firstPlayerSight=-1,firstBotShot=-1,firstBotSightDistance=-1,firstBotShotDistance=-1;int botDecisions=0,searchOnly=0;bool initialBotSight=false,initialPlayerSight=false;const char* reason="timeout";};
Result play(const tactics::Policy& botPolicy,const tactics::Policy& playerPolicy,unsigned seed,Start s,float dt){
    Game g(true);g.aggressiveSearch=true;g.useBotPolicy(botPolicy);g.start();g.bots[0]=g.bots[seed%6];for(int i=1;i<6;i++)g.bots[i].hp=0;g.kills=5;
    g.player.pos=s.player;g.player.yaw=s.pyaw;g.bots[0].pos=g.bots[0].spawn=s.bot;g.bots[0].yaw=s.byaw;g.seedReaction(seed^0x75a13u);
    training::PlayerOpponent p(false);p.policy=&playerPolicy;p.reactionRandom.seed(seed^0xc129bu);Result r;
    r.initialBotSight=g.botObservation(0).visible;r.initialPlayerSight=p.sees(g);
    while(g.mode==Mode::Playing){
        p.observe(g,dt);bool seen=g.botObservation(0).visible;
        if(seen&&r.firstBotSight<0){r.firstBotSight=r.ticks*double(dt);r.firstBotSightDistance=distance(g.player.pos,g.bots[0].pos);}
        if(p.sees(g)&&r.firstPlayerSight<0)r.firstPlayerSight=r.ticks*double(dt);
        auto in=p.input(g,dt);g.step(dt,in);r.ticks++;
        if(g.bots[0].decisionRemaining>=.2499f){r.botDecisions++;auto mask=g.botActionMask(0);int legal=0;for(bool x:mask)legal+=x;r.searchOnly+=legal==1&&mask[tactics::Search];}
        for(auto e:g.events){if(e.type==EventType::BotShot&&e.bot==0){r.shots++;if(r.firstBotShot<0){r.firstBotShot=r.ticks*double(dt);r.firstBotShotDistance=distance(g.player.pos,g.bots[0].pos);}}if(e.type==EventType::PlayerShot)r.playerShots++;}
    }
    r.winner=g.mode==Mode::Draw?-1:g.mode==Mode::Won?1:g.player.health<=0?0:-1;
    r.reason=g.mode==Mode::Draw?"mutual_elimination":r.winner==-1?"timeout":"elimination";r.damage=100-g.player.health;r.taken=100-g.bots[0].hp;return r;
}
