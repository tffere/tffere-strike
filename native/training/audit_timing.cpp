#include "../gameplay.hpp"
#include <iostream>
int main(){using namespace strike;std::cout<<"dt,nominal_sample_mean_ms,mean_reaction_ready_ms,rifle_repeat_interval_ms\n";for(float dt:{.05f,1.f/120}){std::mt19937 rng(9291);double sampled=0,elapsed=0;for(int i=0;i<50000;i++){ReactionDelay r;r.update(true,dt,rng);sampled+=r.sampled;int steps=0;while(!r.ready()){r.update(true,dt,rng);steps++;}elapsed+=steps*double(dt);}WeaponState g;g.reset(WEAPON_SPECS[0]);g.fire();int steps=0;do{g.update(dt);steps++;}while(!g.fire());std::cout<<dt<<','<<sampled/50<<','<<elapsed/50<<','<<steps*dt*1000<<'\n';}}
