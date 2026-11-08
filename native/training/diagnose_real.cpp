// Post-hoc diagnostics only: no Policy::learn calls and no model selection.
#include "real_dual_episode.hpp"
struct Probe {int switches=0,shots=0,damage=0;};
Probe weaponProbe(const ta::Policy& policy,float commitSeconds){
    strike::Game g;g.useBotPolicy(policy);g.start();g.obstacles.clear();g.player.pos={0,-8};
    for(auto& b:g.bots)b.hp=0;
    auto& b=g.bots[0];b=strike::Bot{};b.cooldown=0;Probe result;float commitment=0;
    for(int n=0;n<100;n++){
        // Counterfactual controller guard for this isolated probe only.
        b.forcedAction=commitment>0?(b.selected==strike::WeaponId::AW9?ta::Sniper:ta::Rifle):-1;
        commitment=std::max(0.f,commitment-.05f);
        auto old=b.selected;g.step(.05f);
        if(old!=b.selected){result.switches++;commitment=commitSeconds;}
        for(auto e:g.events)result.shots+=e.type==strike::EventType::BotShot;
    }
    result.damage=100-g.player.health;return result;
}
int main(int argc,char** argv){try{
    if(argc<3)throw std::runtime_error("Usage: diagnose-real SAVED_RUN_DIR EMPTY_OUTPUT_DIR");
    std::filesystem::path run=argv[1],out=argv[2];
    if(std::filesystem::exists(out)&&!std::filesystem::is_empty(out))throw std::runtime_error("Output must be empty");
    std::filesystem::create_directories(out);
    auto originalPlayer=ta::Policy::load((run/"before-player-policy.txt").string());
    auto savedPlayer=originalPlayer.q;
    std::array<std::mt19937,2> rng{{std::mt19937(910031),std::mt19937(910037)}};
    std::ofstream checkpoints(out/"checkpoint-audit.csv");
    checkpoints<<"checkpoint,wins,losses,timeouts,mean_damage,shots,switches\n";
    for(int n=0;n<=100;n+=10){
        auto policy=ta::Policy::load((run/(n==0?"before-bot-policy.txt":"bot-round-"+std::to_string(n)+".txt")).string());
        auto saved=policy.q;int w=0,l=0,t=0,d=0,s=0,x=0;
        for(int j=0;j<20;j++){
            auto r=play(policy,originalPlayer,930000+j,false,0,rng);
            w+=r.winner==0;l+=r.winner==1;t+=r.winner==-1;d+=r.damage[0];s+=r.shots[0];x+=r.switches[0];
        }
        if(saved!=policy.q||savedPlayer!=originalPlayer.q)throw std::runtime_error("Diagnostic mutated policy");
        checkpoints<<n<<','<<w<<','<<l<<','<<t<<','<<d/20.<<','<<s<<','<<x<<'\n';
    }
    auto before=ta::Policy::load((run/"before-bot-policy.txt").string());
    auto after=ta::Policy::load((run/"after-bot-policy.txt").string());
    std::ofstream probes(out/"weapon-probes.csv");probes<<"policy,guard_seconds,switches,shots,damage,simulated_seconds\n";
    for(auto sample:{std::make_pair("before",0.f),std::make_pair("after",0.f),std::make_pair("after",2.f)}){
        auto r=weaponProbe(std::string(sample.first)=="before"?before:after,sample.second);
        probes<<sample.first<<','<<sample.second<<','<<r.switches<<','<<r.shots<<','<<r.damage<<",5\n";
    }
    strike::Game g;g.start();for(auto& b:g.bots)b.hp=0;
    g.player.pos={0,-10};g.obstacles={{2.5f,0,1,4,3}};g.bots[0]=strike::Bot{};g.bots[0].cooldown=100;
    ta::Policy left;for(auto& row:left.q)row[ta::Left]=1;g.useBotPolicy(left);
    auto expected=strike::navigationWaypoint({0,0},{5,0},g.obstacles);g.step(.05f);
    std::ofstream route(out/"route-probe.txt");route<<"intended_goal=5,0\nexpected_waypoint="<<expected.x<<','<<expected.z
        <<"\nactual_waypoint="<<g.bots[0].navigationTarget.x<<','<<g.bots[0].navigationTarget.z<<'\n';
    std::ofstream meta(out/"scope.txt");meta<<"learning=disabled\nmodel_selection=none\ncheckpoint_cases=previously_used_930000..930019\ninterpretation=post_hoc_diagnostics_not_fresh_holdout\nweapon_guard=isolated_probe_only_not_game_patch\n";
    std::cout<<"Saved frozen-checkpoint, weapon-cycle and route diagnostics; no learning or game patch applied.\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
