#include "audit_frozen_support.hpp"
int main(int argc,char** argv){try{
    if(argc<5||argc>6||(argc==6&&std::string(argv[5])!="contact-only"))throw std::runtime_error("audit-frozen INCUMBENT CANDIDATE LEVEL0 EMPTY_OUT [contact-only]");
    auto incumbent=tactics::Policy::load(argv[1]),candidate=tactics::Policy::load(argv[2]),level0=tactics::Policy::load(argv[3]);auto iq=incumbent.q,cq=candidate.q,lq=level0.q;
    auto old=tactics::Policy::load("native/assets/bot-policy.txt");std::filesystem::path out=argv[4];if(std::filesystem::exists(out)&&!std::filesystem::is_empty(out))throw std::runtime_error("Output must be empty");std::filesystem::create_directories(out);
    Game map(true);map.start();auto pool=campaignSpawnPool(map.obstacles);
    // Golden parity: custom frozen loop must reproduce existing campaignEpisode exactly.
    std::array<std::mt19937,2> rng{{std::mt19937(1),std::mt19937(2)}};CampaignConfig cfg;cfg.randomStarts=cfg.wideSearch=cfg.freezePlayer=cfg.winOnly=cfg.episodicLearning=true;
    for(unsigned seed=3200000;seed<3200050;seed++){
        auto a=campaignEpisode(incumbent,level0,seed,false,0,rng,cfg);auto b=play(incumbent,level0,seed,startFor(seed,0,pool),.05f);
        if(a.winner!=b.winner||a.ticks!=b.ticks||a.damage[0]!=b.damage||a.damage[1]!=b.taken||a.shots[0]!=b.shots||a.shots[1]!=b.playerShots)throw std::runtime_error("Diagnostic harness parity failed");
    }
    std::ofstream(out/"parity.txt")<<"50 seeded frozen episodes match original campaign outcomes,ticks,damage and both shot counts exactly. No learning.\n";
    std::ofstream f(out/"probes.csv");f<<std::setprecision(9)<<"condition,policy,seed,dt,start_distance,player_x,player_z,bot_x,bot_z,player_yaw,bot_yaw,initial_bot_sight,initial_player_sight,winner,end_reason,seconds,bot_damage,bot_taken,bot_shots,player_shots,first_bot_sight_seconds,first_player_sight_seconds,first_bot_shot_seconds,bot_decisions,search_only_decisions,first_bot_sight_distance,first_bot_shot_distance\n";
    auto batch=[&](const char* condition,const char* policy,const tactics::Policy& bot,const tactics::Policy& player,int band,int count,unsigned first,float dt,bool swap){
        int wins=0,loss=0,mutual=0,timeouts=0;
        for(int n=0;n<count;n++){
            unsigned seed=first+n;auto s=startFor(seed,band,pool);if(swap){std::swap(s.player,s.bot);std::swap(s.pyaw,s.byaw);}auto a=play(bot,player,seed,s,dt);
            wins+=a.winner==0;loss+=a.winner==1;mutual+=std::string(a.reason)=="mutual_elimination";timeouts+=std::string(a.reason)=="timeout";
            f<<condition<<','<<policy<<','<<seed<<','<<dt<<','<<distance(s.player,s.bot)<<','<<s.player.x<<','<<s.player.z<<','<<s.bot.x<<','<<s.bot.z<<','<<s.pyaw<<','<<s.byaw<<','<<a.initialBotSight<<','<<a.initialPlayerSight<<','<<a.winner<<','<<a.reason<<','<<a.ticks*double(dt)<<','<<a.damage<<','<<a.taken<<','<<a.shots<<','<<a.playerShots<<','<<a.firstBotSight<<','<<a.firstPlayerSight<<','<<a.firstBotShot<<','<<a.botDecisions<<','<<a.searchOnly<<','<<a.firstBotSightDistance<<','<<a.firstBotShotDistance<<'\n';
        }
        f.flush();std::cout<<condition<<' '<<policy<<": "<<wins<<"W "<<loss<<"L "<<mutual<<" mutual "<<timeouts<<" timeout /"<<count<<'\n'<<std::flush;
    };
    if(argc==6){
        batch("standard","v17",incumbent,level0,0,400,3300000,.05f,false);batch("standard","latest1000",candidate,level0,0,400,3300000,.05f,false);
        for(int band=1;band<=3;band++){
            const char* name=band==1?"near_8_12":band==2?"mid_20_30":"far_40_80";
            batch(name,"v17",incumbent,level0,band,200,3400000,.05f,false);batch(name,"latest1000",candidate,level0,band,200,3400000,.05f,false);
        }
        if(incumbent.q!=iq||candidate.q!=cq||level0.q!=lq)throw std::runtime_error("Contact probe changed policy");
        std::ofstream(out/"frozen-check.txt")<<"Contact-only replay, no learning, all policy tables unchanged.\n";return 0;
    }
    batch("standard","older_before_v17",old,level0,0,400,3300000,.05f,false);
    batch("standard","v17",incumbent,level0,0,400,3300000,.05f,false);
    batch("standard","latest1000",candidate,level0,0,400,3300000,.05f,false);
    batch("same_policy_roles","level0_both",level0,level0,0,400,3300000,.05f,false);
    batch("same_policy_roles_swapped_starts","level0_both",level0,level0,0,400,3300000,.05f,true);
    batch("same_policy_roles","v17_both",incumbent,incumbent,0,400,3300000,.05f,false);
    batch("same_policy_roles_swapped_starts","v17_both",incumbent,incumbent,0,400,3300000,.05f,true);
    batch("policies_swapped_roles","level0_bot_v17_player",level0,incumbent,0,400,3300000,.05f,false);
    for(int band=1;band<=3;band++){
        const char* name=band==1?"near_8_12":band==2?"mid_20_30":"far_40_80";
        batch(name,"v17",incumbent,level0,band,200,3400000,.05f,false);batch(name,"latest1000",candidate,level0,band,200,3400000,.05f,false);
    }
    batch("tick_120fps","v17",incumbent,level0,0,200,3300000,1.f/120,false);
    batch("tick_120fps","latest1000",candidate,level0,0,200,3300000,1.f/120,false);
    if(incumbent.q!=iq||candidate.q!=cq||level0.q!=lq)throw std::runtime_error("Audit changed policy values");
    std::ofstream(out/"frozen-check.txt")<<"All probe policy tables unchanged. Training rounds added:0.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
