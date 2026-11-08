#include "self_play.hpp"
#include <iostream>
#include <set>
using namespace strike::training;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
    std::set<int> indices;
    for(int v=0;v<2;v++)for(int r=0;r<3;r++)for(int h=0;h<2;h++)for(int a=0;a<2;a++)for(int c=0;c<2;c++)for(int k=0;k<2;k++)for(int w=0;w<2;w++)
        indices.insert(Observation{bool(v),bool(k),bool(c),bool(h),bool(a),r,w}.index());
    check(indices.size()==STATES&&*indices.begin()==0&&*indices.rbegin()==STATES-1,"Observation encoding collision");
    Duel a(71),b(71);a.obstacles=b.obstacles={{0,4,66,1,10}};
    a.fighters[0].pos=b.fighters[0].pos={0,0};a.fighters[0].yaw=b.fighters[0].yaw=0;
    a.fighters[1].pos={-20,20};b.fighters[1].pos={20,20};
    for(int n=0;n<20;n++){a.step({Search,Shoot});b.step({Search,Shoot});}
    check(strike::distance(a.fighters[0].pos,b.fighters[0].pos)<.001f,"Hidden enemy changed exploration");
    check(a.observe(0).index()==b.observe(0).index(),"Hidden enemy leaked into observation");
    check(a.stats[1].shots==0&&b.stats[1].shots==0,"Shoot ignored visibility");
    Duel c(1);c.obstacles.clear();c.fighters[0].pos={0,0};c.fighters[0].yaw=0;c.fighters[1].pos={0,-5};c.fighters[1].yaw=strike::PI;
    c.fighters[0].selected=c.fighters[1].selected=1;
    c.step({Shoot,Shoot});check(c.winner==-1&&c.fighters[0].hp==0&&c.fighters[1].hp==0,"Damage not simultaneous");
    Duel d(1);d.obstacles={{0,-2.5f,4,1,4}};d.fighters[0].pos={0,0};d.fighters[0].yaw=0;d.fighters[1].pos={0,-5};
    d.step({Shoot,Search});check(d.stats[0].shots==0&&d.stats[0].damage==0,"Shot through cover");
    d.obstacles.clear();d.step({Sniper,Search});int ammo=d.fighters[0].guns[1].ammo;
    d.step({Shoot,Search});check(d.fighters[0].guns[1].ammo==ammo,"Switch delay ignored");
    d.fighters[0].guns[1].ammo=0;d.step({Reload,Search});
    for(int n=0;n<60;n++)d.step({Search,Search});
    check(d.fighters[0].guns[1].ammo>0,"Reload failed");
    Duel attack(1);attack.obstacles.clear();attack.fighters[0].pos={0,0};attack.fighters[0].yaw=0;attack.fighters[1].pos={0,-5};
    for(int n=0;n<20;n++)attack.step({Sniper,Search});
    check(attack.stats[0].shots>0,"Engage-sniper action stalled after switching");
    auto policy=initialPolicy();int state=Observation{}.index();double before=policy.q[state][Hide];
    policy.learn(state,Hide,2,state,true);check(policy.q[state][Hide]>before&&policy.visits[state][Hide]==1,"Q learning did not update");
    Duel e(8),f(8);for(int n=0;n<100;n++){e.step({Search,Search});f.step({Search,Search});}
    check(strike::distance(e.fighters[0].pos,f.fighters[0].pos)<.001f&&e.fighters[0].hp==f.fighters[0].hp,"Simulation not reproducible");
    std::cout<<"PASS: state encoding, sensory isolation, blocked shots, simultaneous damage, switch/reload, learning, determinism\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
