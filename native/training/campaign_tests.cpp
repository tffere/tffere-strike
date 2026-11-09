#include "campaign_metrics.hpp"
#include <iostream>
#include <sstream>
void require(bool b,const char* message){if(!b)throw std::runtime_error(message);}
int main(){try{
    using namespace strike;
    Game game(true);game.start();auto pool=campaignSpawnPool(game.obstacles);
    require(pool.size()>100,"Insufficient random spawn coverage");
    for(auto p:pool)require(!circleBlocked(p,.4f,game.obstacles),"Blocked spawn");
    Game hiddenA(true);hiddenA.start();hiddenA.aggressiveSearch=true;
    hiddenA.obstacles={{0,5,66,1,10}};for(auto& b:hiddenA.bots)b.hp=0;
    hiddenA.bots[0]=Bot{};hiddenA.bots[0].pos=hiddenA.bots[0].spawn={0,0};
    Game hiddenB=hiddenA;hiddenA.player.pos={-20,20};hiddenB.player.pos={20,20};
    for(int n=0;n<30;n++){hiddenA.step(.05f);hiddenB.step(.05f);}
    require(distance(hiddenA.bots[0].pos,hiddenB.bots[0].pos)<.001f&&distance(hiddenA.bots[0].interest,hiddenB.bots[0].interest)<.001f,"Broad search leaked hidden target");
    auto bot=tactics::initialPolicy(),player=bot,original=player;CampaignConfig c;
    c.freezePlayer=true;c.randomStarts=true;c.wideSearch=true;
    std::array<std::mt19937,2> rng{{std::mt19937(12),std::mt19937(13)}};
    auto r=campaignEpisode(bot,player,1100123,true,.2,rng,c);
    require(player.q==original.q&&player.visits==original.visits,"Frozen player learned");
    require(bot.visits!=original.visits,"Bot did not learn");
    require(distance(r.playerStart,r.botStart)>=8,"Overlapping spawns");
    require(!circleBlocked(r.playerStart,.4f,game.obstacles)&&!circleBlocked(r.botStart,.4f,game.obstacles),"Invalid episode spawn");
    require(r.botHits<=r.shots[0]&&r.botHits>=0,"Hits exceed shots");
    require(r.visibleWithoutShotSeconds<=r.visibleSeconds+.001&&r.readyVisibleWithoutShotSeconds<=r.visibleWithoutShotSeconds+.001,"Invalid sight counters");
    require(r.damage[0]>=0&&r.damage[0]<=100&&r.damage[1]>=0&&r.damage[1]<=100,"Damage included overkill");
    require((r.firstShotSeconds>=0)==(r.shots[0]>0),"First-shot censoring wrong");
    auto a=tactics::initialPolicy(),b=a;auto aq=a.q,bq=b.q;
    auto x=campaignEpisode(a,b,1200001,false,0,rng,c),y=campaignEpisode(a,b,1200001,false,0,rng,c);
    require(a.q==aq&&b.q==bq,"Frozen evaluation learned");
    require(x.winner==y.winner&&x.damage==y.damage&&x.shots==y.shots&&x.ticks==y.ticks&&x.botStart.x==y.botStart.x,"Seed replay differs");
    CampaignResult empty;empty.winner=1;empty.endReason="elimination";std::ostringstream out;blockRow(out,1,{empty});
    require(out.str().find("nan")==std::string::npos&&out.str().find("inf")==std::string::npos,"Undefined denominator emitted");
    std::cout<<"Campaign metrics, random spawns, freeze and replay checks passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
