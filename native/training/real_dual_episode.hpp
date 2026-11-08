#pragma once
#include "actual_game.hpp"
#include <filesystem>
#include <iostream>
namespace tr=strike::training;
namespace ta=strike::tactics;
struct Episode {
    int winner=-1,ticks=0;
    std::array<int,2> damage{},shots{},switches{};
    std::array<double,2> reward{};
};
inline Episode play(ta::Policy& botPolicy,ta::Policy& playerPolicy,unsigned seed,bool learning,
             double epsilon,std::array<std::mt19937,2>& random){
    strike::Game game(true);game.useBotPolicy(botPolicy);game.start();
    game.bots[0]=game.bots[seed%6];for(int i=1;i<6;i++)game.bots[i].hp=0;game.kills=5;
    std::mt19937 positions(seed);game.player.pos.x=float(int(positions()%401)-200)/100.f;
    game.player.pos.z=26+float(int(positions()%101)-50)/100.f;
    tr::PlayerOpponent opponent(false);opponent.policy=&playerPolicy;Episode result;
    while(game.mode==strike::Mode::Playing){
        opponent.observe(game);
        std::array<ta::Observation,2> observed{{game.botObservation(0),opponent.observation(game)}};
        std::array<int,2> actions{};
        if(learning){
            actions[0]=observed[0].known?botPolicy.choose(observed[0],epsilon,random[0]):ta::Search;
            actions[1]=observed[1].known?playerPolicy.choose(observed[1],epsilon,random[1]):ta::Search;
            game.bots[0].forcedAction=actions[0];game.bots[0].decisionRemaining=0;
            opponent.forcedAction=actions[1];opponent.decision=0;
        }
        std::array<double,2> reward{};
        for(int frame=0;frame<5&&game.mode==strike::Mode::Playing;frame++){
            if(frame>0)opponent.observe(game);
            int botHP=game.bots[0].hp,playerHP=game.player.health;
            auto botGun=game.bots[0].selected,playerGun=game.selectedWeapon;
            auto input=opponent.input(game);game.step(.05f,input);result.ticks++;
            int botDamage=playerHP-game.player.health,playerDamage=botHP-game.bots[0].hp;
            result.damage[0]+=botDamage;result.damage[1]+=playerDamage;
            result.switches[0]+=botGun!=game.bots[0].selected;result.switches[1]+=playerGun!=game.selectedWeapon;
            reward[0]+=.05*botDamage-.06*playerDamage-.003;
            reward[1]+=.05*playerDamage-.06*botDamage-.003;
            for(auto event:game.events){
                if(event.type==strike::EventType::BotShot&&event.bot==0)result.shots[0]++;
                if(event.type==strike::EventType::PlayerShot)result.shots[1]++;
            }
        }
        bool terminal=game.mode!=strike::Mode::Playing;
        if(terminal){
            result.winner=game.mode==strike::Mode::Draw?-1:game.mode==strike::Mode::Won?1:game.player.health<=0?0:-1;
            for(int i=0;i<2;i++)reward[i]+=result.winner==-1?-5:result.winner==i?20:-20;
        }
        if(learning){
            botPolicy.learn(observed[0].index(),actions[0],reward[0],game.botObservation(0).index(),terminal);
            playerPolicy.learn(observed[1].index(),actions[1],reward[1],opponent.observation(game).index(),terminal);
        }
        for(int i=0;i<2;i++)result.reward[i]+=reward[i];
    }
    return result;
}
