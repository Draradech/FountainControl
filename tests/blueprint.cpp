#include <cassert>
#include "blueprint.hpp"
int main() {
    using namespace fc::blueprint;
    std::string hash(64,'a');
    Metadata data{4,{{0,{350,150}},{2,{0,0}},{3,{1000,-1}}}};
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
    assert(decode(encode(Metadata{4,{}},hash),hash,4));
    assert(crc32(std::span(reinterpret_cast<const uint8_t*>("123456789"),9))==0xcbf43926);
    // A minimal PNG envelope with an opaque native blueprint tail. Pixel data
    // is immaterial to chunk extraction; the tail must remain byte-for-byte.
    std::vector<uint8_t> png(png_signature.begin(),png_signature.end());
    append32(png,0);png.insert(png.end(),{'I','E','N','D'});append32(png,0xae426082);
    png.insert(png.end(),{1,2,3,4,0xd5,0xaf,0x42,0x50});
    auto parsed=extract(png);assert(parsed && !parsed->metadata && parsed->canonical==png);
    auto embedded=embed(*parsed,text);
    auto restored=extract(embedded);
    assert(restored && restored->metadata==text && restored->canonical==png);
    auto edited=embed(*restored,"replacement");auto reread=extract(edited);
    assert(reread && reread->metadata=="replacement" && reread->canonical==png);
    auto broken=embedded;broken[16]^=1;auto corrupt=extract(broken);
    assert(corrupt && !corrupt->metadata && corrupt->canonical==png);
    auto duplicate=embedded;
    duplicate.insert(duplicate.begin()+8,embedded.begin()+8,embedded.begin()+8+text.size()+12);
    auto repeated=extract(duplicate);assert(repeated && !repeated->metadata && repeated->canonical==png);
    auto truncated=embedded;truncated.resize(16);assert(!extract(truncated));
    auto oversized=embedded;oversized[8]=0x7f;assert(!extract(oversized));
    assert(!extract(std::span(png).subspan(1)));
}
