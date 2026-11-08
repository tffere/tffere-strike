#include "actual_game.hpp"
#include <filesystem>
#include <iostream>
using namespace strike;
int main(int argc,char** argv){try{
    if(argc<3)throw std::runtime_error("Usage: train-actual INPUT_POLICY EMPTY_OUTPUT_DIR");
    auto initial=tactics::Policy::load(argv[1]),learned=initial,best=initial;
    std::filesystem::path out=argv[2];
    if(std::filesystem::exists(out)&&!std::filesystem::is_empty(out))throw std::runtime_error("Output must be empty");
    std::filesystem::create_directories(out);std::mt19937 exploration(81173);
    std::ofstream validation(out/"validation.csv");validation<<"round,wins,timeouts,losses,selection_score\n";
    auto validate=[&](const tactics::Policy& p,int round){
        int wins=0,timeouts=0,losses=0;
        for(int n=0;n<20;n++){
            auto r=training::actualEpisode(p,810000+n,n%2);
            wins+=r.winner==0;timeouts+=r.winner==-1;losses+=r.winner==1;
        }
        double score=wins-.25*timeouts;
        validation<<round<<','<<wins<<','<<timeouts<<','<<losses<<','<<score<<'\n';return score;
    };
    double bestScore=validate(initial,0);int bestRound=0;
    initial.save((out/"initial-policy.txt").string());
    std::ofstream rounds(out/"training-rounds.csv");rounds<<"round,seed,opponent,winner,seconds,damage,taken,shots,switches,cover_entries\n";
    for(int n=0;n<100;n++){
        auto r=training::actualEpisode(learned,800000+n,n%2,&learned,&exploration,.35-.25*n/99.);
        rounds<<n+1<<','<<800000+n<<','<<(n%2?"defender":"chaser")<<','<<r.winner<<','<<r.ticks*.05f<<','<<r.damage<<','<<r.taken<<','<<r.shots<<','<<r.switches<<','<<r.coverEntries<<'\n';
        if((n+1)%10==0){
            learned.save((out/("checkpoint-"+std::to_string(n+1)+".txt")).string());
            double score=validate(learned,n+1);
            if(score>bestScore){bestScore=score;best=learned;bestRound=n+1;}
            std::cout<<"Actual game training "<<n+1<<"/100; validation score "<<score<<"; selected "<<bestRound<<'\n'<<std::flush;
        }
    }
    learned.save((out/"final-policy.txt").string());best.save((out/"selected-policy.txt").string());
    std::ofstream eval(out/"evaluation-rounds.csv");eval<<"policy,seed,opponent,winner,seconds,damage,taken,shots,switches,cover_entries\n";
    std::ofstream summary(out/"evaluation-summary.csv");summary<<"policy,opponent,wins,timeouts,losses,mean_damage,mean_taken\n";
    for(bool trained:{false,true})for(bool defender:{false,true}){
        int wins=0,timeouts=0,losses=0,damage=0,taken=0;
        for(int n=0;n<50;n++){
            unsigned seed=820000+n*2+unsigned(defender);
            auto r=training::actualEpisode(trained?best:initial,seed,defender);
            wins+=r.winner==0;timeouts+=r.winner==-1;losses+=r.winner==1;damage+=r.damage;taken+=r.taken;
            eval<<(trained?"selected":"initial")<<','<<seed<<','<<(defender?"defender":"chaser")<<','<<r.winner<<','<<r.ticks*.05f<<','<<r.damage<<','<<r.taken<<','<<r.shots<<','<<r.switches<<','<<r.coverEntries<<'\n';
        }
        summary<<(trained?"selected":"initial")<<','<<(defender?"defender":"chaser")<<','<<wins<<','<<timeouts<<','<<losses<<','<<damage/50.<<','<<taken/50.<<'\n';
        std::cout<<(trained?"Selected":"Initial")<<' '<<(defender?"defender":"chaser")<<": "<<wins<<" wins, "<<timeouts<<" timeouts, "<<losses<<" losses / 50\n"<<std::flush;
    }
    std::ofstream metadata(out/"metadata.txt");metadata<<"gameplay=actual_strike_game_step\nmap=desert\nround_seconds=180\ntraining_rounds=100\ntraining_seeds=800000..800099\nvalidation_seeds=810000..810019\nevaluation_seeds=820000..820099\nselected_round="<<bestRound<<"\nlearner_role=game_bot\nopponent_role=ai_controlled_player\nlearning_during_evaluation=0\nselection=wins_minus_quarter_timeouts\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
