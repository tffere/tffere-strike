#include "campaign_episode.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
using strike::tactics::Policy;
int main(int argc,char** argv){try{
    if(argc<7)throw std::runtime_error("Usage: train-campaign BOT PLAYER REFERENCE EMPTY_OUT CYCLE CONFIG [test-only]");
    Policy bot=Policy::load(argv[1]),player=Policy::load(argv[2]),reference=Policy::load(argv[3]);
    std::filesystem::path out=argv[4];int cycle=std::stoi(argv[5]);bool testOnly=argc>7;
    if(std::filesystem::exists(out)&&!std::filesystem::is_empty(out))throw std::runtime_error("Output must be empty");
    std::filesystem::create_directories(out);CampaignConfig config;
    std::ifstream cfg(argv[6]);if(!cfg)throw std::runtime_error("Missing config");
    std::string key;double value;double epsilonStart=.30,epsilonEnd=.12;
    while(cfg>>key>>value){
        if(key=="alpha")config.alpha=value;else if(key=="gamma")config.gamma=value;
        else if(key=="damage")config.damage=value;else if(key=="taken")config.taken=value;
        else if(key=="switch_cost")config.switchCost=value;else if(key=="timeout")config.timeout=value;
        else if(key=="epsilon_start")epsilonStart=value;else if(key=="epsilon_end")epsilonEnd=value;
        else if(key=="player_alpha_scale")config.playerAlphaScale=value;
        else if(key=="time_cost")config.timeCost=value;else if(key=="win_reward")config.win=value;
        else if(key=="training_seed_base")config.trainSeedBase=unsigned(value);
        else if(key=="evaluation_seed_base")config.evalSeedBase=unsigned(value);
        else if(key=="curriculum")config.curriculum=int(value);else throw std::runtime_error("Unknown config key");
    }
    std::filesystem::copy_file(argv[6],out/"config.txt");
    bot.save((out/"before-bot.txt").string());player.save((out/"before-player.txt").string());reference.save((out/"reference-player.txt").string());
    std::array<std::mt19937,2> rng{{std::mt19937(970001+cycle*101),std::mt19937(970003+cycle*103)}};
    if(!testOnly){
        std::ofstream rounds(out/"training-100.csv");rounds<<"round,seed,winner,seconds,bot_damage,player_damage,bot_shots,player_shots,bot_switches,player_switches,bot_reward,player_reward\n";
        for(int n=0;n<100;n++){
            unsigned seed=config.trainSeedBase+cycle*100+n;
            auto r=campaignEpisode(bot,player,seed,true,epsilonStart+(epsilonEnd-epsilonStart)*n/99.,rng,config);
            rounds<<n+1<<','<<seed<<','<<r.winner<<','<<r.ticks*.05f<<','<<r.damage[0]<<','<<r.damage[1]<<','<<r.shots[0]<<','<<r.shots[1]<<','<<r.switches[0]<<','<<r.switches[1]<<','<<r.reward[0]<<','<<r.reward[1]<<'\n';
            if((n+1)%10==0)std::cout<<"Cycle "<<cycle<<": "<<n+1<<"/100\n"<<std::flush;
        }
    }
    bot.save((out/"after-bot.txt").string());player.save((out/"after-player.txt").string());
    auto botBefore=bot.q,referenceBefore=reference.q;
    std::ofstream eval(out/"evaluation-20.csv");eval<<"round,seed,winner,seconds,bot_damage,player_damage,bot_shots,player_shots,bot_switches,player_switches,end_reason\n";
    int w=0,l=0,t=0,d=0,s=0,x=0,zero=0,timeouts=0,mutual=0;
    for(int n=0;n<20;n++){
        unsigned seed=(testOnly&&cycle==6?950000:config.evalSeedBase)+n;
        auto r=campaignEpisode(bot,reference,seed,false,0,rng,config);
        w+=r.winner==0;l+=r.winner==1;t+=r.winner==-1;timeouts+=std::string(r.endReason)=="timeout";mutual+=std::string(r.endReason)=="mutual_elimination";d+=r.damage[0];s+=r.shots[0];x+=r.switches[0];zero+=r.shots[0]==0;
        eval<<n+1<<','<<seed<<','<<r.winner<<','<<r.ticks*.05f<<','<<r.damage[0]<<','<<r.damage[1]<<','<<r.shots[0]<<','<<r.shots[1]<<','<<r.switches[0]<<','<<r.switches[1]<<','<<r.endReason<<'\n';
    }
    if(bot.q!=botBefore||reference.q!=referenceBefore)throw std::runtime_error("Evaluation mutated policy");
    std::ofstream summary(out/"summary.csv");summary<<"cycle,training_games,wins,losses,draws,win_rate,mean_damage,shots,switches,zero_shot_games,timeouts,mutual_eliminations\n"<<cycle<<','<<(testOnly?0:100)<<','<<w<<','<<l<<','<<t<<','<<w/20.<<','<<d/20.<<','<<s<<','<<x<<','<<zero<<','<<timeouts<<','<<mutual<<'\n';
    std::cout<<"RESULT cycle "<<cycle<<": "<<w<<" wins, "<<l<<" losses, "<<t<<" draws; "<<s<<" shots, "<<x<<" switches, damage "<<d/20.<<'\n';
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
