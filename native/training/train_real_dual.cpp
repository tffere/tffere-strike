#include "real_dual_episode.hpp"
int main(int argc,char** argv){try{
    if(argc<3)throw std::runtime_error("Usage: train-real-dual STARTING_POLICY EMPTY_OUTPUT_DIR");
    auto starting=ta::Policy::load(argv[1]),bot=starting,player=starting;
    std::filesystem::path out=argv[2];
    if(std::filesystem::exists(out)&&!std::filesystem::is_empty(out))throw std::runtime_error("Output must be empty");
    std::filesystem::create_directories(out);
    bot.save((out/"before-bot-policy.txt").string());player.save((out/"before-player-policy.txt").string());
    std::array<std::mt19937,2> random{{std::mt19937(910031),std::mt19937(910037)}};
    std::ofstream rounds(out/"training-100.csv");
    rounds<<"round,seed,winner,seconds,bot_damage,player_damage,bot_shots,player_shots,bot_switches,player_switches,bot_reward,player_reward\n";
    for(int n=0;n<100;n++){
        auto r=play(bot,player,920000+n,true,.35-.25*n/99.,random);
        rounds<<n+1<<','<<920000+n<<','<<r.winner<<','<<r.ticks*.05f<<','<<r.damage[0]<<','<<r.damage[1]<<','<<r.shots[0]<<','<<r.shots[1]<<','<<r.switches[0]<<','<<r.switches[1]<<','<<r.reward[0]<<','<<r.reward[1]<<'\n';
        if((n+1)%10==0){
            bot.save((out/("bot-round-"+std::to_string(n+1)+".txt")).string());
            player.save((out/("player-round-"+std::to_string(n+1)+".txt")).string());
            std::cout<<"Real-game dual learning: "<<n+1<<"/100 complete\n"<<std::flush;
        }
    }
    bot.save((out/"after-bot-policy.txt").string());player.save((out/"after-player-policy.txt").string());
    auto frozenPlayer=ta::Policy::load((out/"before-player-policy.txt").string());
    auto savedBot=bot.q,savedPlayer=frozenPlayer.q;
    std::ofstream tests(out/"test-20.csv");tests<<"round,seed,winner,seconds,bot_damage,original_player_damage,bot_shots,original_player_shots\n";
    int wins=0,losses=0,timeouts=0;
    for(int n=0;n<20;n++){
        auto r=play(bot,frozenPlayer,930000+n,false,0,random);
        wins+=r.winner==0;losses+=r.winner==1;timeouts+=r.winner==-1;
        tests<<n+1<<','<<930000+n<<','<<r.winner<<','<<r.ticks*.05f<<','<<r.damage[0]<<','<<r.damage[1]<<','<<r.shots[0]<<','<<r.shots[1]<<'\n';
    }
    if(savedBot!=bot.q||savedPlayer!=frozenPlayer.q)throw std::runtime_error("Policy changed during frozen tests");
    if(bot.q==starting.q||player.q==starting.q)throw std::runtime_error("Both roles must actually update during training");
    std::ofstream summary(out/"summary.txt");
    summary<<"training_environment=real_strike_Game_step\ntraining_rounds=100\nseparate_learning_policies=2\ntraining_seeds=920000..920099\ntest_rounds=20\ntest_seeds=930000..930019\ntest_learning=disabled\ntest_opponent=before_player_policy\nbot_wins="<<wins<<"\nbot_losses="<<losses<<"\ntimeouts="<<timeouts<<"\nbot_win_rate="<<wins/20.<<"\npolicy_selection=final_round_100_no_test_selection\n";
    std::cout<<"20 frozen tests: "<<wins<<" bot wins, "<<losses<<" bot losses, "<<timeouts<<" timeouts. Win rate "<<wins*5<<"%\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
