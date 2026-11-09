#include "../../../../native/tactical_policy.hpp"
int main(int argc,char**argv){using strike::tactics::Policy;auto base=Policy::load(argv[1]),learned=Policy::load(argv[2]);double weight=std::stod(argv[3]);for(int s=0;s<strike::tactics::STATES;s++)for(int a=0;a<strike::tactics::ACTIONS;a++)base.q[s][a]=(1-weight)*base.q[s][a]+weight*learned.q[s][a];base.save(argv[4]);}
