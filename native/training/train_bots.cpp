#include "self_play.hpp"
#include <filesystem>
#include <iostream>
using namespace strike::training;
struct Outcome {int winner,ticks;std::array<Statistics,2> stats;};
Outcome play(unsigned seed,const Policy& policy,int seat,bool advanced){
    Duel duel(seed);
    while(duel.winner==-2){
        duel.sense(0);duel.sense(1);
        std::array<int,2> actions;
        for(int i=0;i<2;i++)actions[i]=i==seat?policy.greedy(duel.observe(i)):fixedAction(duel.observe(i),advanced);
        for(int frame=0;frame<5&&duel.winner==-2;frame++)duel.step(actions);
    }
    return {duel.winner,duel.ticks,duel.stats};
}
int main(int argc,char** argv){try{
    std::filesystem::path out=argc>1?argv[1]:"training-results";if(std::filesystem::exists(out)&&!std::filesystem::is_empty(out))throw std::runtime_error("Output directory must be empty");
    std::filesystem::create_directories(out);
    unsigned trainingBase=argc>2?unsigned(std::stoul(argv[2])):7000;
    unsigned evaluationBase=argc>3?unsigned(std::stoul(argv[3])):100000;
    if(trainingBase>10000000||evaluationBase>10000000||std::abs(long(trainingBase)-long(evaluationBase))<1000
       ||std::abs(long(trainingBase)-50000)<1000||std::abs(long(evaluationBase)-50000)<1000)
        throw std::runtime_error("Use separate bounded training/evaluation seed ranges");
    Policy initial=initialPolicy(),learned=initial,best=initial;int selectedRound=0;
    std::ofstream validation(out/"validation.csv");validation<<"training_round,score_wins_plus_half_draws,out_of\n";
    auto validate=[&](const Policy& p){
        double score=0;
        for(int n=0;n<20;n++)for(bool advanced:{false,true}){
            auto r=play(50000+n,p,n%2,advanced);
            score+=r.winner==n%2?1:r.winner==-1?.5:0;
        }
        return score;
    };
    double bestScore=validate(initial);validation<<0<<','<<bestScore<<",40\n";initial.save((out/"initial-policy.txt").string());
    std::mt19937 exploration(99117);
    std::ofstream rounds(out/"training-rounds.csv");
    rounds<<"round,seed,epsilon,winner,seconds,reward_a,reward_b,shots_a,shots_b,hits_a,hits_b,switches_a,switches_b,cover_entries_a,cover_entries_b\n";
    for(int round=0;round<100;round++){
        Duel duel(trainingBase+round);double epsilon=.45-.35*round/99.;std::array<double,2> total{};
        while(duel.winner==-2){
            duel.sense(0);duel.sense(1);
            std::array<int,2> states,actions;
            for(int i=0;i<2;i++){auto o=duel.observe(i);states[i]=o.index();actions[i]=learned.choose(o,epsilon,exploration);duel.stats[i].decisions[actions[i]]++;}
            std::array<double,2> reward{};
            for(int frame=0;frame<5&&duel.winner==-2;frame++){
                auto r=duel.step(actions);for(int i=0;i<2;i++)reward[i]+=r[i];
            }
            duel.sense(0);duel.sense(1);
            for(int i=0;i<2;i++){learned.learn(states[i],actions[i],reward[i],duel.observe(i).index(),duel.winner!=-2);total[i]+=reward[i];}
        }
        rounds<<round+1<<','<<trainingBase+round<<','<<epsilon<<','<<duel.winner<<','<<duel.ticks*DT<<','<<total[0]<<','<<total[1];
        for(auto getter:{0,1,2,3})for(int i=0;i<2;i++)rounds<<','<<(getter==0?duel.stats[i].shots:getter==1?duel.stats[i].hits:getter==2?duel.stats[i].switches:duel.stats[i].coverEntries);
        rounds<<'\n';
        if((round+1)%10==0){
            learned.save((out/("checkpoint-"+std::to_string(round+1)+".txt")).string());
            double score=validate(learned);validation<<round+1<<','<<score<<",40\n";
            if(score>bestScore){bestScore=score;best=learned;selectedRound=round+1;}
            std::cout<<"Training round "<<round+1<<"/100 complete; validation "<<score<<"/40\n"<<std::flush;
        }
    }
    learned.save((out/"final-policy.txt").string());
    best.save((out/"trained-policy.txt").string());
    if(Policy::load((out/"trained-policy.txt").string()).q!=best.q)throw std::runtime_error("Policy round-trip mismatch");
    std::ofstream visits(out/"policy-visits.csv");visits<<"state,action,updates,q_value\n";
    unsigned visited=0;for(int s=0;s<STATES;s++)for(int a=0;a<ACTIONS;a++){
        if(learned.visits[s][a])visited++;
        visits<<s<<','<<ACTION_NAMES[a]<<','<<learned.visits[s][a]<<','<<learned.q[s][a]<<'\n';
    }
    std::ofstream eval(out/"evaluation-rounds.csv");eval<<"opponent,seed,seat,policy,winner,seconds,shots,hits,damage,switches,cover_entries\n";
    std::ofstream summary(out/"evaluation-summary.csv");summary<<"opponent,policy,wins,draws,losses,win_rate,mean_damage,shots,hits,switches,cover_entries\n";
    for(bool advanced:{false,true})for(bool trained:{false,true}){
        int wins=0,draws=0,losses=0,damage=0,shots=0,hits=0,switches=0,cover=0;
        for(int n=0;n<100;n++){
            int seat=n%2;auto result=play(evaluationBase+n,trained?best:initial,seat,advanced);const auto& s=result.stats[seat];
            wins+=result.winner==seat;draws+=result.winner==-1;losses+=result.winner==1-seat;
            damage+=s.damage;shots+=s.shots;hits+=s.hits;switches+=s.switches;cover+=s.coverEntries;
            eval<<(advanced?"weapon_aware_defender":"rifle_chaser")<<','<<evaluationBase+n<<','<<seat<<','<<(trained?"trained":"initial")<<','<<result.winner<<','<<result.ticks*DT<<','<<s.shots<<','<<s.hits<<','<<s.damage<<','<<s.switches<<','<<s.coverEntries<<'\n';
        }
        summary<<(advanced?"weapon_aware_defender":"rifle_chaser")<<','<<(trained?"trained":"initial")<<','<<wins<<','<<draws<<','<<losses<<','<<wins/100.<<','<<damage/100.<<','<<shots<<','<<hits<<','<<switches<<','<<cover<<'\n';
        std::cout<<(advanced?"Defender":"Rifle")<<' '<<(trained?"trained":"initial")<<": "<<wins<<" wins, "<<draws<<" draws, "<<losses<<" losses\n"<<std::flush;
    }
    std::ofstream meta(out/"run-metadata.txt");meta<<"trainer=STRIKE_Q_V2\ntraining_rounds=100\ntraining_seeds="<<trainingBase<<".."<<trainingBase+99<<"\nevaluation_seeds="<<evaluationBase<<".."<<evaluationBase+99<<"\nevaluation_policy_updates=0\ntraining_rng_seed=99117\nseconds_per_tick=0.05\nticks_per_decision=5\nmax_round_seconds=45\nq_alpha=0.12\nq_gamma_per_decision=0.96\nepsilon_start=0.45\nepsilon_end=0.10\nselected_checkpoint_round="<<selectedRound<<"\nvalidation_seeds=50000..50019\nvalidation_updates=0\nvisited_state_action_pairs="<<visited<<"\nstate_action_pairs="<<STATES*ACTIONS<<"\n";
    std::cout<<"Saved policies, 100 training rounds and 400 held-out evaluation duels to "<<out<<'\n';
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
