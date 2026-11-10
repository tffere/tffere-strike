#include "context_episode.hpp"
#include <filesystem>
#include <iostream>
void require(bool value,const char*msg){if(!value)throw std::runtime_error(msg);}
int main(){try{
    using namespace strike;using namespace strike::tactics;
    std::set<int> indices;
    for(int v=0;v<2;v++)for(int r=0;r<3;r++)for(int h=0;h<2;h++)for(int a=0;a<2;a++)for(int c=0;c<2;c++)for(int k=0;k<2;k++)for(int w=0;w<2;w++)for(int bits=0;bits<64;bits++)for(int region=0;region<4;region++){
        ContextObservation o{{bool(v),bool(k),bool(c),bool(h),bool(a),r,w},bool(bits&32),bool(bits&16),bool(bits&8),bool(bits&4),bool(bits&2),bool(bits&1),region};indices.insert(o.index());
    }
    require(indices.size()==CONTEXT_STATES&&*indices.begin()==0&&*indices.rbegin()==CONTEXT_STATES-1,"V3 encoder collision");
    auto old=Policy::load((std::filesystem::path(__FILE__).parent_path()/"../assets/bot-policy-trained.txt").string());auto v3=ContextPolicy::inherit(old);auto level0=old;auto original=old.q;
    for(unsigned seed=3900000;seed<3900020;seed++){
        auto a=contextEpisode(nullptr,&old,level0,seed,false,0,0,0);auto b=contextEpisode(&v3,nullptr,level0,seed,false,0,0,0);
        require(a.round.winner==b.round.winner&&a.round.ticks==b.round.ticks&&a.round.shots==b.round.shots&&a.round.damage==b.round.damage,"Inherited V3 changed baseline behavior");
    }
    require(v3.rows.empty(),"Frozen inference allocated learned rows");
    auto result=contextEpisode(&v3,nullptr,level0,3900123,true,.3,.12,.02);
    require(level0.q==original,"Frozen Level0 changed");require(result.round.reward[0]==(result.round.winner==0?1.:0.),"V3 reward is not win-only");require(!v3.rows.empty(),"V3 did not learn contexts");
    for(const auto& [state,row]:v3.rows)for(int a=0;a<CONTEXT_ACTIONS;a++)require(row.q[a]>=0&&row.q[a]<=1&&row.visits[a]<=1,"Invalid V3 probability/first-visit count");
    auto tmp=std::filesystem::temp_directory_path()/"strike-v3-test-policy.txt";v3.save(tmp.string());auto loaded=ContextPolicy::load(tmp.string());
    require(loaded.rows.size()==v3.rows.size()&&loaded.parent.q==v3.parent.q&&loaded.routeQ==v3.routeQ,"V3 roundtrip failed");for(const auto& [s,row]:v3.rows)require(loaded.rows.at(s).q==row.q&&loaded.rows.at(s).visits==row.visits,"V3 context counts not persisted");std::filesystem::remove(tmp);
    Game hiddenA(true);hiddenA.start();hiddenA.aggressiveSearch=true;hiddenA.useContextPolicy(v3);hiddenA.obstacles={{0,5,66,1,10}};for(auto&b:hiddenA.bots)b.hp=0;hiddenA.bots[0]=Bot{};
    Game hiddenB=hiddenA;hiddenA.player.pos={-20,20};hiddenB.player.pos={20,20};hiddenB.selectedWeapon=WeaponId::AW9;hiddenB.player.yaw=2;
    for(int i=0;i<100;i++){
        auto ma=hiddenA.contextActionMask(0),mb=hiddenB.contextActionMask(0);require(hiddenA.botContext(0,ma).index()==hiddenB.botContext(0,mb).index(),"V3 saw hidden target features");hiddenA.step(1.f/120);hiddenB.step(1.f/120);
    }
    require(distance(hiddenA.bots[0].pos,hiddenB.bots[0].pos)<.001f,"V3 route tracks hidden player");
    std::array<Vec2,3> goals;
    for(int a=EastRoute;a<CONTEXT_ACTIONS;a++){
        Game g(true);g.start();g.obstacles.clear();g.player.pos={0,20};for(auto& b:g.bots)b.hp=0;g.bots[0]=Bot{};g.bots[0].pos={-10,0};
        auto policy=ContextPolicy::inherit(initialPolicy());for(auto& row:policy.routeQ)row[a-EastRoute]=1;g.useContextPolicy(policy);g.step(1.f/120);
        require(g.bots[0].tacticalAction==a&&g.bots[0].routeRemaining>0,"New route option not executed");goals[a-EastRoute]=g.bots[0].routeGoal;
        require(!circleBlocked(goals[a-EastRoute],.4f,g.obstacles),"Route goal inside obstacle");
    }
    require(distance(goals[0],goals[1])>8&&distance(goals[0],goals[2])>8&&distance(goals[1],goals[2])>8,"New route actions are aliases");
    std::cout<<"V3 encoding49152x12,20-seed inheritance parity, first-visit/freeze, serialization, hidden-position and distinct route checks passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
