#pragma once
#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include "trajectory.hpp"

namespace fc::blueprint {
// Indexes refer to the serialized building list, not world-specific building IDs.
struct Companion {
    int buildings{};
    std::map<int, Limits> fountains;
};
inline bool valid(Limits v) {
    return std::isfinite(v.drop) && v.drop>=0 && v.drop<=1000 && std::fmod(v.drop,50)==0
        && (v.horizontal==-1 || (std::isfinite(v.horizontal) && v.horizontal>=0
            && v.horizontal<=500 && std::fmod(v.horizontal,50)==0));
}
inline std::string encode(const Companion& data, const std::string& hash) {
    std::ostringstream out;
    out << "FountainControlBlueprint 1\n" << hash << '\n' << data.buildings << ' ' << data.fountains.size() << '\n';
    for (auto [index,value]:data.fountains)
        out << index << ' ' << value.drop << ' ' << value.horizontal << '\n';
    return out.str();
}
inline std::optional<Companion> decode(const std::string& text,const std::string& hash,int buildings) {
    if (text.size()>1048576 || hash.size()!=64 || buildings<0 || buildings>100000) return {};
    std::istringstream in(text);
    std::string magic, fingerprint;
    int version, count;
    Companion result;
    if (!(in>>magic>>version>>fingerprint>>result.buildings>>count) || magic!="FountainControlBlueprint"
        || version!=1 || fingerprint!=hash || result.buildings!=buildings || count<0 || count>buildings) return {};
    for (int n=0;n<count;++n) {
        int index; Limits value;
        if (!(in>>index>>value.drop>>value.horizontal) || index<0 || index>=buildings || !valid(value)
            || !result.fountains.emplace(index,value).second) return {};
    }
    in>>std::ws;
    if (!in.eof()) return {};
    return result;
}
}
