// Tactical benchmark v1. Fixed scenarios, not a human IQ measurement.
#include "gameplay.hpp"
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>
using namespace strike;
tactics::Policy benchmarkPolicy;bool useTrainedPolicy=false;
struct Result {std::string category,name;bool passed;};
Game arena(Vec2 player={0,-12}) {
    Game g;if(useTrainedPolicy)g.useBotPolicy(benchmarkPolicy);g.start();g.obstacles.clear();g.player.pos=player;
    for(auto& b:g.bots)b.hp=0;
    auto& b=g.bots[0];b=Bot{};b.cooldown=100;
    return g;
}
void advance(Game& g,float duration) {
    for(int i=0;i<int(duration/.02f);i++){g.player.health=100;g.step(.02f);}
}
bool shot(const Game& g){for(auto e:g.events)if(e.type==EventType::BotShot)return true;return false;}
int main(int argc,char** argv){
    if(argc>2){benchmarkPolicy=tactics::Policy::load(argv[2]);useTrainedPolicy=true;}
    std::vector<Result> results;
    auto record=[&](std::string category,std::string name,bool pass){results.push_back({category,name,pass});};
    {
        auto g=arena();g.step(.02f);
        record("Perception","Acquire a visible player",g.bots[0].awareness==Awareness::Chase);
        g=arena({0,12});g.bots[0].interest={0,-20};g.bots[0].patrolRemaining=100;g.step(.02f);
        record("Perception","Reject a silent player behind the bot",g.bots[0].awareness==Awareness::Patrol);
        g=arena();g.obstacles={{0,-6,66,1,10}};g.bots[0].cooldown=0;g.step(.02f);
        record("Perception","Do not acquire or shoot through a wall",g.bots[0].awareness==Awareness::Patrol&&!shot(g));
        auto a=arena({-20,20}),b=arena({20,20});a.obstacles=b.obstacles={{0,5,66,1,10}};
        advance(a,3);advance(b,3);
        record("Perception","Hidden player position cannot change patrol",distance(a.bots[0].pos,b.bots[0].pos)<.001f);
    }
    {
        auto g=arena({0,12});g.makeNoise(g.player.pos,38);g.step(.02f);
        record("Hearing","Investigate an audible shot behind the bot",g.bots[0].awareness==Awareness::Investigate);
        g=arena({0,29});g.makeNoise(g.player.pos,8);g.step(.02f);
        record("Hearing","Ignore footsteps beyond hearing range",g.bots[0].awareness==Awareness::Patrol);
        g=arena({0,24});g.obstacles={{0,5,66,1,10}};g.makeNoise(g.player.pos,38);g.step(.02f);
        record("Hearing","Walls attenuate distant shots",g.bots[0].awareness==Awareness::Patrol);
        g=arena({1,12});g.makeNoise(g.player.pos,38);g.step(.02f);Vec2 known=g.bots[0].interest;
        g.player.pos={-20,20};g.step(.02f);
        record("Hearing","Old sound does not track new hidden position",distance(known,g.bots[0].interest)<.001f);
    }
    {
        auto g=arena();g.step(.02f);Vec2 known=g.bots[0].interest;
        g.obstacles={{0,-6,66,1,10}};g.player.pos={20,-20};g.step(.02f);
        record("Search","Remember last sight rather than hidden live position",distance(known,g.bots[0].interest)<.001f);
        g.bots[0].memory=.01f;g.step(.02f);
        record("Search","Search when contact memory expires",g.bots[0].awareness==Awareness::Search);
        Vec2 before=g.bots[0].pos;advance(g,2);
        record("Search","Move during a local search",distance(before,g.bots[0].pos)>.5f);
        g.bots[0].searchRemaining=.01f;g.step(.02f);
        record("Search","Resume patrol after unsuccessful search",g.bots[0].awareness==Awareness::Patrol);
    }
    {
        for(bool desert:{false,true}) {
            Game g(desert);int reached=0;
            for(auto b:g.bots){
                Vec2 p=b.pos;
                for(int n=0;n<300&&distance(p,g.player.pos)>.7f;n++){
                    Vec2 next=navigationWaypoint(p,g.player.pos,g.obstacles);
                    if(distance(next,p)<.001f)break;
                    moveActor(p,next.x-p.x,next.z-p.z,g.obstacles,.36f);
                }
                if(distance(p,g.player.pos)<.7f)reached++;
            }
            record("Navigation",desert?"All desert spawns can reach an evidence location":"All depot spawns can reach an evidence location",reached==6);
        }
        auto g=arena({0,-12});g.obstacles={{0,-6,4,2,4}};g.makeNoise(g.player.pos,48);
        bool collision=false;for(int i=0;i<800;i++){g.player.health=100;g.step(.02f);collision|=circleBlocked(g.bots[0].pos,.35f,g.obstacles);}
        record("Navigation","Live AI routes around cover",distance(g.bots[0].pos,g.player.pos)<7);
        record("Navigation","Live route stays outside solid cover",!collision);
    }
    {
        // These are capability probes; failures are deliberately reported.
        auto g=arena({0,-5});g.bots[0].hp=10;g.bots[0].cooldown=100;
        float initial=distance(g.bots[0].pos,g.player.pos);advance(g,1);
        record("Tactics","Disengage when critically wounded",distance(g.bots[0].pos,g.player.pos)>initial+1);
        g=arena({0,-5});g.bots[0].cooldown=0;advance(g,2);
        record("Tactics","Strafe while exchanging fire",std::abs(g.bots[0].pos.x)>1);
        g=arena({0,-12});g.obstacles={{2,-3,1,4,3}};g.bots[0].hp=10;advance(g,2);
        record("Tactics","Seek cover under threat",!lineOfSight(botEye(g.bots[0]),{g.player.pos.x,g.player.y,g.player.pos.z},g.obstacles));
        g=arena({0,-20});g.bots[0].cooldown=0;g.step(.02f);
        record("Tactics","Enforce reaction time before first shot",!shot(g));
    }
    {
        auto g=arena({0,-12});g.bots[1]=g.bots[0];advance(g,2);
        record("Coordination","Separate overlapping teammates",distance(g.bots[0].pos,g.bots[1].pos)>.7f);
        g=arena({0,-12});g.bots[1]=g.bots[0];g.obstacles={{0,-6,4,2,4}};g.makeNoise(g.player.pos,48);advance(g,2);
        record("Coordination","Choose distinct approaches to cover",distance(g.bots[0].pos,g.bots[1].pos)>2);
        g=arena({0,-8});g.bots[0].cooldown=0;g.bots[1]=Bot{};g.bots[1].pos={0,-3};g.bots[1].cooldown=100;
        g.step(.02f);bool blockedShot=false;for(auto e:g.events)if(e.type==EventType::BotShot&&e.bot==0)blockedShot=true;
        record("Coordination","Hold fire when teammate blocks the firing lane",!blockedShot);
        g=arena({0,-12});g.bots[1]=Bot{};g.bots[1].pos={0,12};g.bots[1].yaw=PI;g.step(.02f);
        record("Coordination","Communicate a sighting to a teammate",g.bots[1].awareness!=Awareness::Patrol);
    }
    std::map<std::string,std::pair<int,int>> groups;
    for(const auto& r:results){auto& group=groups[r.category];group.first+=r.passed;group.second++;}
    float total=0;
    std::cout<<"# Bot tactical benchmark v1\n\nBaseline: "<<(argc>1?argv[1]:"working tree")<<" gameplay; 50 Hz fixed step, deterministic seed 711.\n\n";
    std::cout<<"| Dimension | Passed | Score / 100 |\n|---|---:|---:|\n";
    for(auto [name,count]:groups){float score=100.f*count.first/count.second;total+=score;std::cout<<"| "<<name<<" | "<<count.first<<"/"<<count.second<<" | "<<std::fixed<<std::setprecision(1)<<score<<" |\n";}
    std::cout<<"\n**Overall: "<<std::fixed<<std::setprecision(1)<<total/groups.size()<<" / 100** (equal dimension weights).\n\n";
    std::cout<<"This is a project-specific scenario score, not human IQ, a difficulty rating, or visual-quality evaluation. Basic functionality and currently missing tactical capabilities are intentionally tested separately. Freeze these scenarios/thresholds when comparing versions.\n\n";
    for(const auto& r:results)std::cout<<"- "<<(r.passed?"PASS":"FAIL")<<" — "<<r.category<<": "<<r.name<<"\n";
    return 0; // A capability failure is a measured weakness, not a tool failure.
}
