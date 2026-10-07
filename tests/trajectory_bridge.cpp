#include "trajectory.hpp"
extern "C" int flight(double x,double y,double z,double drop,double horizontal,int remove,double* out) {
    auto result=fc::solve({x,y,z},{drop,horizontal},remove);
    if (!result) return 0;
    out[0]=result->time; out[1]=result->vertical; out[2]=result->repaired;
    return 1;
}
