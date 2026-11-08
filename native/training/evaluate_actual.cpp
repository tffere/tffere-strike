#include "actual_game.hpp"
#include <filesystem>
#include <iostream>
int main(int argc,char** argv){try{
    if(argc<3)throw std::runtime_error("Usage: evaluate-actual POLICY EMPTY_OUTPUT_DIR");
    auto policy=strike::tactics::Policy::load(argv[1]);std::filesystem::path out=argv[2];
    if(std::filesystem::exists(out)&&!std::filesystem::is_empty(out))throw std::runtime_error("Output must be empty");
    std::filesystem::create_directories(out);
    std::ofstream summary(out/"summary.csv"),rounds(out/"rounds.csv");
    summary<<"policy,opponent,wins,timeouts,losses\n";
    rounds<<"policy,opponent,seed,winner,seconds,damage,taken\n";
    for(bool legacy:{true,false})for(bool defender:{false,true}){
        int w=0,t=0,l=0;
        for(int n=0;n<50;n++){
            unsigned seed=840000+n*2+unsigned(defender);
            auto r=strike::training::actualEpisode(policy,seed,defender,nullptr,nullptr,0,legacy);
            w+=r.winner==0;t+=r.winner==-1;l+=r.winner==1;
            rounds<<(legacy?"classic":"trained")<<','<<(defender?"defender":"chaser")<<','<<seed<<','<<r.winner<<','<<r.ticks*.05f<<','<<r.damage<<','<<r.taken<<'\n';
        }
        summary<<(legacy?"classic":"trained")<<','<<(defender?"defender":"chaser")<<','<<w<<','<<t<<','<<l<<'\n';
        std::cout<<(legacy?"Classic":"Trained")<<' '<<(defender?"defender":"chaser")<<": "<<w<<"/50 wins\n";
    }
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
