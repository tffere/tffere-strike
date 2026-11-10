#pragma once
#include "tactical_policy.hpp"
#include <map>
#include <limits>
#include <set>
namespace strike::tactics {
constexpr int CONTEXTS=256, CONTEXT_STATES=STATES*CONTEXTS, CONTEXT_ACTIONS=ACTIONS+3;
enum ContextAction { EastRoute=ACTIONS, WestRoute=ACTIONS+1, CrossRoute=ACTIONS+2 };
using ContextMask=std::array<bool,CONTEXT_ACTIONS>;
struct ContextObservation {
    Observation basic{};
    bool aimed=false,ready=false,nearCover=false,fineRange=false,enemyFacing=false,enemySniper=false;
    int region=0;
    int index()const{
        int n=basic.index();for(bool bit:{aimed,ready,nearCover,fineRange,enemyFacing,enemySniper})n=n*2+int(bit);
        return n*4+region;
    }
};
struct ContextRow {std::array<double,CONTEXT_ACTIONS> q{};std::array<unsigned,CONTEXT_ACTIONS> visits{};};
struct ContextPolicy {
    Policy parent{};
    std::array<std::array<double,3>,STATES> routeQ{};
    std::array<std::array<unsigned,3>,STATES> routeVisits{};
    std::map<int,ContextRow> rows;
    double priorStrength=2;
    static ContextPolicy inherit(const Policy& p){
        ContextPolicy result;result.parent=p;
        for(int s=0;s<STATES;s++)for(auto& v:result.routeQ[s])v=p.q[s][Search];
        return result;
    }
    double prior(int state,int action)const{return action<ACTIONS?parent.q[state/CONTEXTS][action]:routeQ[state/CONTEXTS][action-ACTIONS];}
    double score(int state,int action)const{
        auto it=rows.find(state);if(it==rows.end()||it->second.visits[action]==0)return prior(state,action);
        const auto& row=it->second;double weight=row.visits[action]/(priorStrength+row.visits[action]);
        return (1-weight)*prior(state,action)+weight*row.q[action];
    }
    int greedy(int state,const ContextMask& mask)const{
        int best=-1;for(int a=0;a<CONTEXT_ACTIONS;a++)if(mask[a]&&(best<0||score(state,a)>score(state,best)))best=a;
        return best<0?Search:best;
    }
    int choose(int state,const ContextMask& mask,double epsilon,std::mt19937& rng)const{
        if(epsilon<=0||double(rng()%100000)/100000.>=epsilon)return greedy(state,mask);
        std::array<int,CONTEXT_ACTIONS> options{};int n=0;for(int a=0;a<CONTEXT_ACTIONS;a++)if(mask[a])options[n++]=a;
        return n?options[rng()%n]:Search;
    }
    void learnEpisode(const std::set<std::pair<int,int>>& visited,bool won,double alpha,double parentAlpha){
        if(alpha<0||alpha>1||parentAlpha<0||parentAlpha>1)throw std::runtime_error("Invalid learning rate");
        std::set<std::pair<int,int>> shared;double outcome=won?1.:0.;
        for(auto [state,action]:visited){
            if(state<0||state>=CONTEXT_STATES||action<0||action>=CONTEXT_ACTIONS)throw std::runtime_error("Invalid contextual update");
            auto it=rows.find(state);
            if(it==rows.end()){ContextRow row;for(int a=0;a<CONTEXT_ACTIONS;a++)row.q[a]=prior(state,a);it=rows.emplace(state,row).first;}
            auto& row=it->second;row.q[action]+=alpha*(outcome-row.q[action]);row.visits[action]++;
            shared.insert({state/CONTEXTS,action});
        }
        for(auto [state,action]:shared){
            auto& q=action<ACTIONS?parent.q[state][action]:routeQ[state][action-ACTIONS];q+=parentAlpha*(outcome-q);
            if(action<ACTIONS)parent.visits[state][action]++;else routeVisits[state][action-ACTIONS]++;
        }
    }
    void save(const std::string& path)const{
        std::ofstream out(path);if(!out)throw std::runtime_error("Cannot save contextual policy");
        out<<"STRIKE_Q_V3 "<<CONTEXT_STATES<<' '<<CONTEXT_ACTIONS<<' '<<rows.size()<<' '<<std::setprecision(17)<<priorStrength<<'\n';
        for(int s=0;s<STATES;s++){
            for(double v:parent.q[s])out<<v<<' ';for(auto v:parent.visits[s])out<<v<<' ';
            for(double v:routeQ[s])out<<v<<' ';for(auto v:routeVisits[s])out<<v<<' ';out<<'\n';
        }
        for(const auto& [state,row]:rows){out<<state<<' ';for(double v:row.q)out<<v<<' ';for(auto n:row.visits)out<<n<<' ';out<<'\n';}
        if(!out)throw std::runtime_error("Contextual policy write failed");
    }
    static ContextPolicy load(const std::string& path){
        std::ifstream in(path);std::string magic;int states=0,actions=0;std::size_t count=0;ContextPolicy p;
        if(!(in>>magic>>states>>actions>>count>>p.priorStrength)||magic!="STRIKE_Q_V3"||states!=CONTEXT_STATES||actions!=CONTEXT_ACTIONS||count>CONTEXT_STATES||!std::isfinite(p.priorStrength)||p.priorStrength<0)throw std::runtime_error("Invalid V3 header");
        auto value=[&](double& v){if(!(in>>v)||!std::isfinite(v)||v<0||v>1)throw std::runtime_error("Invalid V3 win value");};
        auto visits=[&](unsigned& v){unsigned long long n=0;if(!(in>>n)||n>std::numeric_limits<unsigned>::max())throw std::runtime_error("Invalid V3 visits");v=unsigned(n);};
        for(int s=0;s<STATES;s++){for(auto& v:p.parent.q[s])value(v);for(auto& v:p.parent.visits[s])visits(v);for(auto& v:p.routeQ[s])value(v);for(auto& v:p.routeVisits[s])visits(v);}
        for(std::size_t i=0;i<count;i++){int state=-1;ContextRow row;if(!(in>>state)||state<0||state>=CONTEXT_STATES||p.rows.count(state))throw std::runtime_error("Invalid V3 state");for(auto& v:row.q)value(v);for(auto& v:row.visits)visits(v);p.rows.emplace(state,row);}
        std::string extra;if(in>>extra)throw std::runtime_error("Trailing V3 data");return p;
    }
};
}
