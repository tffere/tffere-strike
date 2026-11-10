#pragma once
#include "campaign_episode.hpp"
#include <ostream>
#include <vector>
inline const char* metricHeader(){return ",bot_hits,bot_encounters,bot_visible_seconds,bot_visible_without_shot_seconds,bot_ready_visible_without_shot_seconds,bot_first_shot_seconds,bot_first_visible_seconds,bot_visited_4m_cells,bot_start_x,bot_start_z,bot_start_yaw,player_start_x,player_start_z,player_start_yaw,bot_rifle_kills,bot_sniper_kills,player_rifle_kills,player_sniper_kills";}
inline void metricRow(std::ostream& out,const CampaignResult& r){
    out<<','<<r.botHits<<','<<r.encounters<<','<<r.visibleSeconds<<','<<r.visibleWithoutShotSeconds<<','<<r.readyVisibleWithoutShotSeconds<<','<<r.firstShotSeconds<<','<<r.firstVisibleSeconds<<','<<r.visitedCells<<','<<r.botStart.x<<','<<r.botStart.z<<','<<r.botYaw<<','<<r.playerStart.x<<','<<r.playerStart.z<<','<<r.playerYaw<<','<<r.botKillsByWeapon[0]<<','<<r.botKillsByWeapon[1]<<','<<r.playerKillsByWeapon[0]<<','<<r.playerKillsByWeapon[1];
}
inline void blockHeader(std::ostream& out){out<<"first_round,last_round,games,wins,losses,mutual_eliminations,timeouts,mean_damage,mean_taken,mean_win_seconds,winning_games,shots,hits,hit_fraction,encounters,damage_per_encounter,visible_seconds,visible_without_shot_seconds,ready_visible_without_shot_seconds,mean_first_shot_seconds,rounds_with_shots,zero_shot_rounds,switches,mean_visited_4m_cells\n";}
inline void blockRow(std::ostream& out,int first,const std::vector<CampaignResult>& a){
    double damage=0,taken=0,winSeconds=0,firstSeconds=0,cells=0,visible=0,idle=0,readyIdle=0;
    int wins=0,losses=0,mutual=0,timeouts=0,shots=0,hits=0,encounters=0,withShots=0,switches=0;
    for(const auto& r:a){damage+=r.damage[0];taken+=r.damage[1];cells+=r.visitedCells;wins+=r.winner==0;losses+=r.winner==1;
        mutual+=std::string(r.endReason)=="mutual_elimination";timeouts+=std::string(r.endReason)=="timeout";
        if(r.winner==0)winSeconds+=r.ticks*r.stepSeconds;
        if(r.firstShotSeconds>=0){firstSeconds+=r.firstShotSeconds;withShots++;}
        shots+=r.shots[0];hits+=r.botHits;switches+=r.switches[0];encounters+=r.encounters;
        visible+=r.visibleSeconds;idle+=r.visibleWithoutShotSeconds;readyIdle+=r.readyVisibleWithoutShotSeconds;
    }
    out<<first<<','<<first+int(a.size())-1<<','<<a.size()<<','<<wins<<','<<losses<<','<<mutual<<','<<timeouts<<','<<damage/a.size()<<','<<taken/a.size()<<',';
    if(wins)out<<winSeconds/wins;
    out<<','<<wins<<','<<shots<<','<<hits<<',';if(shots)out<<double(hits)/shots;
    out<<','<<encounters<<',';if(encounters)out<<damage/encounters;
    out<<','<<visible<<','<<idle<<','<<readyIdle<<',';if(withShots)out<<firstSeconds/withShots;
    out<<','<<withShots<<','<<a.size()-withShots<<','<<switches<<','<<cells/a.size()<<'\n';
}
