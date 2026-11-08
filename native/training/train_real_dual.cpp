#include "actual_game.hpp"
#include <filesystem>
#include <iostream>
namespace tr=strike::training;
namespace ta=strike::tactics;
struct Episode {
    int winner=-1,ticks=0;
    std::array<int,2> damage{},shots{},switches{};
    std::array<double,2> reward{};
};
Episode play(ta::Policy& botPolicy,ta::Policy& playerPolicy,unsigned seed,bool learning,
             double epsilon,std::array<std::mt19937,2>& random){
    strike::Game game(true);game.useBotPolicy(botPolicy);game.start();
    game.bots[0]=game.bots[seed%6];for(int i=1;i<6;i++)game.bots[i].hp=0;game.kills=5;
    std::mt19937 positions(seed);game.player.pos.x=float(int(positions()%401)-200)/100.f;
    game.player.pos.z=26+float(int(positions()%101)-50)/100.f;
    tr::PlayerOpponent opponent(false);opponent.policy=&playerPolicy;Episode result;
    while(game.mode==strike::Mode::Playing){
        opponent.observe(game);
        std::array<ta::Observation,2> observed{{game.botObservation(0),opponent.observation(game)}};
        std::array<int,2> actions{};
        if(learning){
            actions[0]=observed[0].known?botPolicy.choose(observed[0],epsilon,random[0]):ta::Search;
            actions[1]=observed[1].known?playerPolicy.choose(observed[1],epsilon,random[1]):ta::Search;
            game.bots[0].forcedAction=actions[0];game.bots[0].decisionRemaining=0;
            opponent.forcedAction=actions[1];opponent.decision=0;
        }
        std::array<double,2> reward{};
        for(int frame=0;frame<5&&game.mode==strike::Mode::Playing;frame++){
            if(frame>0)opponent.observe(game);
            int botHP=game.bots[0].hp,playerHP=game.player.health;
            auto botGun=game.bots[0].selected,playerGun=game.selectedWeapon;
            auto input=opponent.input(game);game.step(.05f,input);result.ticks++;
            int botDamage=playerHP-game.player.health,playerDamage=botHP-game.bots[0].hp;
            result.damage[0]+=botDamage;result.damage[1]+=playerDamage;
            result.switches[0]+=botGun!=game.bots[0].selected;result.switches[1]+=playerGun!=game.selectedWeapon;
            reward[0]+=.05*botDamage-.06*playerDamage-.003;
            reward[1]+=.05*playerDamage-.06*botDamage-.003;
            for(auto event:game.events){
                if(event.type==strike::EventType::BotShot&&event.bot==0)result.shots[0]++;
                if(event.type==strike::EventType::PlayerShot)result.shots[1]++;
            }
        }
        bool terminal=game.mode!=strike::Mode::Playing;
        if(terminal){
            result.winner=game.mode==strike::Mode::Won?1:game.player.health<=0?0:-1;
            for(int i=0;i<2;i++)reward[i]+=result.winner==-1?-5:result.winner==i?20:-20;
        }
        if(learning){
            botPolicy.learn(observed[0].index(),actions[0],reward[0],game.botObservation(0).index(),terminal);
            playerPolicy.learn(observed[1].index(),actions[1],reward[1],opponent.observation(game).index(),terminal);
        }
        for(int i=0;i<2;i++)result.reward[i]+=reward[i];
    }
    return result;
}
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
