#include "actual_game.hpp"
#include <fstream>
#include <iostream>
using namespace strike;
int main(int argc,char**argv){try{
    if(argc!=2)throw std::runtime_error("audit-symmetric-duel OUTPUT.csv");std::ofstream f(argv[1]);f<<"seed,dt,winner,mutual,seconds,bot_sampled_reaction,player_sampled_reaction\n";
    tactics::Policy policy;for(auto& row:policy.q)row[tactics::Shoot]=1;
    for(float dt:{.05f,1.f/120}){int wins=0,losses=0,mutual=0;
        for(unsigned seed=3600000;seed<3601000;seed++){
            // Open-space microprobe isolates engine timing, not map geometry.
            Game g(true);g.start();g.obstacles.clear();g.useBotPolicy(policy);for(auto& b:g.bots)b.hp=0;g.kills=5;
            g.player.pos={0,20};g.player.yaw=0;g.selectedWeapon=WeaponId::AW9;g.bots[0]=Bot{};g.bots[0].pos={0,0};g.bots[0].yaw=PI;g.bots[0].selected=WeaponId::AW9;g.seedReaction(seed^0x75a13u);
            training::PlayerOpponent p(false);p.policy=&policy;p.reactionRandom.seed(seed^0xc129bu);int ticks=0;
            while(g.mode==Mode::Playing){p.observe(g,dt);auto input=p.input(g,dt);g.step(dt,input);ticks++;}
            int winner=g.mode==Mode::Draw?-1:g.mode==Mode::Won?1:0;wins+=winner==0;losses+=winner==1;mutual+=winner==-1;
            f<<seed<<','<<dt<<','<<winner<<','<<(g.mode==Mode::Draw)<<','<<ticks*double(dt)<<','<<g.bots[0].reaction.sampled<<','<<p.reaction.sampled<<'\n';
        }
        std::cout<<dt<<": bot"<<wins<<" player"<<losses<<" mutual"<<mutual<<" /1000; fixed20m symmetric sniper duel, no learning\n";
    }
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
