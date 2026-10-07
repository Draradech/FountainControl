#include <cassert>
#include "blueprint.hpp"
int main() {
    using namespace fc::blueprint;
    std::string hash(64,'a');
    Companion data{4,{{0,{350,150}},{2,{0,0}},{3,{1000,-1}}}};
    auto text=encode(data,hash);
    auto decoded=decode(text,hash,4);
    assert(decoded && decoded->fountains.size()==3 && decoded->fountains[0].drop==350);
    assert(decoded->fountains[2].horizontal==0 && decoded->fountains[3].horizontal==-1);
    assert(!decode(text,std::string(64,'b'),4));
    assert(!decode(text,hash,5));
    assert(!decode(text+"junk",hash,4));
    assert(!decode(text.substr(0,text.size()-4),hash,4));
    assert(!decode(std::string(1048577,'x'),hash,4));
    for (auto row:{"-1 350 150", "4 350 150", "0 351 150", "0 1050 150", "0 -50 150",
                   "0 350 550", "0 350 -2", "0 350 151", "0 nan 150", "0 350 inf"}) {
        auto invalid="FountainControlBlueprint 1\n"+hash+"\n4 1\n"+row+"\n";
        assert(!decode(invalid,hash,4));
    }
    assert(!decode("FountainControlBlueprint 1\n"+hash+"\n4 2\n0 350 150\n0 500 250\n",hash,4));
    assert(!decode("FountainControlBlueprint 2\n"+hash+"\n4 0\n",hash,4));
    assert(decode(encode(Companion{4,{}},hash),hash,4));
}
