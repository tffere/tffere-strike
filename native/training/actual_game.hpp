#pragma once
#include "../gameplay.hpp"
namespace strike::training {
struct PlayerOpponent {
    bool advanced=false,known=false;Vec2 evidence{},waypoint{},explore{};
    float memory=0,decision=0,navigation=0,switchDelay=0,targetY=1.1f;
    int action=tactics::Search;unsigned searchStep=0;
    explicit PlayerOpponent(bool stronger):advanced(stronger){}
    bool sees(const Game& game) const {
        Bot eyes;eyes.pos=game.player.pos;eyes.yaw=game.player.yaw;eyes.feet=game.player.y-EYE_HEIGHT;
        const auto& b=game.bots[0];return botSees(eyes,botEye(b),game.obstacles);
    }
    void observe(const Game& game) {
        memory=std::max(0.f,memory-.05f);if(memory<=0)known=false;
        if(sees(game)){
            const auto& b=game.bots[0];evidence=b.pos;known=true;memory=6;targetY=b.feet+1.1f-.55f*b.crouch;
        }else for(const auto& event:game.events){
            float radius=event.type==EventType::BotShot?(event.weapon==WeaponId::AW9?48.f:38.f):
                         event.type==EventType::BotFootstep?8.f*event.value:0;
            if(radius<=0)continue;
            bool wall=!lineOfSight({game.player.pos.x,game.player.y,game.player.pos.z},
                {event.position.x,1,event.position.z},game.obstacles);
            if(distance(game.player.pos,event.position)<=radius*(wall?.45f:1.f)){
                evidence={std::round(event.position.x/3)*3,std::round(event.position.z/3)*3};known=true;memory=6;
            }
        }
    }
    Input input(Game& game) {
        bool visible=sees(game);const auto& p=game.player;
        float range=known?distance(p.pos,evidence):40;
        bool covered=known&&!lineOfSight({p.pos.x,p.y,p.pos.z},{evidence.x,1.7f,evidence.z},game.obstacles);
        const auto& gun=game.weapon();
        tactics::Observation o{visible,known,covered,p.health<=40,gun.ammo<=int(gun.spec.capacity*.2f),range<9?0:range<18?1:2,int(game.selectedWeapon)};
        decision-=.05f;navigation-=.05f;switchDelay=std::max(0.f,switchDelay-.05f);
        if(decision<=.0001f){action=tactics::fixedAction(o,advanced);decision=.25f;}
        Input in;
        if(action==tactics::Rifle&&game.selectedWeapon!=WeaponId::AR7){in.selectAR7=true;switchDelay=.35f;}
        if(action==tactics::Sniper&&game.selectedWeapon!=WeaponId::AW9){in.selectAW9=true;switchDelay=.35f;}
        in.reload=action==tactics::Reload;
        bool attack=action==tactics::Shoot||action==tactics::Rifle||action==tactics::Sniper;
        in.fire=attack&&visible&&switchDelay<=0;
        bool moving=action==tactics::Hide||action==tactics::Chase||action==tactics::Search;
        Vec2 goal=p.pos;
        if(action==tactics::Hide&&known){Bot proxy;proxy.pos=p.pos;proxy.searchCenter=evidence;goal=game.botCoverGoal(proxy);}
        else if(action==tactics::Chase&&known)goal=evidence;
        else if(moving){
            if(distance(p.pos,explore)<1||searchStep==0){
                float a=float(++searchStep)*2.39996f;explore={std::cos(a)*3,std::sin(a)*20};
                if(circleBlocked(explore,.4f,game.obstacles))explore={0,0};
            }
            goal=explore;
        }
        if(moving&&distance(p.pos,goal)>.6f){
            if(!lineOfSight({p.pos.x,.35f,p.pos.z},{goal.x,.35f,goal.z},game.obstacles)){
                if(navigation<=0||distance(p.pos,waypoint)<.2f){waypoint=navigationWaypoint(p.pos,goal,game.obstacles);navigation=.7f;}
                goal=waypoint;
            }
            float dx=goal.x-p.pos.x,dz=goal.z-p.pos.z,d=std::max(.01f,std::hypot(dx,dz));dx/=d;dz/=d;
            float forward=-std::sin(p.yaw)*dx-std::cos(p.yaw)*dz;
            float side=std::cos(p.yaw)*dx-std::sin(p.yaw)*dz;
            in.forward=forward>.25f;in.backward=forward<-.25f;in.right=side>.25f;in.left=side<-.25f;
            in.sprint=known&&range>12;
        }
        in.crouch=action==tactics::Hide&&distance(p.pos,goal)<1;
        Vec2 aim=known?evidence:goal;float dx=aim.x-p.pos.x,dz=aim.z-p.pos.z;
        if(std::hypot(dx,dz)>.01f){
            float yaw=std::atan2(-dx,-dz),pitch=std::atan2((visible?targetY:1.1f)-p.y,std::hypot(dx,dz));
            in.lookX=-std::clamp(std::remainder(yaw-p.yaw,2*PI),-.11f,.11f);
            in.lookY=std::clamp(pitch-p.pitch,-.11f,.11f);
        }
        return in;
    }
};
struct ActualResult {int winner=-1,ticks=0,damage=0,taken=0,shots=0,switches=0,coverEntries=0;};
inline ActualResult actualEpisode(const tactics::Policy& policy,unsigned seed,bool defender,
                                 tactics::Policy* learner=nullptr,std::mt19937* exploration=nullptr,double epsilon=0,bool legacy=false){
    Game game(true);if(!legacy)game.useBotPolicy(policy);game.start();
    const int spawn=int(seed%6);game.bots[0]=game.bots[spawn];
    for(int i=1;i<6;i++)game.bots[i].hp=0;
    game.kills=5;
    std::mt19937 rng(seed);game.player.pos.x=float(int(rng()%401)-200)/100.f;
    game.player.pos.z=26+float(int(rng()%101)-50)/100.f;
    PlayerOpponent opponent(defender);ActualResult result;
    while(game.mode==Mode::Playing){
        auto observation=game.botObservation(0);int state=observation.index();
        int action=observation.known?(learner?learner->choose(observation,epsilon,*exploration):policy.greedy(observation)):tactics::Search;
        // Frozen evaluations run the very same autonomous policy path as the
        // playable game. Only training injects exploratory decisions.
        if(learner){game.bots[0].forcedAction=action;game.bots[0].decisionRemaining=0;}
        double reward=0;bool oldCover=observation.covered;
        for(int frame=0;frame<5&&game.mode==Mode::Playing;frame++){
            opponent.observe(game);auto input=opponent.input(game);
            int hp=game.bots[0].hp,playerHP=game.player.health;auto selected=game.bots[0].selected;
            game.step(.05f,input);result.ticks++;
            int dealt=playerHP-game.player.health,taken=hp-game.bots[0].hp;
            result.damage+=dealt;result.taken+=taken;result.switches+=selected!=game.bots[0].selected;
            reward+=.05*dealt-.06*taken-.003;
            for(auto event:game.events)if(event.type==EventType::BotShot&&event.bot==0)result.shots++;
        }
        auto next=game.botObservation(0);
        if(next.covered&&!oldCover){result.coverEntries++;if(next.lowHealth)reward+=.15;}
        if((action==tactics::Shoot||action==tactics::Rifle||action==tactics::Sniper)&&next.lowAmmo)reward-=.15;
        bool terminal=game.mode!=Mode::Playing;
        if(terminal){
            result.winner=game.mode==Mode::Won?1:game.player.health<=0?0:-1;
            reward+=result.winner==0?20:result.winner==1?-20:-5;
        }
        if(learner)learner->learn(state,action,reward,next.index(),terminal);
    }
    return result;
}
}
