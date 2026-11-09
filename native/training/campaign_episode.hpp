#pragma once
#include "actual_game.hpp"
#include <array>
#include <set>
#include <stdexcept>
struct CampaignConfig {
    bool freezePlayer=false,randomStarts=false,wideSearch=false;
    int evaluationGames=20;
    double explorationReward=0,mutualPenalty=5;
    double playerAlphaScale=1;
    double alpha=.10,gamma=.995,damage=.10,taken=.08,timeCost=.001,switchCost=.20;
    double win=40,loss=25,timeout=35;
    int curriculum=0;
    unsigned trainSeedBase=960000,evalSeedBase=940000;
};
// Integer navigation cells in the connected component of the normal player spawn.
inline std::vector<strike::Vec2> campaignSpawnPool(const std::vector<strike::Obstacle>& obstacles){
    constexpr int side=61;std::array<bool,side*side> seen{};std::vector<int> todo;
    auto point=[=](int i){return strike::Vec2{float(i%side-30),float(i/side-30)};};
    int start=56*side+30;
    if(strike::circleBlocked(point(start),.4f,obstacles))throw std::runtime_error("Spawn anchor blocked");
    todo.push_back(start);seen[start]=true;
    for(std::size_t n=0;n<todo.size();n++){
        int i=todo[n],x=i%side,z=i/side;
        for(auto d:std::array<std::pair<int,int>,4>{{{0,1},{1,0},{0,-1},{-1,0}}}){
            int xx=x+d.first,zz=z+d.second;if(xx<0||xx>=side||zz<0||zz>=side)continue;
            int j=zz*side+xx;if(!seen[j]&&!strike::circleBlocked(point(j),.4f,obstacles)){seen[j]=true;todo.push_back(j);}
        }
    }
    std::vector<strike::Vec2> pool;for(int i:todo)pool.push_back(point(i));return pool;
}
struct CampaignResult {
    int winner=-1,ticks=0;
    const char* endReason="ongoing";
    std::array<int,2> shots{},switches{},damage{},decisions{};
    std::array<double,2> reward{};
    std::array<int,2> botKillsByWeapon{},playerKillsByWeapon{};
    int botHits=0,encounters=0,visitedCells=0;
    double visibleSeconds=0,visibleWithoutShotSeconds=0,readyVisibleWithoutShotSeconds=0;
    double firstShotSeconds=-1,firstVisibleSeconds=-1;
    strike::Vec2 botStart{},playerStart{};float botYaw=0,playerYaw=0;

};
inline CampaignResult campaignEpisode(strike::tactics::Policy& botPolicy,strike::tactics::Policy& playerPolicy,
        unsigned seed,bool learning,double epsilon,std::array<std::mt19937,2>& rng,const CampaignConfig& config){
    using namespace strike;namespace ta=strike::tactics;
    Game game(true);game.aggressiveSearch=config.wideSearch;game.useBotPolicy(botPolicy);game.start();game.bots[0]=game.bots[seed%6];
    for(int i=1;i<6;i++)game.bots[i].hp=0;
    game.kills=5;std::mt19937 positions(seed);
    game.player.pos.x=float(int(positions()%401)-200)/100.f;
    game.player.pos.z=26+float(int(positions()%101)-50)/100.f;
    if(config.randomStarts){
        static const auto pool=campaignSpawnPool(game.obstacles);
        if(pool.size()<2)throw std::runtime_error("Insufficient connected spawn cells");
        game.player.pos=pool[positions()%pool.size()];
        do{game.bots[0].pos=pool[positions()%pool.size()];}while(distance(game.player.pos,game.bots[0].pos)<8);
        game.bots[0].spawn=game.bots[0].pos;
        game.player.yaw=float(positions()%100000)/100000.f*2*PI-PI;
        game.bots[0].yaw=float(positions()%100000)/100000.f*2*PI-PI;
    }
    if(learning&&config.curriculum&&seed%3==0){
        // Real-map encounter curriculum; physical rules and evaluation spawns stay unchanged.
        std::array<Vec2,3> playerStarts{{{0,15},{25,17},{-25,17}}};
        std::array<Vec2,3> botStarts{{{0,3},{25,3},{-25,3}}};int n=int(seed%3);
        if(config.curriculum==2)n=int((seed/3)%3);
        game.player.pos=playerStarts[n];game.bots[0].pos=game.bots[0].spawn=botStarts[n];
        // Curriculum begins at a visible encounter with both weapons ready.
        // Normal spawn timing and all final evaluation starts remain unchanged.
        game.bots[0].yaw=PI;game.bots[0].cooldown=0;
    }
    training::PlayerOpponent opponent(false);opponent.policy=&playerPolicy;CampaignResult result;
    result.botStart=game.bots[0].pos;result.playerStart=game.player.pos;
    result.botYaw=game.bots[0].yaw;result.playerYaw=game.player.yaw;
    bool previousVisible=false;std::set<std::pair<int,int>> cells;
    const auto frozenPlayer=playerPolicy.q;const auto frozenVisits=playerPolicy.visits;

    while(game.mode==Mode::Playing){
        opponent.observe(game);
        std::array<ta::Observation,2> observed{{game.botObservation(0),opponent.observation(game)}};
        std::array<int,2> requested{};
        if(learning){
            requested[0]=botPolicy.chooseMasked(observed[0],game.botActionMask(0),epsilon,rng[0]);
            if(!config.freezePlayer)requested[1]=playerPolicy.chooseMasked(observed[1],opponent.actionMask(game),epsilon,rng[1]);
            game.bots[0].forcedAction=requested[0];game.bots[0].decisionRemaining=0;
            if(!config.freezePlayer){opponent.forcedAction=requested[1];opponent.decision=0;}
        }
        std::array<double,2> reward{};std::array<int,2> executed=requested;
        std::array<int,2> states{{observed[0].index(),observed[1].index()}};
        for(int frame=0;frame<5&&game.mode==Mode::Playing;frame++){
            if(frame)opponent.observe(game);
            int hp=game.bots[0].hp,playerHP=game.player.health;
            auto botGun=game.bots[0].selected,playerGun=game.selectedWeapon;
            const bool visible=game.botObservation(0).visible;
            const auto& b=game.bots[0];const auto& gun=b.guns[static_cast<int>(b.selected)];
            const bool ready=b.cooldown<=0&&b.switchRemaining<=0&&gun.cooldown<=0&&gun.reloadRemaining<=0&&gun.ammo>0;
            if(visible&&!previousVisible)result.encounters++;
            if(visible&&result.firstVisibleSeconds<0)result.firstVisibleSeconds=result.ticks*.05;
            previousVisible=visible;
            auto input=opponent.input(game);game.step(.05f,input);result.ticks++;
            bool fired=false;
            for(auto e:game.events){
                if(e.type==EventType::BotShot&&e.bot==0){fired=true;if(result.firstShotSeconds<0)result.firstShotSeconds=result.ticks*.05;}
                if(e.type==EventType::PlayerHit&&e.bot==0){
                    result.botHits++;
                    if(playerHP>0&&game.player.health==0)result.botKillsByWeapon[static_cast<int>(e.weapon)]++;
                }
                if(e.type==EventType::BotHit&&e.bot==0&&hp>0&&game.bots[0].hp==0)
                    result.playerKillsByWeapon[static_cast<int>(e.weapon)]++;
            }
            if(visible){result.visibleSeconds+=.05;if(!fired){result.visibleWithoutShotSeconds+=.05;if(ready)result.readyVisibleWithoutShotSeconds+=.05;}}
            const auto cell=std::make_pair(int(std::floor(game.bots[0].pos.x/4)),int(std::floor(game.bots[0].pos.z/4)));
            const bool newCell=cells.insert(cell).second;
            if(learning&&newCell&&!visible)reward[0]+=config.explorationReward;

            if(frame==0){executed={game.bots[0].tacticalAction,opponent.action};states={game.bots[0].decisionState,opponent.decisionState};}
            std::array<int,2> dealt{{playerHP-game.player.health,hp-game.bots[0].hp}};
            std::array<int,2> changed{{int(botGun!=game.bots[0].selected),int(playerGun!=game.selectedWeapon)}};
            for(int i=0;i<2;i++){
                result.damage[i]+=dealt[i];result.switches[i]+=changed[i];
                reward[i]+=config.damage*dealt[i]-config.taken*dealt[1-i]-config.timeCost-config.switchCost*changed[i];
            }
            for(auto event:game.events){
                if(event.type==EventType::BotShot&&event.bot==0)result.shots[0]++;
                if(event.type==EventType::PlayerShot)result.shots[1]++;
            }
        }
        bool terminal=game.mode!=Mode::Playing;
        if(terminal){result.winner=game.mode==Mode::Draw?-1:game.mode==Mode::Won?1:game.player.health<=0?0:-1;
            result.endReason=game.mode==Mode::Draw?"mutual_elimination":result.winner==-1?"timeout":"elimination";
            for(int i=0;i<2;i++)reward[i]+=result.winner==-1?(game.mode==Mode::Draw?-config.mutualPenalty:-config.timeout):result.winner==i?config.win:-config.loss;}
        if(learning){
            auto next=game.botObservation(0);botPolicy.learnMasked(states[0],executed[0],reward[0],next.index(),terminal,game.botActionMask(0),config.alpha,config.gamma);
            auto playerNext=opponent.observation(game);if(!config.freezePlayer)playerPolicy.learnMasked(states[1],executed[1],reward[1],playerNext.index(),terminal,opponent.actionMask(game),config.alpha*config.playerAlphaScale,config.gamma);
        }
        for(int i=0;i<2;i++){result.reward[i]+=reward[i];result.decisions[i]++;}
    }
    result.visitedCells=int(cells.size());
    if(config.freezePlayer&&(playerPolicy.q!=frozenPlayer||playerPolicy.visits!=frozenVisits))throw std::runtime_error("Frozen player mutated");
    return result;
}
