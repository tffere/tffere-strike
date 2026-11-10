#include "context_episode.hpp"
#include <filesystem>
#include <iostream>
using strike::tactics::ContextPolicy;using strike::tactics::Policy;
int main(int argc,char**argv){try{
    if(argc==4&&std::string(argv[1])=="init"){ContextPolicy::inherit(Policy::load(argv[2])).save(argv[3]);return 0;}
    if(argc<7||argc>8||(argc==8&&std::string(argv[7])!="test-only"))throw std::runtime_error("Usage: strike-context-trainer run|legacy POLICY LEVEL0 EMPTY_OUT CYCLE CONFIG [test-only]");
    bool legacy=std::string(argv[1])=="legacy",testOnly=legacy||argc>7;if(!legacy&&std::string(argv[1])!="run")throw std::runtime_error("Unknown mode");
    auto frozen=Policy::load(argv[3]);auto frozenQ=frozen.q;auto frozenVisits=frozen.visits;Policy old;ContextPolicy bot;
    if(legacy)old=Policy::load(argv[2]);else bot=ContextPolicy::load(argv[2]);
    std::filesystem::path out=argv[4];if(std::filesystem::exists(out)&&!std::filesystem::is_empty(out))throw std::runtime_error("Output must be empty");std::filesystem::create_directories(out);
    int cycle=std::stoi(argv[5]);ContextRunConfig cfg;std::ifstream config(argv[6]);if(!config)throw std::runtime_error("Missing config");std::string key;double value;
    while(config>>key){if(!(config>>value)||!std::isfinite(value))throw std::runtime_error("Invalid V3 config value");if(key=="games")cfg.games=int(value);else if(key=="evaluation_games")cfg.evaluationGames=int(value);else if(key=="training_seed_base")cfg.trainSeedBase=unsigned(value);else if(key=="evaluation_seed_base")cfg.evalSeedBase=unsigned(value);else if(key=="epsilon_start")cfg.epsilonStart=value;else if(key=="epsilon_end")cfg.epsilonEnd=value;else if(key=="alpha")cfg.alpha=value;else if(key=="parent_alpha")cfg.parentAlpha=value;else throw std::runtime_error("Unknown V3 config key");}
    if(cfg.games<=0||cfg.evaluationGames<=0||cfg.epsilonStart<0||cfg.epsilonStart>1||cfg.epsilonEnd<0||cfg.epsilonEnd>1||cfg.alpha<0||cfg.alpha>1||cfg.parentAlpha<0||cfg.parentAlpha>1)throw std::runtime_error("Invalid episode count");std::filesystem::copy_file(argv[6],out/"config.txt");
    frozen.save((out/"level0-before.txt").string());if(legacy)old.save((out/"before-bot.txt").string());else bot.save((out/"before-bot.txt").string());
    auto header=[](std::ostream& f){f<<"round,seed,winner,end_reason,seconds,bot_damage,player_damage,bot_shots,player_shots,bot_switches,player_switches,bot_reward,player_reward"<<metricHeader()<<",unique_contexts,route_decisions,route_choice_samples\n";};
    auto row=[](std::ostream& f,int n,unsigned seed,const FineResult& x){const auto&r=x.round;f<<n<<','<<seed<<','<<r.winner<<','<<r.endReason<<','<<r.ticks*r.stepSeconds<<','<<r.damage[0]<<','<<r.damage[1]<<','<<r.shots[0]<<','<<r.shots[1]<<','<<r.switches[0]<<','<<r.switches[1]<<','<<r.reward[0]<<','<<r.reward[1];metricRow(f,r);f<<','<<x.choiceContexts<<','<<x.routeDecisions<<','<<x.routeOptions<<'\n';};
    if(!testOnly){std::ofstream f(out/"training.csv");header(f);std::ofstream blocks(out/"training-blocks-10.csv");blockHeader(blocks);std::vector<CampaignResult> block;
        for(int n=0;n<cfg.games;n++){unsigned seed=cfg.trainSeedBase+cycle*cfg.games+n;double epsilon=cfg.epsilonStart+(cfg.epsilonEnd-cfg.epsilonStart)*n/std::max(1,cfg.games-1);
            auto result=contextEpisode(&bot,nullptr,frozen,seed,true,epsilon,cfg.alpha,cfg.parentAlpha);row(f,n+1,seed,result);block.push_back(result.round);
            if((n+1)%10==0){blockRow(blocks,n-8,block);block.clear();blocks.flush();std::cout<<"block"<<cycle<<' '<<n+1<<'/'<<cfg.games<<" sparse_rows="<<bot.rows.size()<<'\n'<<std::flush;}
        }
    }
    if(legacy)old.save((out/"after-bot.txt").string());else bot.save((out/"after-bot.txt").string());
    const auto beforeParent=bot.parent.q;const auto beforeRoutes=bot.routeQ;const auto beforeRows=bot.rows.size();
    std::ofstream eval(out/"evaluation.csv");header(eval);std::vector<CampaignResult> evaluated;
    for(int n=0;n<cfg.evaluationGames;n++){unsigned seed=cfg.evalSeedBase+n;auto result=contextEpisode(legacy?nullptr:&bot,legacy?&old:nullptr,frozen,seed,false,0,0,0);row(eval,n+1,seed,result);evaluated.push_back(result.round);}
    if(frozen.q!=frozenQ||frozen.visits!=frozenVisits||bot.parent.q!=beforeParent||bot.routeQ!=beforeRoutes||bot.rows.size()!=beforeRows)throw std::runtime_error("Frozen evaluation changed policy");
    if(legacy)old.save((out/"after-evaluation-bot.txt").string());else bot.save((out/"after-evaluation-bot.txt").string());
    {std::ifstream a(out/"after-bot.txt"),b(out/"after-evaluation-bot.txt");std::string x((std::istreambuf_iterator<char>(a)),{}),y((std::istreambuf_iterator<char>(b)),{});if(x!=y)throw std::runtime_error("Evaluation changed V3 contents");}
    frozen.save((out/"level0-after.txt").string());std::ofstream stats(out/"evaluation-metrics.csv");blockHeader(stats);blockRow(stats,1,evaluated);
    int wins=0;for(auto&r:evaluated)wins+=r.winner==0;std::cout<<"RESULT block"<<cycle<<": "<<wins<<'/'<<cfg.evaluationGames<<" wins; rows="<<bot.rows.size()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
