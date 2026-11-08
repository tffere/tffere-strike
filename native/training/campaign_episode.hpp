#pragma once
#include "actual_game.hpp"
#include <array>
struct CampaignConfig {
    double playerAlphaScale=1;
    double alpha=.10,gamma=.995,damage=.10,taken=.08,timeCost=.001,switchCost=.20;
    double win=40,loss=25,timeout=35;
    int curriculum=0;
    unsigned trainSeedBase=960000,evalSeedBase=940000;
};
struct CampaignResult {
    int winner=-1,ticks=0;
    const char* endReason="ongoing";
    std::array<int,2> shots{},switches{},damage{},decisions{};
    std::array<double,2> reward{};
};
inline CampaignResult campaignEpisode(strike::tactics::Policy& botPolicy,strike::tactics::Policy& playerPolicy,
        unsigned seed,bool learning,double epsilon,std::array<std::mt19937,2>& rng,const CampaignConfig& config){
    using namespace strike;namespace ta=strike::tactics;
    Game game(true);game.useBotPolicy(botPolicy);game.start();game.bots[0]=game.bots[seed%6];
    for(int i=1;i<6;i++)game.bots[i].hp=0;
    game.kills=5;std::mt19937 positions(seed);
    game.player.pos.x=float(int(positions()%401)-200)/100.f;
    game.player.pos.z=26+float(int(positions()%101)-50)/100.f;
    if(learning&&config.curriculum&&seed%3==0){
        // Real-map encounter curriculum; physical rules and evaluation spawns stay unchanged.
        std::array<Vec2,3> playerStarts{{{0,15},{25,17},{-25,17}}};
        std::array<Vec2,3> botStarts{{{0,3},{25,3},{-25,3}}};int n=int(seed%3);
        if(config.curriculum==2)n=int((seed/3)%3);
        game.player.pos=playerStarts[n];game.bots[0].pos=game.bots[0].spawn=botStarts[n];
        // Curriculum begins at a visible encounter with both weapons ready.
        // Normal spawn timing and all final evaluation starts remain unchanged.
        game.bots[0].yaw=PI;game.bots[0].cooldown=0;
    }
    training::PlayerOpponent opponent(false);opponent.policy=&playerPolicy;CampaignResult result;
    while(game.mode==Mode::Playing){
        opponent.observe(game);
        std::array<ta::Observation,2> observed{{game.botObservation(0),opponent.observation(game)}};
        std::array<int,2> requested{};
        if(learning){
            requested[0]=botPolicy.chooseMasked(observed[0],game.botActionMask(0),epsilon,rng[0]);
            requested[1]=playerPolicy.chooseMasked(observed[1],opponent.actionMask(game),epsilon,rng[1]);
            game.bots[0].forcedAction=requested[0];game.bots[0].decisionRemaining=0;
            opponent.forcedAction=requested[1];opponent.decision=0;
        }
        std::array<double,2> reward{};std::array<int,2> executed=requested;
        std::array<int,2> states{{observed[0].index(),observed[1].index()}};
        for(int frame=0;frame<5&&game.mode==Mode::Playing;frame++){
            if(frame)opponent.observe(game);
            int hp=game.bots[0].hp,playerHP=game.player.health;
            auto botGun=game.bots[0].selected,playerGun=game.selectedWeapon;
            auto input=opponent.input(game);game.step(.05f,input);result.ticks++;
            if(frame==0){executed={game.bots[0].tacticalAction,opponent.action};states={game.bots[0].decisionState,opponent.decisionState};}
            std::array<int,2> dealt{{playerHP-game.player.health,hp-game.bots[0].hp}};
            std::array<int,2> changed{{int(botGun!=game.bots[0].selected),int(playerGun!=game.selectedWeapon)}};
            for(int i=0;i<2;i++){
                result.damage[i]+=dealt[i];result.switches[i]+=changed[i];
                reward[i]+=config.damage*dealt[i]-config.taken*dealt[1-i]-config.timeCost-config.switchCost*changed[i];
            }
            for(auto event:game.events){
                if(event.type==EventType::BotShot&&event.bot==0)result.shots[0]++;
                if(event.type==EventType::PlayerShot)result.shots[1]++;
            }
        }
        bool terminal=game.mode!=Mode::Playing;
        if(terminal){result.winner=game.mode==Mode::Draw?-1:game.mode==Mode::Won?1:game.player.health<=0?0:-1;
            result.endReason=game.mode==Mode::Draw?"mutual_elimination":result.winner==-1?"timeout":"elimination";
            for(int i=0;i<2;i++)reward[i]+=result.winner==-1?(game.mode==Mode::Draw?-5:-config.timeout):result.winner==i?config.win:-config.loss;}
        if(learning){
            auto next=game.botObservation(0);botPolicy.learnMasked(states[0],executed[0],reward[0],next.index(),terminal,game.botActionMask(0),config.alpha,config.gamma);
            auto playerNext=opponent.observation(game);playerPolicy.learnMasked(states[1],executed[1],reward[1],playerNext.index(),terminal,opponent.actionMask(game),config.alpha*config.playerAlphaScale,config.gamma);
        }
        for(int i=0;i<2;i++){result.reward[i]+=reward[i];result.decisions[i]++;}
    }
    return result;
}
