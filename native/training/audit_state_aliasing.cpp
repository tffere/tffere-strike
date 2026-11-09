#include "audit_frozen_support.hpp"
struct Counter {int winner=-1,executed=-1,state=-1,damage=0,taken=0;};
Counter counterfactual(Game g,training::PlayerOpponent player,int action){
    g.bots[0].forcedAction=action;g.bots[0].decisionRemaining=0;Counter r;int frame=0;
    while(g.mode==Mode::Playing){
        if(frame==5)g.bots[0].forcedAction=-1;
        player.observe(g);auto input=player.input(g);g.step(.05f,input);
        if(frame==0){r.executed=g.bots[0].tacticalAction;r.state=g.bots[0].decisionState;}
        frame++;
    }
    r.winner=g.mode==Mode::Draw?-1:g.mode==Mode::Won?1:g.player.health<=0?0:-1;r.damage=100-g.player.health;r.taken=100-g.bots[0].hp;return r;
}
int main(int argc,char** argv){try{
    if(argc!=4)throw std::runtime_error("audit-state-aliasing BOT LEVEL0 OUTPUT");
    auto bot=tactics::Policy::load(argv[1]),level0=tactics::Policy::load(argv[2]);Game map(true);map.start();auto pool=campaignSpawnPool(map.obstacles);
    std::ofstream out(argv[3]);out<<"probe,seed,state,action,executed,winner,damage,taken,player_x,player_z,bot_x,bot_z,range,bot_reaction_remaining,player_reaction_remaining\n";
    const int target=tactics::Observation{true,true,false,false,false,1,0}.index();int probes=0;
    for(unsigned seed=3500000;seed<3502000&&probes<100;seed++){
        auto s=startFor(seed,0,pool);Game g(true);g.aggressiveSearch=true;g.useBotPolicy(bot);g.start();g.bots[0]=g.bots[seed%6];for(int i=1;i<6;i++)g.bots[i].hp=0;g.kills=5;
        g.player.pos=s.player;g.player.yaw=s.pyaw;g.bots[0].pos=g.bots[0].spawn=s.bot;g.bots[0].yaw=s.byaw;g.seedReaction(seed^0x75a13u);
        training::PlayerOpponent p(false);p.policy=&level0;p.reactionRandom.seed(seed^0xc129bu);
        while(g.mode==Mode::Playing){
            auto allowed=g.botActionMask(0);
            if(g.botObservation(0).index()==target&&allowed[tactics::Shoot]&&allowed[tactics::Chase]&&allowed[tactics::Left]&&allowed[tactics::Right]){
                std::array<int,4> actions{{tactics::Shoot,tactics::Chase,tactics::Left,tactics::Right}};std::array<Counter,4> results;bool valid=true;
                for(int k=0;k<4;k++){results[k]=counterfactual(g,p,actions[k]);valid=valid&&results[k].executed==actions[k]&&results[k].state==target;}
                if(valid){probes++;for(int k=0;k<4;k++){auto r=results[k];out<<probes<<','<<seed<<','<<target<<','<<tactics::ACTION_NAMES[actions[k]]<<','<<r.executed<<','<<r.winner<<','<<r.damage<<','<<r.taken<<','<<g.player.pos.x<<','<<g.player.pos.z<<','<<g.bots[0].pos.x<<','<<g.bots[0].pos.z<<','<<distance(g.player.pos,g.bots[0].pos)<<','<<g.bots[0].reaction.remaining<<','<<p.reaction.remaining<<'\n';}break;}
            }
            p.observe(g);auto input=p.input(g);g.step(.05f,input);
        }
    }
    std::cout<<probes<<" frozen same-encoded-state snapshots,4 legal one-decision counterfactuals each; no learning\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
